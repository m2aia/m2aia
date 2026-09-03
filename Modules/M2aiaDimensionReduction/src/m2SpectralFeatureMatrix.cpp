/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2SpectralFeatureMatrix.h>

#include <m2ImzMLSpectrumImage.h>
#include <m2SpectrumContainerImage.h>

#include <mitkImageAccessByItk.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImageReadAccessor.h>
#include <mitkImageWriteAccessor.h>

#include <itkShrinkImageFilter.h>

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace
{
  /** The spectra of a spectrum image are held by the concrete image classes, not by the base, so
      the index source has to ask each of them in turn. */
  template <class TImage>
  bool CollectSpectraIndicesOf(const m2::SpectrumImage *image,
                               const mitk::Image *mask,
                               std::vector<itk::Index<3>> &indices,
                               std::vector<unsigned int> &classes)
  {
    auto concreteImage = dynamic_cast<const TImage *>(image);
    if (!concreteImage)
      return false;

    if (!mask)
    {
      for (const auto &spectrum : concreteImage->GetSpectra())
      {
        indices.push_back(spectrum.index);
        classes.push_back(1);
      }
      return true;
    }

    mitk::ImagePixelReadAccessor<mitk::MultiLabelSegmentation::LabelValueType, 3> accessor(mask);
    for (const auto &spectrum : concreteImage->GetSpectra())
    {
      const auto labelValue = accessor.GetPixelByIndex(spectrum.index);
      if (labelValue != 0)
      {
        indices.push_back(spectrum.index);
        classes.push_back(static_cast<unsigned int>(labelValue));
      }
    }

    return true;
  }
} // namespace

void m2::SpectralFeatureMatrixBuilder::AddImage(const m2::SpectrumImage *image, const mitk::Image *mask)
{
  if (!image)
  {
    MITK_WARN << "No image passed to the feature matrix builder; it is skipped.";
    return;
  }

  m_Images.push_back({image, const_cast<mitk::Image *>(mask)});
}

void m2::SpectralFeatureMatrixBuilder::CollectValidPixels(const m2::SpectrumImage *image,
                                                          const mitk::Image *mask,
                                                          m2::FeatureIndexSource source,
                                                          std::vector<itk::Index<3>> &indices,
                                                          std::vector<unsigned int> &classes)
{
  if (source == m2::FeatureIndexSource::Spectra)
  {
    if (CollectSpectraIndicesOf<m2::ImzMLSpectrumImage>(image, mask, indices, classes) ||
        CollectSpectraIndicesOf<m2::SpectrumContainerImage>(image, mask, indices, classes))
      return;

    MITK_WARN << "The image does not expose its spectra; the mask is scanned instead.";
  }

  // the pixels are visited in x/y/z order, the order the results are scattered back in
  if (!mask)
  {
    const auto dimensions = image->GetDimensions();
    for (unsigned int x = 0; x < dimensions[0]; ++x)
      for (unsigned int y = 0; y < dimensions[1]; ++y)
        for (unsigned int z = 0; z < dimensions[2]; ++z)
        {
          indices.push_back({x, y, z});
          classes.push_back(1);
        }
    return;
  }

  AccessFixedDimensionByItk(
    mask,
    ([&indices, &classes](auto itkMask) {
      const auto size = itkMask->GetLargestPossibleRegion().GetSize();
      for (unsigned int x = 0; x < size[0]; ++x)
        for (unsigned int y = 0; y < size[1]; ++y)
          for (unsigned int z = 0; z < size[2]; ++z)
          {
            const auto labelValue = itkMask->GetPixel({{x, y, z}});
            if (labelValue > 0)
            {
              indices.push_back({{x, y, z}});
              classes.push_back(static_cast<unsigned int>(labelValue));
            }
          }
    }),
    3);
}

std::vector<itk::Index<3>> m2::SpectralFeatureMatrixBuilder::CollectValidIndices(const m2::SpectrumImage *image,
                                                                                const mitk::Image *mask,
                                                                                m2::FeatureIndexSource source)
{
  std::vector<itk::Index<3>> indices;
  std::vector<unsigned int> classes;
  CollectValidPixels(image, mask, source, indices, classes);

  return indices;
}

mitk::Image::Pointer m2::SpectralFeatureMatrixBuilder::Shrink(const mitk::Image *image, unsigned int factor)
{
  if (factor <= 1)
    return const_cast<mitk::Image *>(image);

  // cast into a fresh pointer, casting into the source would overwrite it
  mitk::Image::Pointer shrunkImage;
  AccessFixedDimensionByItk(
    image,
    ([factor, &shrunkImage](auto itkImage) {
      using ImageType = typename std::remove_cv<typename std::remove_reference<decltype(*itkImage)>::type>::type;
      auto shrinker = itk::ShrinkImageFilter<ImageType, ImageType>::New();
      shrinker->SetInput(itkImage);
      shrinker->SetShrinkFactor(0, factor);
      shrinker->SetShrinkFactor(1, factor);
      shrinker->SetShrinkFactor(2, 1);
      shrinker->Update();
      mitk::CastToMitkImage(shrinker->GetOutput(), shrunkImage);
    }),
    3);

  return shrunkImage;
}

void m2::SpectralFeatureMatrixBuilder::ApplyScaling(Eigen::MatrixXf &matrix, m2::FeatureScaling scaling)
{
  if (scaling == m2::FeatureScaling::None || matrix.rows() == 0)
    return;

  for (Eigen::Index column = 0; column < matrix.cols(); ++column)
  {
    auto columnValues = matrix.col(column);
    switch (scaling)
    {
      case m2::FeatureScaling::MeanCentered:
        columnValues.array() -= columnValues.mean();
        break;

      case m2::FeatureScaling::Standardized:
      {
        const float mean = columnValues.mean();
        columnValues.array() -= mean;
        // the sample standard deviation, as the t-SNE input has always been standardized with
        const float variance =
          matrix.rows() > 1 ? columnValues.squaredNorm() / static_cast<float>(matrix.rows() - 1) : 0.0f;
        const float deviation = std::sqrt(variance);
        if (deviation > 0.0f)
          columnValues /= deviation;
        break;
      }

      case m2::FeatureScaling::MinMax:
      {
        const float minimum = columnValues.minCoeff();
        const float maximum = columnValues.maxCoeff();
        if (maximum - minimum > 0.0f)
          columnValues = (columnValues.array() - minimum) / (maximum - minimum);
        else
          columnValues.setZero();
        break;
      }

      default:
        break;
    }
  }
}

m2::SpectralFeatureMatrix m2::SpectralFeatureMatrixBuilder::InitializeMatrix(unsigned int numberOfColumns) const
{
  m2::SpectralFeatureMatrix result;

  unsigned int shrinkFactor = m_ShrinkFactor;
  if (shrinkFactor > 1 && m_IndexSource == m2::FeatureIndexSource::Spectra)
  {
    // the indices of the spectra address the full resolution grid and cannot be shrunk with it
    MITK_WARN << "A shrink factor cannot be combined with the spectra index source; it is ignored.";
    shrinkFactor = 1;
  }

  for (const auto &entry : m_Images)
  {
    // the results live on the grid of the image, so the image and not the mask carries the
    // geometry they are given; both are shrunk the same way and stay on the same grid
    mitk::Image::Pointer reference = Shrink(static_cast<const mitk::Image *>(entry.image), shrinkFactor);
    mitk::Image::Pointer mask = entry.mask.IsNull() ? nullptr : Shrink(entry.mask, shrinkFactor);

    std::vector<itk::Index<3>> indices;
    std::vector<unsigned int> classes;
    CollectValidPixels(entry.image, mask, m_IndexSource, indices, classes);

    result.indices.insert(result.indices.end(), indices.begin(), indices.end());
    result.classes.insert(result.classes.end(), classes.begin(), classes.end());
    result.references.push_back(reference);
    result.imageRowOffsets.push_back(static_cast<unsigned int>(result.indices.size()));
  }

  if (result.indices.empty())
  {
    MITK_ERROR << "None of the images contributes a single pixel; check the mask and the label value.";
    return result;
  }

  result.data.resize(static_cast<Eigen::Index>(result.indices.size()), numberOfColumns);
  result.data.setZero();

  return result;
}

m2::SpectralFeatureMatrix m2::SpectralFeatureMatrixBuilder::Build(const std::vector<m2::Interval> &intervals) const
{
  if (m_Images.empty() || intervals.empty())
  {
    MITK_ERROR << "The feature matrix needs at least one image and one peak.";
    return {};
  }

  auto result = InitializeMatrix(static_cast<unsigned int>(intervals.size()));
  if (result.IsEmpty())
    return result;

  const auto totalSteps = static_cast<unsigned int>(m_Images.size() * intervals.size());
  unsigned int step = 0;

  for (unsigned int imageIndex = 0; imageIndex < m_Images.size(); ++imageIndex)
  {
    const auto &entry = m_Images[imageIndex];
    const auto rowOffset = result.imageRowOffsets[imageIndex];
    const auto rowCount = result.GetRowCount(imageIndex);

    // one ion image is allocated for the whole run and overwritten for every peak, so the memory
    // needed does not grow with the length of the peak list
    auto ionImage = mitk::Image::New();
    ionImage->Initialize(static_cast<const mitk::Image *>(entry.image));

    for (unsigned int column = 0; column < intervals.size(); ++column)
    {
      const auto mz = intervals[column].x.mean();
      entry.image->GetImage(mz, entry.image->ApplyTolerance(mz), entry.mask, ionImage);

      const auto sampledImage = Shrink(ionImage, m_ShrinkFactor);
      mitk::ImagePixelReadAccessor<m2::DisplayImagePixelType, 3> accessor(sampledImage);
      for (unsigned int row = 0; row < rowCount; ++row)
        result.data(rowOffset + row, column) = accessor.GetPixelByIndex(result.indices[rowOffset + row]);

      if (m_ProgressCallback)
        m_ProgressCallback(++step, totalSteps);
    }
  }

  ApplyScaling(result.data, m_Scaling);

  return result;
}

m2::SpectralFeatureMatrix m2::SpectralFeatureMatrixBuilder::BuildFromComponentImage(
  const mitk::Image *componentImage) const
{
  if (m_Images.size() != 1 || !componentImage)
  {
    MITK_ERROR << "A component image can only be turned into a feature matrix together with exactly one image.";
    return {};
  }

  const auto numberOfComponents = componentImage->GetPixelType().GetNumberOfComponents();
  auto result = InitializeMatrix(numberOfComponents);
  if (result.IsEmpty())
    return result;

  const auto rowCount = result.GetRowCount(0);
  const auto dimensions = componentImage->GetDimensions();
  const auto numberOfPixels = dimensions[0] * dimensions[1] * dimensions[2];

  // the components are unpacked one at a time; only one full resolution image exists at once
  auto scratchImage = mitk::Image::New();
  scratchImage->Initialize(static_cast<const mitk::Image *>(m_Images.front().image));

  mitk::ImageReadAccessor componentAccessor(componentImage);
  const auto *componentData = static_cast<const m2::DisplayImagePixelType *>(componentAccessor.GetData());

  for (unsigned int component = 0; component < numberOfComponents; ++component)
  {
    {
      mitk::ImageWriteAccessor scratchAccessor(scratchImage);
      auto *scratchData = static_cast<m2::DisplayImagePixelType *>(scratchAccessor.GetData());
      for (unsigned int pixel = 0; pixel < numberOfPixels; ++pixel)
        scratchData[pixel] = componentData[pixel * numberOfComponents + component];
    }

    const auto sampledImage = Shrink(scratchImage, m_ShrinkFactor);
    mitk::ImagePixelReadAccessor<m2::DisplayImagePixelType, 3> accessor(sampledImage);
    for (unsigned int row = 0; row < rowCount; ++row)
      result.data(row, component) = accessor.GetPixelByIndex(result.indices[row]);

    if (m_ProgressCallback)
      m_ProgressCallback(component + 1, numberOfComponents);
  }

  ApplyScaling(result.data, m_Scaling);

  return result;
}
