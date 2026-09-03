/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2EmbeddingResult.h>

#include <mitkImageCast.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkLabelSetImage.h>

#include <itkVectorImage.h>

#include <algorithm>

namespace
{
  using VectorImageType = itk::VectorImage<m2::DisplayImagePixelType, 3>;
  using LabelValueType = mitk::MultiLabelSegmentation::LabelValueType;
}

mitk::Image::Pointer m2::EmbeddingResult::ToVectorImage(const Eigen::MatrixXf &embedding,
                                                        const std::vector<itk::Index<3>> &indices,
                                                        unsigned int rowOffset,
                                                        unsigned int rowCount,
                                                        const mitk::Image *reference)
{
  if (embedding.cols() == 0 || !reference)
  {
    MITK_ERROR << "An embedding without components cannot be turned into an image.";
    return nullptr;
  }

  const auto numberOfComponents = static_cast<unsigned int>(embedding.cols());
  const auto dimensions = reference->GetDimensions();

  VectorImageType::RegionType region;
  VectorImageType::SizeType size;
  size[0] = dimensions[0];
  size[1] = dimensions[1];
  size[2] = dimensions[2];
  region.SetSize(size);
  region.SetIndex({{0, 0, 0}});

  auto vectorImage = VectorImageType::New();
  vectorImage->SetVectorLength(numberOfComponents);
  vectorImage->SetRegions(region);
  vectorImage->Allocate();

  itk::VariableLengthVector<m2::DisplayImagePixelType> zero;
  zero.SetSize(numberOfComponents);
  zero.Fill(0);
  vectorImage->FillBuffer(zero);

  auto *outputData = vectorImage->GetBufferPointer();
  for (unsigned int row = 0; row < rowCount; ++row)
  {
    const auto &index = indices[rowOffset + row];
    const size_t offset = index[0] + size[0] * (index[1] + size[1] * index[2]);
    for (unsigned int component = 0; component < numberOfComponents; ++component)
      outputData[offset * numberOfComponents + component] = embedding(rowOffset + row, component);
  }

  mitk::Image::Pointer image;
  mitk::CastToMitkImage(vectorImage, image);

  // the itk image was allocated without a geometry, so it starts at the origin with unit spacing
  // and an identity direction; the whole geometry of the reference is taken over at once rather
  // than patching single fields onto that default
  image->SetClonedGeometry(reference->GetGeometry());

  return image;
}

mitk::Image::Pointer m2::EmbeddingResult::ToVectorImage(const Eigen::MatrixXf &embedding,
                                                        const m2::SpectralFeatureMatrix &features,
                                                        unsigned int imageIndex)
{
  return ToVectorImage(embedding,
                       features.indices,
                       features.imageRowOffsets.at(imageIndex),
                       features.GetRowCount(imageIndex),
                       features.references.at(imageIndex));
}

mitk::Image::Pointer m2::EmbeddingResult::ToLabelImage(const std::vector<unsigned int> &labels,
                                                       const std::vector<itk::Index<3>> &indices,
                                                       unsigned int rowOffset,
                                                       unsigned int rowCount,
                                                       const mitk::Image *reference)
{
  if (labels.empty() || !reference)
  {
    MITK_ERROR << "An empty cluster assignment cannot be turned into a label image.";
    return nullptr;
  }

  // A label image always has the pixel type of a label value, whatever the reference is made of.
  // The whole time geometry of the reference is used, not just the spatial one: a segmentation
  // built from this image takes its geometry over, and mitk::Image::Initialize does not set the
  // time bounds of a single time step correctly on its own.
  auto labelImage = mitk::Image::New();
  labelImage->Initialize(mitk::MakeScalarPixelType<LabelValueType>(), *reference->GetTimeGeometry());

  {
    mitk::ImagePixelWriteAccessor<LabelValueType, 3> accessor(labelImage);
    auto *data = accessor.GetData();
    const auto dimensions = labelImage->GetDimensions();
    std::fill(data, data + dimensions[0] * dimensions[1] * dimensions[2], 0);

    // zero stays the unlabeled value, so the clusters are shifted by one
    for (unsigned int row = 0; row < rowCount; ++row)
      accessor.SetPixelByIndex(indices[rowOffset + row], static_cast<LabelValueType>(labels[rowOffset + row] + 1));
  }

  return labelImage;
}

mitk::Image::Pointer m2::EmbeddingResult::ToLabelImage(const std::vector<unsigned int> &labels,
                                                       const m2::SpectralFeatureMatrix &features,
                                                       unsigned int imageIndex)
{
  return ToLabelImage(labels,
                      features.indices,
                      features.imageRowOffsets.at(imageIndex),
                      features.GetRowCount(imageIndex),
                      features.references.at(imageIndex));
}

void m2::EmbeddingResult::RescaleToByteRange(Eigen::MatrixXf &embedding, unsigned int rowOffset, unsigned int rowCount)
{
  if (rowCount == 0)
    return;

  for (Eigen::Index component = 0; component < embedding.cols(); ++component)
  {
    auto block = embedding.block(rowOffset, component, rowCount, 1);
    const float minimum = block.minCoeff();
    const float maximum = block.maxCoeff();
    if (maximum - minimum > 0.0f)
      block = (block.array() - minimum) / (maximum - minimum) * 255.0f;
    else
      block.setZero();
  }
}
