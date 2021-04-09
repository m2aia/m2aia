/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <itkBarrier.h>
#include <m2Baseline.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2Morphology.h>
#include <m2Normalization.h>
#include <m2PeakDetection.h>
#include <m2Pooling.h>
#include <m2Process.hpp>
#include <m2RunningMedian.h>
#include <m2Smoothing.h>
#include <mitkImageAccessByItk.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkLabelSetImage.h>
#include <mitkProperties.h>
#include <mitkTimer.h>
#include <mutex>

template <class T>
void m2::ImzMLSpectrumImage::binaryDataToVector(std::ifstream &f,
                                                const unsigned long long int &offset,
                                                const unsigned long int &length,
                                                std::vector<T> &vec) noexcept
{
  vec.resize(length);
  f.seekg(offset);
  f.read((char *)vec.data(), length * sizeof(T));
}

void m2::ImzMLSpectrumImage::GenerateImageData(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const
{
  m2::SpectrumImageBase::GenerateImageData(mz, tol, mask, img);
}

m2::ImzMLSpectrumImage::Source &m2::ImzMLSpectrumImage::GetSpectrumImageSource(unsigned int i)
{
  if (i >= m_SourcesList.size())
    mitkThrow() << "No source exist for index " << i << " in the source list with size " << m_SourcesList.size();
  return m_SourcesList[i];
}

const m2::ImzMLSpectrumImage::Source &m2::ImzMLSpectrumImage::GetSpectrumImageSource(unsigned int i) const
{
  if (i >= m_SourcesList.size())
    mitkThrow() << "No source exist for index " << i << " in the source list with size " << m_SourcesList.size();
  return m_SourcesList[i];
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::ImzMLProcessor<MassAxisType, IntensityType>::CreateIonImagePrivate(
  double xRangeCenter, double xRangeTol, const mitk::Image *mask, mitk::Image *destImage) const
{
  AccessByItk(destImage, [](auto itkImg) { itkImg->FillBuffer(0); });
  using namespace m2;
  // accessors
  mitk::ImagePixelWriteAccessor<DisplayImagePixelType, 3> imageAccess(destImage);
  mitk::ImagePixelReadAccessor<NormImagePixelType, 3> normAccess(p->GetNormalizationImage());
  std::shared_ptr<mitk::ImagePixelReadAccessor<mitk::LabelSetImage::PixelType, 3>> maskAccess;

  if (mask)
  {
    maskAccess.reset(new mitk::ImagePixelReadAccessor<mitk::LabelSetImage::PixelType, 3>(mask));
    MITK_INFO << "> Use mask image";
  }
  p->SetProperty("x_range_center", mitk::DoubleProperty::New(xRangeCenter));
  p->SetProperty("x_range_tol", mitk::DoubleProperty::New(xRangeTol));
  auto mdMz = itk::MetaDataObject<double>::New();
  mdMz->SetMetaDataObjectValue(xRangeCenter);
  auto mdTol = itk::MetaDataObject<double>::New();
  mdTol->SetMetaDataObjectValue(xRangeTol);
  p->GetMetaDataDictionary()["x_range_center"] = mdMz;
  p->GetMetaDataDictionary()["x_range_tol"] = mdTol;

  std::vector<MassAxisType> mzs;
  std::vector<double> kernel;

  // Profile (continuous) spectrum
  const auto importMode = p->GetImportMode();
  if (importMode == m2::SpectrumFormatType::ContinuousProfile)
  {
    // xRangeCenter subrange
    const auto mzs = p->GetXAxis();
    auto subRes = m2::Signal::Subrange(mzs, xRangeCenter - xRangeTol, xRangeCenter + xRangeTol);
    const auto _BaselineCorrectionHWS = p->GetBaseLineCorrectionHalfWindowSize();
    const auto _BaseLineCorrectionStrategy = p->GetBaselineCorrectionStrategy();
    const unsigned int offset_right = (mzs.size() - (subRes.first + subRes.second));
    const unsigned int offset_left = subRes.first;
    const unsigned int padding_left =
      (offset_left / _BaselineCorrectionHWS >= 1 ? _BaselineCorrectionHWS : offset_left) *
      (_BaseLineCorrectionStrategy != m2::BaselineCorrectionType::None);
    const unsigned int padding_right =
      (offset_right / _BaselineCorrectionHWS >= 1 ? _BaselineCorrectionHWS : offset_right) *
      (_BaseLineCorrectionStrategy != m2::BaselineCorrectionType::None);

    const unsigned int length = subRes.second + padding_left + padding_right;

    for (auto &source : p->GetSpectrumImageSourceList())
    {
      // map all spectra to several threads for processing
      const unsigned long n = source.m_Spectra.size();
      const unsigned t = p->GetNumberOfThreads();
      const auto _NormalizationStrategy = p->GetNormalizationStrategy();
      m2::Process::Map(n, t, [&](auto /*id*/, auto a, auto b) {
        std::ifstream f(source.m_BinaryDataPath, std::iostream::binary);
        std::vector<IntensityType> ints(length);
        std::vector<IntensityType> baseline(length);
        auto s = std::next(std::begin(ints), padding_left);
        auto e = std::prev(std::end(ints), padding_right);

        const auto Smoother =
          m2::Signal::CreateSmoother<IntensityType>(p->GetSmoothingStrategy(), p->GetSmoothingHalfWindowSize(), true);

        const auto BaselineSubstractor = m2::Signal::CreateSubstractBaselineConverter<IntensityType>(
          p->GetBaselineCorrectionStrategy(), p->GetBaseLineCorrectionHalfWindowSize());

        double d;
        const auto divide = [&d](const auto &v) { return v / d; };

        const auto &m_Spectra = source.m_Spectra;
        for (unsigned int i = a; i < b; ++i)
        {
          const auto &spectrum = m_Spectra[i];
          if (maskAccess && maskAccess->GetPixelByIndex(spectrum.index + source._offset) == 0)
          {
            imageAccess.SetPixelByIndex(spectrum.index + source._offset, 0);
            continue;
          }

          // ints container represents a extended region over which a accumulation is performed
          binaryDataToVector(
            f, source.m_Spectra[i].intOffset + (subRes.first - padding_left) * sizeof(IntensityType), length, ints);

          // if a normalization image exist, apply normalization before any further calculations are performed
          if (_NormalizationStrategy != NormalizationStrategyType::None)
          {
            d = normAccess.GetPixelByIndex(spectrum.index + source._offset);
            std::transform(std::begin(ints), std::end(ints), std::begin(ints), divide);
          }

          // smooth signal
          Smoother(ints);

          // substract basline
          BaselineSubstractor(ints);

          const auto val = Signal::RangePooling<IntensityType>(s, e, p->GetRangePoolingStrategy());
          imageAccess.SetPixelByIndex(spectrum.index + source._offset, val);
        }
      });
    }
  }
  else if (any(importMode & (m2::SpectrumFormatType::ContinuousCentroid | m2::SpectrumFormatType::ProcessedCentroid)))
  {
    for (auto &source : p->GetSpectrumImageSourceList())
    {
      // map all spectra to several threads for processing
      const unsigned long n = source.m_Spectra.size();
      const unsigned t = p->GetNumberOfThreads();
      const auto _NormalizationStrategy = p->GetNormalizationStrategy();
      m2::Process::Map(n, t, [&](auto /*id*/, auto a, auto b) {
        std::ifstream f(source.m_BinaryDataPath, std::iostream::binary);
        std::vector<IntensityType> ints;
        std::vector<MassAxisType> mzs;
        for (unsigned int i = a; i < b; ++i)
        {
          auto &spectrum = source.m_Spectra[i];
          if (maskAccess && maskAccess->GetPixelByIndex(spectrum.index + source._offset) == 0)
          {
            imageAccess.SetPixelByIndex(spectrum.index + source._offset, 0);
            continue;
          }
          binaryDataToVector(f, spectrum.mzOffset, spectrum.mzLength, mzs); // !! read mass axis for each spectrum
          auto subRes = m2::Signal::Subrange(mzs, xRangeCenter - xRangeTol, xRangeCenter + xRangeTol);
          if (subRes.second == 0)
          {
            imageAccess.SetPixelByIndex(spectrum.index + source._offset, 0);
            continue;
          }

          binaryDataToVector(f, spectrum.intOffset + subRes.first * sizeof(IntensityType), subRes.second, ints);
          if (_NormalizationStrategy != m2::NormalizationStrategyType::None)
          {
            IntensityType norm = normAccess.GetPixelByIndex(spectrum.index + source._offset);
            std::transform(std::begin(ints), std::end(ints), std::begin(ints), [&norm](auto &v) { return v / norm; });
          }

          auto val =
            Signal::RangePooling<IntensityType>(std::begin(ints), std::end(ints), p->GetRangePoolingStrategy());
          imageAccess.SetPixelByIndex(spectrum.index + source._offset, val);
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
    SetXInputType(m2::NumericType::Float);
    if (intensitiesDataTypeString.compare("32-bit float") == 0)
    {
      this->m_Processor.reset((m2::SpectrumImageBase::ProcessorBase *)new ImzMLProcessor<float, float>(this));
      SetYInputType(m2::NumericType::Float);
    }
    else if (intensitiesDataTypeString.compare("64-bit float") == 0)
    {
      SetYInputType(m2::NumericType::Double);
      this->m_Processor.reset((m2::SpectrumImageBase::ProcessorBase *)new ImzMLProcessor<float, double>(this));
    }
    else if (intensitiesDataTypeString.compare("32-bit integer") == 0)
    {
      this->m_Processor.reset((m2::SpectrumImageBase::ProcessorBase *)new ImzMLProcessor<float, long int>(this));
      // SetYInputType(m2::NumericType::Double);
    }
    else if (intensitiesDataTypeString.compare("64-bit integer") == 0)
    {
      this->m_Processor.reset((m2::SpectrumImageBase::ProcessorBase *)new ImzMLProcessor<float, long long int>(this));
      // SetYInputType(m2::NumericType::Double);
    }
  }
  else if (mzValueTypeString.compare("64-bit float") == 0)
  {
    SetXInputType(m2::NumericType::Double);
    if (intensitiesDataTypeString.compare("32-bit float") == 0)
    {
      SetYInputType(m2::NumericType::Float);
      this->m_Processor.reset((m2::SpectrumImageBase::ProcessorBase *)new ImzMLProcessor<double, float>(this));
    }
    else if (intensitiesDataTypeString.compare("64-bit float") == 0)
    {
      SetYInputType(m2::NumericType::Double);
      this->m_Processor.reset((m2::SpectrumImageBase::ProcessorBase *)new ImzMLProcessor<double, double>(this));
    }
    else if (intensitiesDataTypeString.compare("32-bit integer") == 0)
    {
      this->m_Processor.reset((m2::SpectrumImageBase::ProcessorBase *)new ImzMLProcessor<double, long int>(this));
      // SetYInputType(m2::NumericType::Double);
    }
    else if (intensitiesDataTypeString.compare("64-bit integer") == 0)
    {
      this->m_Processor.reset((m2::SpectrumImageBase::ProcessorBase *)new ImzMLProcessor<double, long long int>(this));
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
}

m2::ImzMLSpectrumImage::Pointer m2::ImzMLSpectrumImage::Combine(const m2::ImzMLSpectrumImage *A,
                                                                const m2::ImzMLSpectrumImage *B,
                                                                const char stackAxis)
{
  auto C = m2::ImzMLSpectrumImage::New();
  C->SetImportMode(A->GetImportMode());
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

  auto &C_sources = C->GetSpectrumImageSourceList();

  const auto &A_sources = A->GetSpectrumImageSourceList();
  const auto &B_sources = B->GetSpectrumImageSourceList();

  C_sources = A_sources;
  MITK_INFO << "Merge sources";

  std::transform(std::begin(B_sources), std::end(B_sources), std::back_inserter(C_sources), [&](Source s) {
    if (stackAxis == 'x')
      s._offset[0] += A_x; // shift along x-axis
    if (stackAxis == 'y')
      s._offset[1] += A_y; // shift along y-axis
    if (stackAxis == 'z')
      s._offset[2] += A_z; // shift along z-axis
    return s;
  });

  C->InitializeGeometry();
  std::set<m2::MassValue> set;
  set.insert(std::begin(A->GetPeaks()), std::end(A->GetPeaks()));
  set.insert(std::begin(B->GetPeaks()), std::end(B->GetPeaks()));

  C->GetPeaks().insert(std::end(C->GetPeaks()), std::begin(set), std::end(set));

  // for (auto &s : C->GetSpectrumImageSourceList())
  //{
  //  MITK_INFO << static_cast<unsigned>(s.ImportMode);
  //  MITK_INFO << s.m_BinaryDataPath;
  //  MITK_INFO << s.m_ImzMLDataPath;
  //  MITK_INFO << s._offset;
  //  MITK_INFO << s.m_Spectra.size();
  //}

  return C;
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::ImzMLProcessor<MassAxisType, IntensityType>::InitializeGeometry()
{
  auto &imageArtifacts = p->GetImageArtifacts();

  std::array<itk::SizeValueType, 3> imageSize = {p->GetPropertyValue<unsigned>("max count of pixel x"),
                                                 p->GetPropertyValue<unsigned>("max count of pixel y"),
                                                 p->GetPropertyValue<unsigned>("max count of pixel z")};

  std::array<double, 3> imageOrigin = {p->GetPropertyValue<double>("origin x") * 0.001,
                                       p->GetPropertyValue<double>("origin y") * 0.001,
                                       p->GetPropertyValue<double>("origin z") * 0.001};

  using ImageType = itk::Image<m2::DisplayImagePixelType, 3>;
  auto itkIonImage = ImageType::New();
  ImageType::IndexType idx;
  ImageType::SizeType size;

  idx.Fill(0);

  for (unsigned int i = 0; i < imageSize.size(); i++)
    size[i] = imageSize[i];
  ImageType::RegionType region(idx, size);
  itkIonImage->SetRegions(region);
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
void m2::ImzMLSpectrumImage::ImzMLProcessor<MassAxisType, IntensityType>::InitializeImageAccess()
{
  //////////---------------------------

  const auto importMode = p->GetImportMode();
  if (importMode == m2::SpectrumFormatType::ProcessedProfile)
  {
    mitkThrow() << R"(
		This ImzML file seems to contain profile spectra in a processed memory order. 
		This is not supported in M2aia! If there are really individual m/z axis for
		each spectrum, please resample the m/z axis and create one that is commonly
		used for all spectra. Save it as continuous ImzML!)";
    InitializeImageAccessProcessedProfile();
  }
  else if (importMode == m2::SpectrumFormatType::ContinuousProfile)
  {
    MITK_INFO << "Processed Profile";
    InitializeImageAccessContinuousProfile();
  }
  else if (importMode == m2::SpectrumFormatType::ProcessedCentroid)
  {
    MITK_INFO << "Processed Centroid";
    InitializeImageAccessProcessedCentroid();
  }
  else if (importMode == m2::SpectrumFormatType::ContinuousCentroid)
  {
    MITK_INFO << "Continuous Centroid";
    InitializeImageAccessContinuousCentroid();
  }

  // DEFAULT
  // INITIALIZE MASK, INDEX, NORMALIZATION IMAGES
  auto accMask = std::make_shared<mitk::ImagePixelWriteAccessor<mitk::LabelSetImage::PixelType, 3>>(p->GetMaskImage());
  auto accIndex = std::make_shared<mitk::ImagePixelWriteAccessor<m2::IndexImagePixelType, 3>>(p->GetIndexImage());
  auto accNorm = std::make_shared<mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3>>(p->GetNormalizationImage());
  for (const auto &source : p->GetSpectrumImageSourceList())
  {
    const auto &spectra = source.m_Spectra;
    m2::Process::Map(spectra.size(), p->GetNumberOfThreads(), [&](unsigned int /*t*/, unsigned int a, unsigned int b) {
      for (unsigned int i = a; i < b; i++)
      {
        const auto &spectrum = spectra[i];

        accIndex->SetPixelByIndex(spectrum.index + source._offset, i);

        // If mask content is generated elsewhere
        if (!p->GetUseExternalMask())
          accMask->SetPixelByIndex(spectrum.index + source._offset, 1);

        // If it is a processed file, normalization maps are set to 1 - assuming that spectra were already processed
        if (any(importMode & (m2::SpectrumFormatType::ProcessedCentroid | m2::SpectrumFormatType::ProcessedProfile)))
          accNorm->SetPixelByIndex(spectrum.index + source._offset, 1);
      }
    });
  }

  // reset prevention flags
  p->UseExternalMaskOff();
  p->UseExternalNormalizationOff();
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::ImzMLProcessor<MassAxisType, IntensityType>::InitializeImageAccessContinuousProfile()
{
  auto accNorm = std::make_shared<mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3>>(p->GetNormalizationImage());
  unsigned long long intsOffsetBytes = 0;

  const auto &source = p->GetSpectrumImageSourceList().front();
  const auto importMode = p->GetImportMode();
  std::vector<std::vector<double>> skylineT;
  std::vector<std::vector<double>> sumT;
  std::vector<MassAxisType> mzs;

  // load m/z axis
  {
    const auto &spectra = source.m_Spectra;
    std::ifstream f(source.m_BinaryDataPath, std::ios::binary);
    binaryDataToVector(f, spectra[0].mzOffset, spectra[0].mzLength, mzs);
    auto &massAxis = p->GetXAxis();
    massAxis.clear();
    std::copy(std::begin(mzs), std::end(mzs), std::back_inserter(massAxis));
    p->SetPropertyValue<unsigned>("spectral depth", mzs.size());
    p->SetPropertyValue<double>("x_min", mzs.front());
    p->SetPropertyValue<double>("x_max", mzs.back());
  }

  skylineT.resize(p->GetNumberOfThreads(), std::vector<double>(mzs.size(), 0));
  sumT.resize(p->GetNumberOfThreads(), std::vector<double>(mzs.size(), 0));
  // Functors and operators
  const auto Smoother =
    m2::Signal::CreateSmoother<IntensityType>(p->GetSmoothingStrategy(), p->GetSmoothingHalfWindowSize(), true);
  const auto BaselineSubstractor = m2::Signal::CreateSubstractBaselineConverter<IntensityType>(
    p->GetBaselineCorrectionStrategy(), p->GetBaseLineCorrectionHalfWindowSize());
  const auto Normalizator = m2::Signal::CreateNormalizor<MassAxisType, IntensityType>(p->GetNormalizationStrategy());
  const auto Maximum = [](const auto &a, const auto &b) { return a > b ? a : b; };
  const auto plus = std::plus<>();

  mitk::Timer t("Initialize image");
  for (const auto &source : p->GetSpectrumImageSourceList())
  {
    const auto &spectra = source.m_Spectra;

    m2::Process::Map(
      source.m_Spectra.size(), p->GetNumberOfThreads(), [&](unsigned int t, unsigned int a, unsigned int b) {
        std::vector<IntensityType> ints(mzs.size(), 0);
        std::vector<IntensityType> baseline(mzs.size(), 0);

        std::ifstream f(source.m_BinaryDataPath, std::ifstream::binary);

        for (unsigned long int i = a; i < b; i++)
        {
          const auto &spectrum = spectra[i];

          // Read data from file ------------
          binaryDataToVector(f, spectrum.intOffset + intsOffsetBytes, mzs.size(), ints);
          if (ints.front() == 0)
            ints[0] = ints[1];
          if (ints.back() == 0)
            ints.back() = *(ints.rbegin() + 1);

          // --------------------------------

          if (!p->GetUseExternalNormalization())
          {
            auto val = Normalizator(mzs, ints, accNorm->GetPixelByIndex(spectrum.index + source._offset));
            accNorm->SetPixelByIndex(spectrum.index + source._offset, val); // Set normalization image pixel value
          }
          else
          {
            // Normalization-image content was set elsewhere
            auto val = accNorm->GetPixelByIndex(spectrum.index + source._offset);
            std::transform(
              std::begin(ints), std::end(ints), std::begin(ints), [&val](const auto &a) { return a / val; });
          }

          Smoother(ints);
          BaselineSubstractor(ints);

          std::transform(std::begin(ints), std::end(ints), sumT.at(t).begin(), sumT.at(t).begin(), plus);
          std::transform(std::begin(ints), std::end(ints), skylineT.at(t).begin(), skylineT.at(t).begin(), Maximum);
        }
      });
  }

  auto &skyline = p->SkylineSpectrum();
  skyline.resize(mzs.size(), 0);
  for (unsigned int t = 0; t < p->GetNumberOfThreads(); ++t)
    std::transform(skylineT[t].begin(), skylineT[t].end(), skyline.begin(), skyline.begin(), [](auto &a, auto &b) {
      return a > b ? a : b;
    });

  auto &mean = p->MeanSpectrum();
  auto &sum = p->SumSpectrum();

  mean.resize(mzs.size(), 0);
  sum.resize(mzs.size(), 0);

  auto N = std::accumulate(std::begin(p->GetSpectrumImageSourceList()),
                           std::end(p->GetSpectrumImageSourceList()),
                           unsigned(0),
                           [](const auto &a, const auto &source) { return a + source.m_Spectra.size(); });

  for (unsigned int t = 0; t < p->GetNumberOfThreads(); ++t)
    std::transform(sumT[t].begin(), sumT[t].end(), sum.begin(), sum.begin(), [](auto &a, auto &b) { return a + b; });
  std::transform(sum.begin(), sum.end(), mean.begin(), [&](auto &a) { return a / double(N); });
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::ImzMLProcessor<MassAxisType, IntensityType>::InitializeImageAccessContinuousCentroid()
{
  auto accNorm = std::make_shared<mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3>>(p->GetNormalizationImage());
  unsigned long long intsOffsetBytes = 0;

  const auto &source = p->GetSpectrumImageSourceList().front();
  const auto importMode = p->GetImportMode();
  std::vector<std::vector<double>> skylineT;
  std::vector<std::vector<double>> sumT;
  std::vector<MassAxisType> mzs;

  // load m/z axis
  {
    const auto &spectra = source.m_Spectra;
    std::ifstream f(source.m_BinaryDataPath, std::ios::binary);
    binaryDataToVector(f, spectra[0].mzOffset, spectra[0].mzLength, mzs);
    auto &massAxis = p->GetXAxis();
    massAxis.clear();
    std::copy(std::begin(mzs), std::end(mzs), std::back_inserter(massAxis));
    p->SetPropertyValue<unsigned>("spectral depth", mzs.size());
    p->SetPropertyValue<double>("x_min", mzs.front());
    p->SetPropertyValue<double>("x_max", mzs.back());
  }

  skylineT.resize(p->GetNumberOfThreads(), std::vector<double>(mzs.size(), 0));
  sumT.resize(p->GetNumberOfThreads(), std::vector<double>(mzs.size(), 0));

  std::vector<std::vector<m2::MassValue>> peaksT(p->GetNumberOfThreads());

  for (const auto &source : p->GetSpectrumImageSourceList())
  {
    const auto &spectra = source.m_Spectra;

    m2::Process::Map(spectra.size(), p->GetNumberOfThreads(), [&](unsigned int t, unsigned int a, unsigned int b) {
      std::ifstream f;
      f.open(source.m_BinaryDataPath, std::ios::binary);

      std::vector<IntensityType> ints;

      peaksT[t].resize(mzs.size());
      auto pIt = std::begin(peaksT[t]);
      unsigned int index = 0;
      for (const auto &mz : mzs)
      {
        pIt->mass = mz;
        (pIt++)->massAxisIndex = index++;
      }

      auto iO = spectra[0].intOffset;
      auto iL = spectra[0].intLength;

      for (unsigned i = a; i < b; i++)
      {
        iO = spectra[i].intOffset;
        binaryDataToVector(f, iO, iL, ints);

        double val = 1;
        if (p->GetNormalizationStrategy() == m2::NormalizationStrategyType::InFile)
        {
          if (spectra[i].normalize != 0)
          {
            val = spectra[i].normalize;
            std::transform(std::begin(ints), std::end(ints), std::begin(ints), [&val](auto &v) { return v / val; });
          }
        }
        accNorm->SetPixelByIndex(spectra[i].index + source._offset, val); // Set normalization image pixel value

        auto intsIt = std::cbegin(ints);
        auto mzsIt = std::cbegin(mzs);
        auto pIt = std::begin(peaksT[t]);

        while (intsIt != std::cend(ints))
        {
          const auto &i = *intsIt;
          if (i > pIt->intensity)
          {
            pIt->intensity = i;
          }
          pIt->intensitySum += i;

          ++intsIt;
          ++mzsIt;
          ++pIt;
        }
      }

      f.close();
    });

    auto &mzAxis = p->GetXAxis();
    mzAxis.clear();
    std::copy(std::cbegin(mzs), std::cend(mzs), std::back_inserter(mzAxis));

    auto &skyline = p->SkylineSpectrum();
    auto &sum = p->SumSpectrum();
    auto &mean = p->MeanSpectrum();
    skyline.clear();
    sum.clear();
    mean.clear();

    sum.resize(mzs.size());
    mean.resize(mzs.size());
    skyline.resize(mzs.size());

    unsigned int numberOfSpectra = 0;
    for (const auto &s : p->GetSpectrumImageSourceList())
      numberOfSpectra += s.m_Spectra.size();

    for (auto &peaks : peaksT)
    {
      const auto tol = p->GetBinningTolerance();
      std::vector<m2::MassValue> binPeaks;
      m2::Signal::binPeaks(std::begin(peaks), std::end(peaks), std::back_inserter(binPeaks), tol * 10e-6);

      for (const auto &peak : binPeaks)
      {
        const auto &idx = peak.massAxisIndex;
        skyline[idx] = std::max(skyline[idx], peak.intensity);
      }
    }

    std::copy(std::begin(skyline), std::end(skyline), std::begin(sum));
    std::copy(std::begin(skyline), std::end(skyline), std::begin(mean));
    p->SetPropertyValue<double>("x_min", mzAxis.front());
    p->SetPropertyValue<double>("x_max", mzAxis.back());
  }
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::ImzMLProcessor<MassAxisType, IntensityType>::InitializeImageAccessProcessedCentroid()
{
  std::vector<std::list<m2::MassValue>> peaksT(p->GetNumberOfThreads());
  for (const auto &source : p->GetSpectrumImageSourceList())
  {
    const auto &spectra = source.m_Spectra;
    const auto &T = p->GetNumberOfThreads();
    const auto &binsN = p->GetNumberOfBins();
    std::vector<double> xMin(T, std::numeric_limits<double>::max()), xMax(T, 0);
    std::vector<std::vector<double>> xT(T, std::vector<double>(binsN, 0));
    std::vector<std::vector<double>> yT(T, std::vector<double>(binsN, 0));
    std::vector<std::vector<double>> yMaxT(T, std::vector<double>(binsN, 0));
    std::vector<std::vector<unsigned int>> hT(T, std::vector<unsigned int>(binsN, 0));

    auto barrier = itk::Barrier::New();
    barrier->Initialize(T);

    double max = 1;
    double min = 0;
    double binSize = 1;

    // MAP
    m2::Process::Map(spectra.size(), T, [&](unsigned int t, unsigned int a, unsigned int b) {
      std::ifstream f(source.m_BinaryDataPath, std::ios::binary);

      std::vector<MassAxisType> mzs;
      std::vector<IntensityType> ints;

      std::list<m2::MassValue> peaks, tempList;
	  // find x min/max
      for (unsigned i = a; i < b; i++)
      {
        const auto &mzO = spectra[i].mzOffset;
        const auto &mzL = spectra[i].mzLength;
        binaryDataToVector(f, mzO, mzL, mzs);
        xMin[t] = std::min(xMin[t], (double)mzs.front());
        xMax[t] = std::max(xMax[t], (double)mzs.back());
      }

      barrier->Wait();

      if (t == 0)
      {
		// find overall min/max
        max = *std::max_element(std::begin(xMax), std::end(xMax));
        min = *std::min_element(std::begin(xMin), std::end(xMin));
        binSize = (max - min) / double(binsN);
      }

      barrier->Wait();

      for (unsigned i = a; i < b; i++)
      {
        const auto &mzO = spectra[i].mzOffset;
        const auto &mzL = spectra[i].mzLength;
        binaryDataToVector(f, mzO, mzL, mzs);

        const auto &intO = spectra[i].intOffset;
        const auto &intL = spectra[i].intLength;
        binaryDataToVector(f, intO, intL, ints);

        for (unsigned int k = 0; k < mzs.size(); ++k)
        {
          auto j = (long)((mzs[k] - min) / binSize);

          if (j >= binsN)
            j = binsN - 1;
          else if (j < 0)
            j = 0;

          xT[t][j] += mzs[k];                                   // mass sum
          yT[t][j] += ints[k];                                  // intensitiy sum
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

    auto &mzAxis = p->GetXAxis();
    mzAxis.clear();
    auto &sum = p->SumSpectrum();
    sum.clear();
    auto &mean = p->MeanSpectrum();
    mean.clear();
    auto &skyline = p->SkylineSpectrum();
    skyline.clear();

    for (int k = 0; k < binsN; ++k)
    {
      if (hT[0][k] > 0)
      {
        mzAxis.push_back(xT[0][k] / (double)hT[0][k]);
        sum.push_back(yT[0][k]);
        mean.push_back(yT[0][k] / (double)hT[0][k]);
        skyline.push_back(yMaxT[0][k]);
        std::cout << mzAxis.back() << " " << hT[0][k] << ", ";
      }
    }

    p->SetPropertyValue<double>("x_min", mzAxis.front());
    p->SetPropertyValue<double>("x_max", mzAxis.back());
  }
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::ImzMLProcessor<MassAxisType, IntensityType>::GrabIntensityPrivate(
  unsigned long int index, std::vector<double> &ints, unsigned int sourceIndex) const
{
  mitk::ImagePixelReadAccessor<m2::NormImagePixelType, 3> normAcc(p->GetNormalizationImage());

  const auto &source = p->GetSpectrumImageSourceList()[sourceIndex];
  const auto &spectrum = source.m_Spectra[index];
  const auto &intso = spectrum.intOffset;
  const auto &intsl = spectrum.intLength;
  const auto Smoother =
    m2::Signal::CreateSmoother<IntensityType>(p->GetSmoothingStrategy(), p->GetSmoothingHalfWindowSize(), true);

  const auto BaslineSubstractor = m2::Signal::CreateSubstractBaselineConverter<IntensityType>(
    p->GetBaselineCorrectionStrategy(), p->GetBaseLineCorrectionHalfWindowSize());
  const auto importMode = p->GetImportMode();

  std::ifstream f;
  f.open(source.m_BinaryDataPath, std::ios::binary);

  std::vector<IntensityType> ints_get, baseline;
  binaryDataToVector(f, intso, intsl, ints_get);
  baseline.resize(intsl, 0);
  if (any(importMode & (m2::SpectrumFormatType::ContinuousProfile)))
  {
    if (p->GetNormalizationStrategy() != m2::NormalizationStrategyType::None)
    {
      const auto normalization = normAcc.GetPixelByIndex(spectrum.index + source._offset);
      std::transform(ints_get.begin(), ints_get.end(), ints_get.begin(), [&normalization](auto &val) {
        return val / normalization;
      });
    }

    Smoother(ints_get);
    BaslineSubstractor(ints_get);
  }

  ints.clear();
  std::copy(std::begin(ints_get), std::end(ints_get), std::back_inserter(ints));
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImage::ImzMLProcessor<MassAxisType, IntensityType>::GrabMassPrivate(
  unsigned long int index, std::vector<double> &mzs, unsigned int sourceIndex) const
{
  const auto &source = p->GetSpectrumImageSourceList()[sourceIndex];
  const auto &spectrum = source.m_Spectra[index];
  const auto &mzo = spectrum.mzOffset;
  const auto &mzl = spectrum.mzLength;

  std::ifstream f;
  f.open(source.m_BinaryDataPath, std::ios::binary);

  std::vector<MassAxisType> mzs_get;
  binaryDataToVector(f, mzo, mzl, mzs_get);
  mzs.clear();

  std::copy(std::begin(mzs_get), std::end(mzs_get), std::back_inserter(mzs));
}

m2::ImzMLSpectrumImage::~ImzMLSpectrumImage()
{
  MITK_INFO << GetStaticNameOfClass() << " destroyed!";
}

m2::ImzMLSpectrumImage::ImzMLSpectrumImage() : m2::SpectrumImageBase()
{
  MITK_INFO << GetStaticNameOfClass() << " created!";
  this->SetPropertyValue<std::string>("x_label", "m/z");
  this->SetExportMode(m2::SpectrumFormatType::ContinuousProfile);
}
