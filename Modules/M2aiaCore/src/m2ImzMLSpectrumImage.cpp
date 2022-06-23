/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2ImzMLSpectrumImage.h>
#include <m2Process.hpp>
#include <m2SpectrumImageProcessor.h>
#include <m2Timer.h>
#include <mitkImageAccessByItk.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkLabelSetImage.h>
#include <mitkProperties.h>
#include <mutex>
#include <signal/m2Baseline.h>
#include <signal/m2Morphology.h>
#include <signal/m2Normalization.h>
#include <signal/m2PeakDetection.h>
#include <signal/m2Pooling.h>
#include <signal/m2RunningMedian.h>
#include <signal/m2Smoothing.h>
#include <signal/m2Transformer.h>

void m2::ImzMLSpectrumImage::GetImage(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const
{
  m_Processor->GetImagePrivate(mz, tol, mask, img);
}

m2::ImzMLSpectrumImage::ImzMLImageSource &m2::ImzMLSpectrumImage::GetImzMLSpectrumImageSource(unsigned int i)
{
  if (i >= m_SourcesList.size())
    mitkThrow() << "No source exist for index " << i << " in the source list with size " << m_SourcesList.size();
  return m_SourcesList[i];
}

const m2::ImzMLSpectrumImage::ImzMLImageSource &m2::ImzMLSpectrumImage::GetImzMLSpectrumImageSource(
  unsigned int i) const
{
  if (i >= m_SourcesList.size())
    mitkThrow() << "No source exist for index " << i << " in the source list with size " << m_SourcesList.size();
  return m_SourcesList[i];
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::Processor<MassAxisType, IntensityType>::GetImagePrivate(double xRangeCenter,
                                                                                     double xRangeTol,
                                                                                     const mitk::Image *mask,
                                                                                     mitk::Image *destImage)
{
  AccessByItk(destImage, [](auto itkImg) { itkImg->FillBuffer(0); });
  using namespace m2;
  // accessors
  mitk::ImagePixelWriteAccessor<DisplayImagePixelType, 3> imageAccess(destImage);
  mitk::ImagePixelReadAccessor<NormImagePixelType, 3> normAccess(p->GetNormalizationImage());
  std::shared_ptr<mitk::ImagePixelReadAccessor<mitk::LabelSetImage::PixelType, 3>> maskAccess;

  if (mask)
    maskAccess.reset(new mitk::ImagePixelReadAccessor<mitk::LabelSetImage::PixelType, 3>(mask));

  p->SetProperty("x_range_center", mitk::DoubleProperty::New(xRangeCenter));
  p->SetProperty("x_range_tol", mitk::DoubleProperty::New(xRangeTol));
  auto mdMz = itk::MetaDataObject<double>::New();
  mdMz->SetMetaDataObjectValue(xRangeCenter);
  auto mdTol = itk::MetaDataObject<double>::New();
  mdTol->SetMetaDataObjectValue(xRangeTol);
  p->GetMetaDataDictionary()["x_range_center"] = mdMz;
  p->GetMetaDataDictionary()["x_range_tol"] = mdTol;

  // Profile (continuous) spectrum
  const auto spectrumType = p->GetSpectrumType();
  const unsigned t = p->GetNumberOfThreads();
  const bool useNormalization = p->GetNormalizationStrategy() != m2::NormalizationStrategyType::None;

  if (spectrumType.Format == m2::SpectrumFormat::ContinuousProfile)
  {
    // xRangeCenter subrange
    const auto mzs = p->GetXAxis();
    const auto _BaselineCorrectionHWS = p->GetBaseLineCorrectionHalfWindowSize();
    const auto _BaseLineCorrectionStrategy = p->GetBaselineCorrectionStrategy();

    // Image generateion is based on range operations on the full spectrum for each pixel.
    // Since we are not intrested in values outside of the range, read only values of interest
    // from the *.ibd file. For this we have to manipulate the byte offset position.
    // 1) This is the full spectrum from start '|' to end '|'
    // |-----------------------------------------------------------------|

    // 2) Subrange from '(' to ')' with center 'c', offset left '>' and offset right '<'
    // |>>>>>>>>>>>>>>>(********c********)<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<|
    auto subRes = m2::Signal::Subrange(mzs, xRangeCenter - xRangeTol, xRangeCenter + xRangeTol);
    const unsigned int offset_right = (mzs.size() - (subRes.first + subRes.second));
    const unsigned int offset_left = subRes.first;

    // 3) Pad the range for overcoming border-problems with kernel based operations
    // |>>>>>>>>>[^^^^^(********c********)^^^^^]<<<<<<<<<<<<<<<<<<<<<<<<<|
    const unsigned int padding_left =
      (offset_left / _BaselineCorrectionHWS >= 1 ? _BaselineCorrectionHWS : offset_left) *
      (_BaseLineCorrectionStrategy != m2::BaselineCorrectionType::None);
    const unsigned int padding_right =
      (offset_right / _BaselineCorrectionHWS >= 1 ? _BaselineCorrectionHWS : offset_right) *
      (_BaseLineCorrectionStrategy != m2::BaselineCorrectionType::None);

    // 4) We read data from the *ibd from '[' to ']' using a padded left offset
    // Continue at 5.
    const auto newLength = subRes.second + padding_left + padding_right;
    const auto newOffsetModifier = (subRes.first - padding_left) * sizeof(IntensityType);

    for (auto &source : p->GetImzMLSpectrumImageSourceList())
    {
      m2::Process::Map(
        source.m_Spectra.size(),
        t,
        [&](auto /*id*/, auto a, auto b)
        {
          std::ifstream f(source.m_BinaryDataPath, std::iostream::binary);
          std::vector<IntensityType> ints(newLength);
          std::vector<IntensityType> baseline(newLength);
          // 5) (For a specific thread), save the true range positions '(' and ')'
          // for pooling in the data vector. Continue at 6.
          // |>>>>>>>>>[^^^^^(********c********)^^^^^]<<<<<<<<<<<<<<<<<<<<<<<<<|
          auto s = std::next(std::begin(ints), padding_left);
          auto e = std::prev(std::end(ints), padding_right);

          for (unsigned int i = a; i < b; ++i)
          {
            const auto &spectrum = source.m_Spectra[i];

            // check if outside of mask
            if (maskAccess && maskAccess->GetPixelByIndex(spectrum.index + source.m_Offset) == 0)
            {
              imageAccess.SetPixelByIndex(spectrum.index + source.m_Offset, 0);
              continue;
            }
            // 6) (For a specific pixel) recalculate the new offset
            // |>>>>>>>>>[^^^^^(********c********)^^^^^]<<<<<<<<<<<<<<<<<<<<<<<<<|
            const auto newOffset = source.m_Spectra[i].intOffset + newOffsetModifier; // new offset
            binaryDataToVector(f, newOffset, newLength, ints.data());

            // ----- Normalization
            if (useNormalization)
            { // check if it is not NormalizationStrategy::None.
              IntensityType norm = normAccess.GetPixelByIndex(spectrum.index + source.m_Offset);
              std::transform(std::begin(ints), std::end(ints), std::begin(ints), [&norm](auto &v) { return v / norm; });
            }

            // ----- Smoothing
            m_Smoother.operator()(std::begin(ints), std::end(ints));

            // ----- Baseline Substraction
            m_BaselineSubstractor(std::begin(ints), std::end(ints), std::begin(baseline));

            // ----- Intensity Transformation
            m_Transformer(std::begin(ints), std::end(ints));

            // ----- Pool the range
            const auto val = Signal::RangePooling<IntensityType>(s, e, p->GetRangePoolingStrategy());

            // finally set the pixel value
            imageAccess.SetPixelByIndex(spectrum.index + source.m_Offset, val);
          }
        });
    }
  }
  else if (any(spectrumType.Format & (m2::SpectrumFormat::ContinuousCentroid | m2::SpectrumFormat::ProcessedCentroid |
                                      m2::SpectrumFormat::ProcessedProfile)))
  {
    for (auto &source : p->GetImzMLSpectrumImageSourceList())
    {
      m2::Process::Map(
        source.m_Spectra.size(),
        t,
        [&](auto /*id*/, auto a, auto b)
        {
          std::ifstream f(source.m_BinaryDataPath, std::iostream::binary);
          std::vector<IntensityType> ints;
          std::vector<MassAxisType> mzs;
          for (unsigned int i = a; i < b; ++i)
          {
            auto &spectrum = source.m_Spectra[i];
            if (maskAccess && maskAccess->GetPixelByIndex(spectrum.index + source.m_Offset) == 0)
            {
              imageAccess.SetPixelByIndex(spectrum.index + source.m_Offset, 0);
              continue;
            }

            mzs.resize(spectrum.mzLength);
            binaryDataToVector(
              f, spectrum.mzOffset, spectrum.mzLength, mzs.data()); // !! read mass axis for each spectrum

            auto subRes = m2::Signal::Subrange(mzs, xRangeCenter - xRangeTol, xRangeCenter + xRangeTol);
            if (subRes.second == 0)
            {
              imageAccess.SetPixelByIndex(spectrum.index + source.m_Offset, 0);
              continue;
            }

            ints.resize(subRes.second);
            binaryDataToVector(
              f, spectrum.intOffset + subRes.first * sizeof(IntensityType), subRes.second, ints.data());

            // TODO: Is it useful to normalize centroid data?
            if (p->GetNormalizationStrategy() != m2::NormalizationStrategyType::None)
            {
              std::transform(std::begin(ints),
                             std::end(ints),
                             std::begin(ints),
                             [&](auto &v) { return v / spectrum.normalizationFactor; });
            }

            auto val =
              Signal::RangePooling<IntensityType>(std::begin(ints), std::end(ints), p->GetRangePoolingStrategy());
            imageAccess.SetPixelByIndex(spectrum.index + source.m_Offset, val);
          }
        });
    }
  }
}

void m2::ImzMLSpectrumImage::InitializeProcessor()
{
  auto intensitiesDataTypeString = GetPropertyValue<std::string>("intensity array value type");
  auto mzValueTypeString = GetPropertyValue<std::string>("m/z array value type");
  if (mzValueTypeString.compare("32-bit float") == 0)
  {
    m_SpectrumType.XAxisType = m2::NumericType::Float;
    if (intensitiesDataTypeString.compare("32-bit float") == 0)
    {
      m_SpectrumType.YAxisType = m2::NumericType::Float;
      this->m_Processor.reset((ProcessorBase *)new Processor<float, float>(this));
    }
    else if (intensitiesDataTypeString.compare("64-bit float") == 0)
    {
      m_SpectrumType.YAxisType = m2::NumericType::Double;
      this->m_Processor.reset((ProcessorBase *)new Processor<float, double>(this));
    }
    else if (intensitiesDataTypeString.compare("32-bit integer") == 0)
    {
      MITK_ERROR(GetStaticNameOfClass()) << "Using 32-bit integer. Not implemented!";
      // this->m_Processor.reset((ProcessorBase *)new Processor<float, long int>(this));
      // SetYInputType(m2::NumericType::Double);
    }
    else if (intensitiesDataTypeString.compare("64-bit integer") == 0)
    {
      MITK_ERROR(GetStaticNameOfClass()) << "Using 64-bit integer. Not implemented!";
      // this->m_Processor.reset((ProcessorBase *)new Processor<float, long long int>(this));
      // SetYInputType(m2::NumericType::Double);
    }
  }
  else if (mzValueTypeString.compare("64-bit float") == 0)
  {
    m_SpectrumType.XAxisType = m2::NumericType::Double;
    if (intensitiesDataTypeString.compare("32-bit float") == 0)
    {
      m_SpectrumType.YAxisType = m2::NumericType::Float;
      this->m_Processor.reset((ProcessorBase *)new Processor<double, float>(this));
    }
    else if (intensitiesDataTypeString.compare("64-bit float") == 0)
    {
      m_SpectrumType.YAxisType = m2::NumericType::Double;
      this->m_Processor.reset((ProcessorBase *)new Processor<double, double>(this));
    }
    else if (intensitiesDataTypeString.compare("32-bit integer") == 0)
    {
      MITK_ERROR(GetStaticNameOfClass()) << "Using 32-bit integer. Not implemented!";
      // this->m_Processor.reset((ProcessorBase *)new Processor<double, long int>(this));
      // SetYInputType(m2::NumericType::Double);
    }
    else if (intensitiesDataTypeString.compare("64-bit integer") == 0)
    {
      // this->m_Processor.reset((ProcessorBase *)new Processor<double, long long int>(this));
      MITK_ERROR(GetStaticNameOfClass()) << "Using 64-bit integer. Not implemented!";
      // SetYInputType(m2::NumericType::Double);
    }
  }
}

void m2::ImzMLSpectrumImage::InitializeGeometry()
{
  if (!m_Processor)
    this->InitializeProcessor();
  this->m_Processor->InitializeGeometry();
  this->SetImageGeometryInitialized(true);
}

void m2::ImzMLSpectrumImage::InitializeImageAccess()
{
  this->m_Processor->InitializeImageAccess();
  this->SetImageAccessInitialized(true);

  // by default set the export type to import type.
  SetExportSpectrumType(GetSpectrumType());
}

m2::ImzMLSpectrumImage::Pointer m2::ImzMLSpectrumImage::Combine(const m2::ImzMLSpectrumImage *A,
                                                                const m2::ImzMLSpectrumImage *B,
                                                                const char stackAxis)
{
  auto C = m2::ImzMLSpectrumImage::New();
  C->SetSpectrumType(A->GetSpectrumType());
  {
    auto A_mzGroup = A->GetPropertyValue<std::string>("mzGroupName");
    auto A_mzValueType = A->GetPropertyValue<std::string>(A_mzGroup + " value type");
    auto B_mzGroup = B->GetPropertyValue<std::string>("mzGroupName");
    auto B_mzValueType = B->GetPropertyValue<std::string>(B_mzGroup + " value type");
    if (A_mzValueType.compare(B_mzValueType) != 0)
      mitkThrow() << "m/z value types must the same!";

    auto typeSizeInBytes = A->GetPropertyValue<unsigned>(A_mzGroup + " value type (bytes)");
    auto refGroupID = A->GetPropertyValue<std::string>(A_mzGroup);

    C->SetPropertyValue<std::string>(A_mzGroup, refGroupID);
    C->SetPropertyValue<unsigned>(A_mzGroup + " value type (bytes)", typeSizeInBytes);
    C->SetPropertyValue<std::string>(A_mzGroup + " value type", A_mzValueType);
    C->SetPropertyValue<std::string>("mzGroupName", A_mzGroup);
  }

  {
    auto A_intensityGroup = A->GetPropertyValue<std::string>("intensityGroupName");
    auto A_intensityValueType = A->GetPropertyValue<std::string>(A_intensityGroup + " value type");
    auto B_intensityGroup = B->GetPropertyValue<std::string>("intensityGroupName");
    auto B_intensityValueType = B->GetPropertyValue<std::string>(B_intensityGroup + " value type");
    if (B_intensityValueType.compare(A_intensityValueType) != 0)
      mitkThrow() << "Intensity value types must the same!";

    auto typeSizeInBytes = A->GetPropertyValue<unsigned>(A_intensityGroup + " value type (bytes)");
    auto refGroupID = A->GetPropertyValue<std::string>(A_intensityGroup);

    C->SetPropertyValue<std::string>(A_intensityGroup, refGroupID);
    C->SetPropertyValue<unsigned>(A_intensityGroup + " value type (bytes)", typeSizeInBytes);
    C->SetPropertyValue<std::string>(A_intensityGroup + " value type", A_intensityValueType);
    C->SetPropertyValue<std::string>("intensityGroupName", A_intensityGroup);
  }

  {
    auto A_mass = A->GetXAxis();
    auto B_mass = B->GetXAxis();
    auto isMassEqual = std::equal(std::begin(A_mass), std::end(A_mass), std::begin(B_mass));
    if (!isMassEqual)
      mitkThrow() << "Mass axis must be the same!";
  }

  {
    auto A_x = A->GetPropertyValue<double>("pixel size x");
    auto A_y = A->GetPropertyValue<double>("pixel size y");
    auto A_z = A->GetPropertyValue<double>("pixel size z");

    auto B_x = B->GetPropertyValue<double>("pixel size x");
    auto B_y = B->GetPropertyValue<double>("pixel size y");
    auto B_z = B->GetPropertyValue<double>("pixel size z");

    if ((std::abs(A_x - B_x) > 10e-5 && std::abs(A_y - B_y) > 10e-5) || std::abs(A_z - B_z) > 10e-5)
      mitkThrow() << "Pixel size must be the same!";

    C->SetPropertyValue<double>("pixel size x", A_x);
    C->SetPropertyValue<double>("pixel size y", A_y);
    C->SetPropertyValue<double>("pixel size z", A_z);
  }

  unsigned A_x, A_y, A_z;
  unsigned B_x, B_y, B_z;
  A_x = A->GetPropertyValue<unsigned>("max count of pixel x");
  B_x = B->GetPropertyValue<unsigned>("max count of pixel x");
  A_y = A->GetPropertyValue<unsigned>("max count of pixel y");
  B_y = B->GetPropertyValue<unsigned>("max count of pixel y");
  A_z = A->GetPropertyValue<unsigned>("max count of pixel z");
  B_z = B->GetPropertyValue<unsigned>("max count of pixel z");

  switch (stackAxis)
  {
    case 'x':
    {
      // Images are stacked along the x axis
      C->SetPropertyValue<unsigned>("max count of pixel x", A_x + B_x);
      C->SetPropertyValue<unsigned>("max count of pixel y", std::max(A_y, B_y));
      C->SetPropertyValue<unsigned>("max count of pixel z", std::max(A_z, B_z));
    }
    break;
    case 'y':
    {
      C->SetPropertyValue<unsigned>("max count of pixel x", std::max(A_x, B_x));
      C->SetPropertyValue<unsigned>("max count of pixel y", A_y + B_y);
      C->SetPropertyValue<unsigned>("max count of pixel z", std::max(A_z, B_z));
    }
    break;
    case 'z':
    {
      C->SetPropertyValue<unsigned>("max count of pixel x", std::max(A_x, B_x));
      C->SetPropertyValue<unsigned>("max count of pixel y", std::max(A_y, B_y));
      C->SetPropertyValue<unsigned>("max count of pixel z", A_z + B_z);
    }
    break;
  }

  {
    C->SetPropertyValue<double>("origin x", 0.0);
    C->SetPropertyValue<double>("origin y", 0.0);
    C->SetPropertyValue<double>("origin z", 0.0);
  }

  C->SetPropertyValue<bool>("continuous", true);

  auto &C_sources = C->GetImzMLSpectrumImageSourceList();

  const auto &A_sources = A->GetImzMLSpectrumImageSourceList();
  const auto &B_sources = B->GetImzMLSpectrumImageSourceList();

  C_sources = A_sources;
  // MITK_INFO(m2::ImzMLSpectrumImage::GetStaticNameOfClass()) << "Merge sources";

  std::transform(std::begin(B_sources),
                 std::end(B_sources),
                 std::back_inserter(C_sources),
                 [&](ImzMLImageSource s)
                 {
                   if (stackAxis == 'x')
                     s.m_Offset[0] += A_x; // shift along x-axis
                   if (stackAxis == 'y')
                     s.m_Offset[1] += A_y; // shift along y-axis
                   if (stackAxis == 'z')
                     s.m_Offset[2] += A_z; // shift along z-axis
                   return s;
                 });

  C->InitializeGeometry();
  std::set<m2::Peak> set;
  set.insert(std::begin(A->GetPeaks()), std::end(A->GetPeaks()));
  set.insert(std::begin(B->GetPeaks()), std::end(B->GetPeaks()));

  C->GetPeaks().insert(std::end(C->GetPeaks()), std::begin(set), std::end(set));
  return C;
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::Processor<MassAxisType, IntensityType>::InitializeGeometry()
{
  auto &imageArtifacts = p->GetImageArtifacts();

  std::array<itk::SizeValueType, 3> imageSize = {p->GetPropertyValue<unsigned>("max count of pixel x"),
                                                 p->GetPropertyValue<unsigned>("max count of pixel y"),
                                                 p->GetPropertyValue<unsigned>("max count of pixel z")};

  std::array<double, 3> imageOrigin = {p->GetPropertyValue<double>("origin x"),
                                       p->GetPropertyValue<double>("origin y"),
                                       p->GetPropertyValue<double>("origin z")};

  using ImageType = itk::Image<m2::DisplayImagePixelType, 3>;
  auto itkIonImage = ImageType::New();

  itkIonImage->SetRegions({{0, 0, 0}, {imageSize[0], imageSize[1], imageSize[2]}});
  itkIonImage->Allocate();
  itkIonImage->FillBuffer(0);

  auto s = itkIonImage->GetSpacing();
  auto o = itkIonImage->GetOrigin();
  o[0] = imageOrigin[0];
  o[1] = imageOrigin[1];
  o[2] = imageOrigin[2];

  //
  s[0] = p->GetPropertyValue<double>("pixel size x");
  s[1] = p->GetPropertyValue<double>("pixel size y");
  s[2] = p->GetPropertyValue<double>("pixel size z");

  auto d = itkIonImage->GetDirection();

  itkIonImage->SetSpacing(s);
  itkIonImage->SetOrigin(o);
  itkIonImage->SetDirection(d);

  {
    using LocalImageType = itk::Image<m2::DisplayImagePixelType, 3>;
    auto caster = itk::CastImageFilter<ImageType, LocalImageType>::New();
    caster->SetInput(itkIonImage);
    caster->Update();
    p->InitializeByItk(caster->GetOutput());

    mitk::ImagePixelWriteAccessor<m2::DisplayImagePixelType, 3> acc(p);
    std::memset(acc.GetData(), 0, imageSize[0] * imageSize[1] * imageSize[2] * sizeof(m2::DisplayImagePixelType));
  }

  {
    using LocalImageType = itk::Image<m2::IndexImagePixelType, 3>;
    auto caster = itk::CastImageFilter<ImageType, LocalImageType>::New();
    caster->SetInput(itkIonImage);
    caster->Update();
    auto indexImage = mitk::Image::New();
    imageArtifacts["index"] = indexImage;
    indexImage->InitializeByItk(caster->GetOutput());

    mitk::ImagePixelWriteAccessor<m2::IndexImagePixelType, 3> acc(indexImage);
    std::memset(acc.GetData(), 0, imageSize[0] * imageSize[1] * imageSize[2] * sizeof(m2::IndexImagePixelType));
  }

  {
    auto image = mitk::LabelSetImage::New();
    imageArtifacts["mask"] = image.GetPointer();
    image->Initialize((mitk::Image *)p);
    auto ls = image->GetActiveLabelSet();

    mitk::Color color;
    color.Set(0.0, 1, 0.0);
    auto label = mitk::Label::New();
    label->SetColor(color);
    label->SetName("Valid Spectrum");
    label->SetOpacity(0.0);
    label->SetLocked(true);
    label->SetValue(1);
    ls->AddLabel(label);
  }

  {
    using LocalImageType = itk::Image<m2::NormImagePixelType, 3>;
    auto caster = itk::CastImageFilter<ImageType, LocalImageType>::New();
    caster->SetInput(itkIonImage);
    caster->Update();
    auto normImage = mitk::Image::New();
    imageArtifacts["NormalizationImage"] = normImage;
    normImage->InitializeByItk(caster->GetOutput());

    mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3> acc(normImage);
    std::memset(acc.GetData(), 1, imageSize[0] * imageSize[1] * imageSize[2] * sizeof(m2::NormImagePixelType));
  }

  mitk::ImagePixelWriteAccessor<m2::DisplayImagePixelType, 3> acc(p);
  auto max_dim0 = p->GetDimensions()[0];
  auto max_dim1 = p->GetDimensions()[1];
  acc.SetPixelByIndex({0, 0, 0}, 1);
  acc.SetPixelByIndex({0, max_dim1 - 1, 0}, max_dim1 / 2);
  acc.SetPixelByIndex({max_dim0 - 1, 0, 0}, max_dim0 / 2);
  acc.SetPixelByIndex({max_dim0 - 1, max_dim1 - 1, 0}, max_dim1 + max_dim0);
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::Processor<MassAxisType, IntensityType>::InitializeImageAccess()
{
  //////////---------------------------
  m_Smoother.Initialize(p->GetSmoothingStrategy(), p->GetSmoothingHalfWindowSize());
  m_BaselineSubstractor.Initialize(p->GetBaselineCorrectionStrategy(), p->GetBaseLineCorrectionHalfWindowSize());
  m_Transformer.Initialize(p->GetIntensityTransformationStrategy());

  const auto spectrumType = p->GetSpectrumType();

  // MITK_INFO(m2::ImzMLSpectrumImage::GetStaticNameOfClass()) << m2::to_string(spectrumType);

  if (spectrumType.Format == m2::SpectrumFormat::ProcessedProfile)
  {
    // mitkThrow() << m2::ImzMLSpectrumImage::GetStaticNameOfClass() << R"(
    // This ImzML file seems to contain profile spectra in a processed memory order.
    // This is not supported in M2aia! If there are really individual m/z axis for
    // each spectrum, please resample the m/z axis and create one that is commonly
    // used for all spectra. Save it as continuous ImzML!)";
    InitializeImageAccessProcessedProfile();
  }
  else if (spectrumType.Format == m2::SpectrumFormat::ContinuousProfile)
    InitializeImageAccessContinuousProfile();
  else if (spectrumType.Format == m2::SpectrumFormat::ProcessedCentroid)
    InitializeImageAccessProcessedCentroid();
  else if (spectrumType.Format == m2::SpectrumFormat::ContinuousCentroid)
    InitializeImageAccessContinuousCentroid();

  // DEFAULT
  // INITIALIZE MASK, INDEX, NORMALIZATION IMAGES
  auto accMask = std::make_shared<mitk::ImagePixelWriteAccessor<mitk::LabelSetImage::PixelType, 3>>(p->GetMaskImage());
  auto accIndex = std::make_shared<mitk::ImagePixelWriteAccessor<m2::IndexImagePixelType, 3>>(p->GetIndexImage());
  auto accNorm = std::make_shared<mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3>>(p->GetNormalizationImage());
  for (const auto &source : p->GetImzMLSpectrumImageSourceList())
  {
    const auto &spectra = source.m_Spectra;
    m2::Process::Map(spectra.size(),
                     p->GetNumberOfThreads(),
                     [&](unsigned int /*t*/, unsigned int a, unsigned int b)
                     {
                       for (unsigned int i = a; i < b; i++)
                       {
                         const auto &spectrum = spectra[i];

                         accIndex->SetPixelByIndex(spectrum.index + source.m_Offset, i);

                         // If mask content is generated elsewhere
                         if (!p->GetUseExternalMask())
                           accMask->SetPixelByIndex(spectrum.index + source.m_Offset, 1);

                         // If it is a processed file, normalization maps are set to 1 - assuming that spectra were
                         // already processed if (any(importMode & (m2::SpectrumFormatType::ProcessedCentroid |
                         // m2::SpectrumFormatType::ProcessedProfile)))
                         //   accNorm->SetPixelByIndex(spectrum.index + source.m_Offset, 1);
                       }
                     });
  }

  // reset prevention flags
  p->UseExternalMaskOff();
  p->UseExternalNormalizationOff();
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::Processor<MassAxisType, IntensityType>::InitializeImageAccessProcessedProfile()
{
  // MITK_INFO("m2::ImzMLSpectrumImage") << "Start InitializeImageAccessProcessedProfile";
  InitializeImageAccessProcessedData();
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::Processor<MassAxisType, IntensityType>::InitializeImageAccessContinuousProfile()
{
  auto accNorm = std::make_shared<mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3>>(p->GetNormalizationImage());

  auto &source = p->GetImzMLSpectrumImageSourceList().front();
  std::vector<std::vector<double>> skylineT;
  std::vector<std::vector<double>> sumT;
  std::vector<MassAxisType> mzs;
  const auto normalizationStrategy = p->GetNormalizationStrategy();

  // load m/z axis
  {
    const auto &spectra = source.m_Spectra;
    std::ifstream f(source.m_BinaryDataPath, std::ios::binary);
    mzs.resize(spectra[0].mzLength);
    binaryDataToVector(f, spectra[0].mzOffset, spectra[0].mzLength, mzs.data());
    auto &massAxis = p->GetXAxis();
    massAxis.clear();
    std::copy(std::begin(mzs), std::end(mzs), std::back_inserter(massAxis));
    p->SetPropertyValue<unsigned>("spectral depth", mzs.size());
    p->SetPropertyValue<double>("x_min", mzs.front());
    p->SetPropertyValue<double>("x_max", mzs.back());
  }

  skylineT.resize(p->GetNumberOfThreads(), std::vector<double>(mzs.size(), 0));
  sumT.resize(p->GetNumberOfThreads(), std::vector<double>(mzs.size(), 0));

  const auto Maximum = [](const auto &a, const auto &b) { return a > b ? a : b; };
  const auto plus = std::plus<>();

  // m2::Timer t("Initialize image");
  for (auto &source : p->GetImzMLSpectrumImageSourceList())
  {
    auto &spectra = source.m_Spectra;

    m2::Process::Map(
      source.m_Spectra.size(),
      p->GetNumberOfThreads(),
      [&](unsigned int t, unsigned int a, unsigned int b)
      {
        std::vector<IntensityType> ints(mzs.size(), 0);
        std::vector<IntensityType> baseline(mzs.size(), 0);
        std::ifstream f(source.m_BinaryDataPath, std::ifstream::binary);

        for (unsigned long int i = a; i < b; i++)
        {
          auto &spectrum = spectra[i];

          // Read data from file ------------
          binaryDataToVector(f, spectrum.intOffset, spectrum.intLength, ints.data());

          // std::transform(std::begin(ints),std::end(ints),std::begin(ints),[](auto & a){return std::log(a);});

          if (ints.front() == 0)
            ints[0] = ints[1];
          if (ints.back() == 0)
            ints.back() = *(ints.rbegin() + 1);

          // --------------------------------

          if (!p->GetUseExternalNormalization())
          {
            // if (normalizationStrategy == m2::NormalizationStrategyType::InFile)
            //   spectrum.normalize = spectrum.normalize;
            spectrum.normalizationFactor = GetNormalizationFactor(
              normalizationStrategy, &mzs.front(), &mzs.back() + 1, &ints.front(), &ints.back() + 1);

            if (normalizationStrategy == m2::NormalizationStrategyType::InFile)
              spectrum.normalizationFactor = spectrum.inFileNormalizationFactor;

            accNorm->SetPixelByIndex(spectrum.index + source.m_Offset,
                                     spectrum.normalizationFactor); // Set normalization image pixel value
          }
          else
          {
            // Normalization-image content was set elsewhere
            spectrum.normalizationFactor = accNorm->GetPixelByIndex(spectrum.index + source.m_Offset);
          }
          std::transform(std::begin(ints),
                         std::end(ints),
                         std::begin(ints),
                         [&spectrum](const auto &a) { return a / spectrum.normalizationFactor; });

          m_Smoother(std::begin(ints), std::end(ints));
          m_BaselineSubstractor(std::begin(ints), std::end(ints), std::begin(baseline));
          m_Transformer(std::begin(ints), std::end(ints));


          std::transform(std::begin(ints), std::end(ints), sumT.at(t).begin(), sumT.at(t).begin(), plus);
          std::transform(std::begin(ints), std::end(ints), skylineT.at(t).begin(), skylineT.at(t).begin(), Maximum);
        }
      });
  }

  auto &skyline = p->SkylineSpectrum();
  skyline.resize(mzs.size(), 0);
  for (unsigned int t = 0; t < p->GetNumberOfThreads(); ++t)
    std::transform(skylineT[t].begin(),
                   skylineT[t].end(),
                   skyline.begin(),
                   skyline.begin(),
                   [](auto &a, auto &b) { return a > b ? a : b; });

  auto &mean = p->MeanSpectrum();
  auto &sum = p->SumSpectrum();

  mean.resize(mzs.size(), 0);
  sum.resize(mzs.size(), 0);

  auto N = std::accumulate(std::begin(p->GetImzMLSpectrumImageSourceList()),
                           std::end(p->GetImzMLSpectrumImageSourceList()),
                           unsigned(0),
                           [](const auto &a, const auto &source) { return a + source.m_Spectra.size(); });

  for (unsigned int t = 0; t < p->GetNumberOfThreads(); ++t)
    std::transform(sumT[t].begin(), sumT[t].end(), sum.begin(), sum.begin(), [](auto &a, auto &b) { return a + b; });
  std::transform(sum.begin(), sum.end(), mean.begin(), [&](auto &a) { return a / double(N); });
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::Processor<MassAxisType, IntensityType>::InitializeImageAccessContinuousCentroid()
{
  const auto normalizationStrategy = p->GetNormalizationStrategy();
  auto accNorm = std::make_shared<mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3>>(p->GetNormalizationImage());

  const auto &source = p->GetImzMLSpectrumImageSourceList().front();
  std::vector<MassAxisType> mzs;

  { // load continuous x axis
    const auto &spectra = source.m_Spectra;
    mzs.resize(spectra[0].mzLength);

    std::ifstream f(source.m_BinaryDataPath, std::ios::binary);
    binaryDataToVector(f, spectra[0].mzOffset, spectra[0].mzLength, mzs.data());

    auto &massAxis = p->GetXAxis();
    massAxis.clear();
    std::copy(std::begin(mzs), std::end(mzs), std::back_inserter(massAxis));
    p->SetPropertyValue<unsigned>("spectral depth", mzs.size());
    p->SetPropertyValue<double>("x_min", mzs.front());
    p->SetPropertyValue<double>("x_max", mzs.back());
  }

  // each thread pixelwise accumulate peaks
  std::vector<std::vector<m2::Peak>> peaksT(p->GetNumberOfThreads());
  for (auto &peaks : peaksT)
    peaks.resize(mzs.size());

  for (auto &source : p->GetImzMLSpectrumImageSourceList())
  {
    auto &spectra = source.m_Spectra;

    m2::Process::Map(spectra.size(),
                     p->GetNumberOfThreads(),
                     [&](unsigned int t, unsigned int a, unsigned int b)
                     {
                       std::ifstream f;
                       f.open(source.m_BinaryDataPath, std::ios::binary);

                       std::vector<IntensityType> ints;
                       auto iO = spectra[0].intOffset;
                       auto iL = spectra[0].intLength;
                       ints.resize(iL);

                       for (unsigned i = a; i < b; i++)
                       {
                         auto &spectrum = spectra[i];
                         iO = spectrum.intOffset;
                         binaryDataToVector(f, iO, iL, ints.data());

                         // Normalization
                         if (!p->GetUseExternalNormalization())
                         {
                           spectrum.normalizationFactor = GetNormalizationFactor(
                             normalizationStrategy, &mzs.front(), &mzs.back() + 1, &ints.front(), &ints.back() + 1);

                           if (normalizationStrategy == m2::NormalizationStrategyType::InFile)
                             spectrum.normalizationFactor = spectrum.inFileNormalizationFactor;

                           accNorm->SetPixelByIndex(spectrum.index, spectrum.normalizationFactor);

                           std::transform(std::begin(ints),
                                          std::end(ints),
                                          std::begin(ints),
                                          [&spectrum](auto &v) { return v / spectrum.normalizationFactor; });
                         }

                         for (size_t i = 0; i < mzs.size(); ++i)
                           peaksT[t][i].Insert(i, mzs[i], ints[i]);
                       }

                       f.close();
                     });

    auto &skyline = p->SkylineSpectrum();
    auto &sum = p->SumSpectrum();
    auto &mean = p->MeanSpectrum();
    skyline.clear();
    sum.clear();
    mean.clear();

    // merge all peaks
    std::vector<m2::Peak> finalPeaks(mzs.size());
    for (auto &peaks : peaksT)
      for (size_t i = 0; i < peaks.size(); ++i)
        finalPeaks[i].Insert(peaks[i]);

    for (const auto &peak : finalPeaks)
    {
      skyline.push_back(peak.GetYMax());
      sum.push_back(peak.GetYSum());
      mean.push_back(peak.GetY());
    }
  }
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::Processor<MassAxisType, IntensityType>::InitializeImageAccessProcessedCentroid()
{
  // MITK_INFO("m2::ImzMLSpectrumImage") << "Start InitializeImageAccessProcessedCentroid";
  InitializeImageAccessProcessedData();
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::Processor<MassAxisType, IntensityType>::InitializeImageAccessProcessedData()
{
  std::vector<std::list<m2::Peak>> peaksT(p->GetNumberOfThreads());
  for (auto &source : p->GetImzMLSpectrumImageSourceList())
  {
    auto &spectra = source.m_Spectra;
    const auto &T = p->GetNumberOfThreads();
    const auto &binsN = p->GetNumberOfBins();
    std::vector<double> xMin(T, std::numeric_limits<double>::max());
    std::vector<double> xMax(T, std::numeric_limits<double>::min());
    
    std::vector<std::vector<double>> yT(T, std::vector<double>(binsN, 0));
    std::vector<std::vector<double>> yMaxT(T, std::vector<double>(binsN, 0));
    std::vector<std::vector<unsigned int>> hT(T, std::vector<unsigned int>(binsN, 0));
    std::vector<std::vector<double>> xT(T, std::vector<double>(binsN, 0));

    const auto normalizationStrategy = p->GetNormalizationStrategy();
    auto accNorm =
      std::make_shared<mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3>>(p->GetNormalizationImage());

    // MITK_INFO << spectra.size();
    // MAP
    m2::Process::Map(spectra.size(),
                     T,
                     [&](unsigned int t, unsigned int a, unsigned int b)
                     {
                      std::ifstream f(source.m_BinaryDataPath, std::ios::binary);
                       std::vector<MassAxisType> mzs;
                       std::list<m2::Peak> peaks, tempList;
                       // find x min/max
                       for (unsigned i = a; i < b; i++)
                       {
                         const auto &mzO = spectra[i].mzOffset;
                         const auto &mzL = spectra[i].mzLength;
                         mzs.resize(mzL);
                         binaryDataToVector(f, mzO, mzL, mzs.data());
                         xMin[t] = std::min(xMin[t], (double)mzs.front());
                         xMax[t] = std::max(xMax[t], (double)mzs.back());
                       }

                     });

                       
    // find overall min/max
    double binSize = 1;
    double max = std::numeric_limits<double>::min();
    double min = std::numeric_limits<double>::max();

    max = *std::max_element(std::begin(xMax), std::end(xMax));
    min = *std::min_element(std::begin(xMin), std::end(xMin));
    binSize = (max - min) / double(binsN);

  
    m2::Process::Map(spectra.size(),
                     T,
                     [&](unsigned int t, unsigned int a, unsigned int b)
                     {
                       std::ifstream f(source.m_BinaryDataPath, std::ios::binary);
                       std::vector<MassAxisType> mzs;
                       std::vector<IntensityType> ints;

                       for (unsigned i = a; i < b; i++)
                       {
                         auto &spectrum = spectra[i];
                         const auto &mzO = spectrum.mzOffset;
                         const auto &mzL = spectrum.mzLength;
                         mzs.resize(mzL);
                         binaryDataToVector(f, mzO, mzL, mzs.data());

                         const auto &intO = spectrum.intOffset;
                         const auto &intL = spectrum.intLength;
                         ints.resize(intL);
                         binaryDataToVector(f, intO, intL, ints.data());

                         // Normalization
                         if (!p->GetUseExternalNormalization())
                         {
                           spectrum.normalizationFactor = GetNormalizationFactor(
                             normalizationStrategy, &mzs.front(), &mzs.back() + 1, &ints.front(), &ints.back() + 1);

                           if (normalizationStrategy == m2::NormalizationStrategyType::InFile)
                             spectrum.normalizationFactor = spectrum.inFileNormalizationFactor;

                           accNorm->SetPixelByIndex(spectrum.index, spectrum.normalizationFactor);

                           std::transform(std::begin(ints),
                                          std::end(ints),
                                          std::begin(ints),
                                          [&spectrum](auto &v) { return v / spectrum.normalizationFactor; });
                         }


                         for (unsigned int k = 0; k < mzs.size(); ++k)
                         {
                           // find index of the bin for the k'th m/z value of the pixel
                           auto j = (long)((mzs[k] - min) / binSize);

                           if (j >= binsN)
                             j = binsN - 1;
                           else if (j < 0)
                             j = 0;

                           xT[t][j] += mzs[k];                                   // mass sum
                           yT[t][j] += ints[k] < 10e-256 ? 0 : ints[k];              // intensitiy sum
                           yMaxT[t][j] = std::max(yMaxT[t][j], double(ints[k])); // intensitiy max
                           hT[t][j]++;                                           // hits
                         }
                       }

                       f.close();
                     });

    // REDUCE
    for (unsigned int i = 1; i < T; ++i)
    {
      for (int k = 0; k < binsN; ++k)
      {
        xT[0][k] += xT[i][k];
        yT[0][k] += yT[i][k];
        yMaxT[0][k] = std::max(yMaxT[0][k], yMaxT[i][k]);
        hT[0][k] += hT[i][k];
      }
    }

    // for(int i = 0 ; i < binsN; ++i){
    //   if(hT[0][i] > 0){
    //     MITK_INFO << xT[0][i]/(double)hT[0][i] << " " <<  yT[0][i] << " max(" << yMaxT[0][i] << ") [" << hT[0][i] << "]";
    //   }
    // }

    auto &mzAxis = p->GetXAxis();
    auto &sum = p->SumSpectrum();
    auto &mean = p->MeanSpectrum();
    auto &skyline = p->SkylineSpectrum();

    mzAxis.clear();
    sum.clear();
    mean.clear();
    skyline.clear();

    for (int k = 0; k < binsN; ++k)
    {
      if (hT[0][k] > 0)
      {
        mzAxis.push_back(xT[0][k] / (double)hT[0][k]);
        sum.push_back(yT[0][k]);
        mean.push_back(yT[0][k] / (double)hT[0][k]);
        skyline.push_back(yMaxT[0][k]);
      }
    }

    xT.clear();
    yT.clear();
    yMaxT.clear();
    hT.clear();
    
    p->SetPropertyValue<double>("x_min", mzAxis.front());
    p->SetPropertyValue<double>("x_max", mzAxis.back());
    p->SetPropertyValue<unsigned>("spectral depth (bins of overview spectrum)", mzAxis.size());
  }
}

void m2::ImzMLSpectrumImage::GetSpectrum(unsigned int id,
                                         std::vector<float> &xs,
                                         std::vector<float> &ys,
                                         unsigned int source) const
{
  m_Processor->GetXValues(id, xs, source);
  m_Processor->GetYValues(id, ys, source);
}

void m2::ImzMLSpectrumImage::GetIntensities(unsigned int id,
                                         std::vector<float> &ys,
                                         unsigned int source) const
{
  m_Processor->GetYValues(id, ys, source);
}

template <class MassAxisType, class IntensityType>
template <class OutputType>
void m2::ImzMLSpectrumImage::Processor<MassAxisType, IntensityType>::GetXValues(unsigned int id,
                                                                                std::vector<OutputType> &xd,
                                                                                unsigned int sourceId)
{
  const auto &source = p->m_SourcesList[sourceId];
  std::ifstream f(source.m_BinaryDataPath, std::ios::binary);

  const auto &spectrum = source.m_Spectra[id];
  const auto &length = spectrum.mzLength;
  const auto &offset = spectrum.mzOffset;

  if (std::is_same<MassAxisType, OutputType>::value)
  {
    xd.resize(length);
    binaryDataToVector(f, offset, length, xd.data());
  }
  else
  {
    std::vector<MassAxisType> xs;
    xs.resize(length);
    binaryDataToVector(f, offset, length, xs.data());
    // copy and convert
    xd.resize(length);
    std::copy(std::begin(xs), std::end(xs), std::begin(xd));
  }
}

template <class MassAxisType, class IntensityType>
template <class OutputType>
void m2::ImzMLSpectrumImage::Processor<MassAxisType, IntensityType>::GetYValues(unsigned int id,
                                                                                std::vector<OutputType> &yd,
                                                                                unsigned int sourceId)
{
  const auto &source = p->m_SourcesList[sourceId];
  std::ifstream f(source.m_BinaryDataPath, std::ios::binary);

  const auto &spectrum = source.m_Spectra[id];
  const auto &length = spectrum.intLength;
  const auto &offset = spectrum.intOffset;

  mitk::ImagePixelReadAccessor<m2::NormImagePixelType, 3> normAccess(p->GetNormalizationImage());

  {
    std::vector<IntensityType> ys;
    ys.resize(length);
    binaryDataToVector(f, offset, length, ys.data());
    if (p->GetNormalizationStrategy() != m2::NormalizationStrategyType::None)
    { // check if it is not NormalizationStrategy::None.
      IntensityType norm = normAccess.GetPixelByIndex(spectrum.index + source.m_Offset);
      std::transform(std::begin(ys), std::end(ys), std::begin(ys), [&norm](auto &v) { return v / norm; });
    }

    // ----- Smoothing
    m_Smoother(std::begin(ys), std::end(ys));

    // ----- Baseline Substraction
    std::vector<IntensityType> baseline(length);
    m_BaselineSubstractor(std::begin(ys), std::end(ys), std::begin(baseline));

    // ----- Intensity Transformation
    m_Transformer(std::begin(ys), std::end(ys));

    // copy and convert
    yd.resize(length);
    std::copy(std::begin(ys), std::end(ys), std::begin(yd));

  }
}

// template <class MassAxisType, class IntensityType>
// void m2::ImzMLSpectrumImage::ImzMLImageProcessor<MassAxisType, IntensityType>::GetSpectrumPrivate(
//   unsigned int id, std::vector<float> &xd, std::vector<float> &yd, unsigned int sourceId)
// {
//   mitk::ImagePixelReadAccessor<m2::NormImagePixelType, 3> normAcc(p->GetNormalizationImage());

//   const auto &source = p->m_SourcesList[sourceId];
//   std::ifstream f;
//   f.open(source.m_BinaryDataPath, std::ios::binary);

//   const auto &spectrumType = p->GetSpectrumType();
//   const auto &spectrum = source.m_Spectra[id];
//   const auto &length = spectrum.intLength;
//   const auto &ysOffset = spectrum.intOffset;

//   std::vector<IntensityType> ys;
//   ys.resize(length);
//   binaryDataToVector(f, ysOffset, length, ys.data());

//   // No preprocessing is applied for single spectrum export
//   yd.resize(length);
//   xd.resize(length);

//   std::copy(std::begin(ys), std::end(ys), std::begin(yd));

//   // continuous data -> mass axis already exist and can be used directly
//   if (any(spectrumType.Format & m2::SpectrumFormatType::ContinuousCentroid |
//   m2::SpectrumFormatType::ContinuousProfile))
//   {
//     auto &xAxis = p->GetXAxis();
//     std::copy(std::begin(xAxis), std::end(xAxis), std::begin(xd));
//   }
//   else
//   {
//     // processed data -> mass axis for individual pixels
//     const auto &xsOffset = spectrum.mzOffset;
//     if (spectrumType.XAxisType == m2::NumericType::Float)
//     {
//       // float must be converted to output type "double"
//       std::vector<MassAxisType> xs;
//       xs.resize(length);
//       binaryDataToVector(f, xsOffset, length, xs.data());
//       std::copy(std::begin(xs), std::end(xs), std::begin(xd));
//     }
//     else if (spectrumType.XAxisType == m2::NumericType::Double)
//     {
//       // copy directly
//       xd.resize(length);
//       binaryDataToVector(f, xsOffset, length, xd.data());
//     }
//   }
// }

// template <class MassAxisType, class IntensityType>
// void m2::ImzMLSpectrumImage::ImzMLImageProcessor<MassAxisType, IntensityType>::GrabMassPrivate(
//   unsigned long int index, std::vector<double> &mzs, unsigned int sourceIndex) const
// {
//   const auto &source = p->GetImzMLSpectrumImageSourceList()[sourceIndex];
//   const auto &spectrum = source.m_Spectra[index];
//   const auto &mzo = spectrum.mzOffset;
//   const auto &mzl = spectrum.mzLength;

//   std::ifstream f;
//   f.open(source.m_BinaryDataPath, std::ios::binary);

//   std::vector<MassAxisType> mzs_get;
//   binaryDataToVector(f, mzo, mzl, mzs_get);
//   mzs.clear();

//   std::copy(std::begin(mzs_get), std::end(mzs_get), std::back_inserter(mzs));
// }

m2::ImzMLSpectrumImage::~ImzMLSpectrumImage()
{
  // MITK_INFO(m2::ImzMLSpectrumImage::GetStaticNameOfClass()) << " destroyed!";
}

m2::ImzMLSpectrumImage::ImzMLSpectrumImage() : m2::SpectrumImageBase()
{
  // MITK_INFO(m2::ImzMLSpectrumImage::GetStaticNameOfClass()) << " created!";
  m_SpectrumType.XAxisLabel = "m/z";
  m_ExportSpectrumType.XAxisLabel = "m/z";
}
