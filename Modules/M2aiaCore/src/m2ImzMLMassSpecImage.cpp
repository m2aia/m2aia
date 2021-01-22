/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <boost/range/combine.hpp>
#include <forward_list>
#include <future>
#include <itkImageDuplicator.h>
#include <itkLog10ImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <m2Calibration.hpp>
#include <m2ImzMLMassSpecImage.h>
#include <m2Morphology.hpp>
#include <m2NoiseEstimators.hpp>
#include <m2PeakDetection.hpp>
#include <m2Process.hpp>
#include <m2RunningMedian.hpp>
#include <m2Smoothing.hpp>
#include <mitkImage2DToImage3DSliceFilter.h>
#include <mitkImage3DSliceToImage2DFilter.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkLookupTableProperty.h>
#include <mitkProperties.h>
#include <mitkTimer.h>
#include <mitkWeakPointerProperty.h>

void m2::ImzMLMassSpecImage::GrabIonImage(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const
{
  m2::MSImageBase::GrabIonImage(mz, tol, mask, img);
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLMassSpecImage::ImzMLProcessor<MassAxisType, IntensityType>::GrabIonImagePrivate(
  double mz, double tol, const mitk::Image *mask, mitk::Image *destImage) const
{
  AccessByItk(destImage, [](auto itkImg) { itkImg->FillBuffer(0); });
  using namespace m2;
  // accessors
  mitk::ImagePixelWriteAccessor<IonImagePixelType, 3> imageAccess(destImage);
  mitk::ImagePixelReadAccessor<NormImagePixelType, 3> normAccess(p->GetNormalizationImage());
  std::shared_ptr<mitk::ImagePixelReadAccessor<MaskImagePixelType, 3>> maskAccess;

  const auto _BaselineCorrectionHWS = p->m_BaseLinecorrectionHalfWindowSize;
  const auto _NormalizationStrategy = p->GetNormalizationStrategy();
  auto _SmoothingStrategy = p->GetSmoothingStrategy();
  const auto _BaseLineCorrectionStrategy = p->GetBaselineCorrectionStrategy();
  const auto _IonImageGrabStrategy = p->GetIonImageGrabStrategy();

  if (mask)
  {
    maskAccess.reset(new mitk::ImagePixelReadAccessor<MaskImagePixelType, 3>(mask));
    MITK_INFO << "> Use mask image";
  }
  p->SetProperty("mz", mitk::DoubleProperty::New(mz));
  p->SetProperty("tol", mitk::DoubleProperty::New(tol));
  auto mdMz = itk::MetaDataObject<double>::New();
  mdMz->SetMetaDataObjectValue(mz);
  auto mdTol = itk::MetaDataObject<double>::New();
  mdTol->SetMetaDataObjectValue(tol);
  p->GetMetaDataDictionary()["mz"] = mdMz;
  p->GetMetaDataDictionary()["tol"] = mdTol;

  std::vector<MassAxisType> mzs;
  std::vector<double> kernel;

  if (_SmoothingStrategy == m2::SmoothingType::SavitzkyGolay)
    kernel = m2::Smoothing::savitzkyGolayKernel(p->m_SmoothingHalfWindowSize, 3);

  // if (_SmoothingStrategy == m2::SmoothingType::Gaussian)
  //  kernel = m2::Smoothing::savitzkyGolayKernel(p->m_SmoothingHalfWindowSize, 3);

  const auto accumulate = [&_IonImageGrabStrategy](auto s, auto e) {
    // Grab the value
    // IonImageGrabStrategy
    double val = 0;
    switch (_IonImageGrabStrategy)
    {
      case IonImageGrabStrategyType::None:
        break;
      case IonImageGrabStrategyType::Sum:
        val = std::accumulate(s, e, IntensityType(0));
        break;
      case IonImageGrabStrategyType::Mean:
        val = std::accumulate(s, e, IntensityType(0)) / IntensityType(std::distance(s, e));
        break;
      case IonImageGrabStrategyType::Maximum:
        val = *std::max_element(s, e);
        break;
      case IonImageGrabStrategyType::Median:
      {
        const unsigned int _N = std::distance(s, e);
        double median = 0;
        if ((_N % 2) == 0)
        {
          std::nth_element(s, s + _N * 0.5, e);
          std::nth_element(s, s + _N * 0.5 + 1, e);
          median = 0.5 * (*std::next(s, _N * 0.5) + *std::next(s, _N * 0.5 + 1));
        }
        else
        {
          std::nth_element(s, s + ((_N + 1) * 0.5), e);
          median = 0.5 * (*std::next(s, _N * 0.5) + *std::next(s, _N * 0.5 + 1));
        }
        val = median;
        break;
      }
    }

    return val;
  };

  // Profile (continuous) spectrum
  const auto &source = p->GetSpectraSource();
  if (any(source.ImportMode & ImzMLFormatType::ContinuousProfile))
  {
    { // open binary file stream and read mzs once
      std::ifstream f(source._BinaryDataPath, std::iostream::binary);
      binaryDataToVector(f, source._Spectra[0].mzOffset, source._Spectra[0].mzLength, mzs);
      f.close();
      MITK_INFO << "Init mzAxis: " << mzs.front() << " " << mzs.back();
    }
    // mz subrange
    auto subRes = m2::Peaks::Subrange(mzs, mz - tol, mz + tol);

    const unsigned int offset_right = (mzs.size() - (subRes.first + subRes.second));
    const unsigned int offset_left = subRes.first;
    const unsigned int padding_left =
      (offset_left / _BaselineCorrectionHWS >= 1 ? _BaselineCorrectionHWS : offset_left) *
      (_BaseLineCorrectionStrategy != m2::BaselineCorrectionType::None);
    const unsigned int padding_right =
      (offset_right / _BaselineCorrectionHWS >= 1 ? _BaselineCorrectionHWS : offset_right) *
      (_BaseLineCorrectionStrategy != m2::BaselineCorrectionType::None);

    const unsigned int length = subRes.second + padding_left + padding_right;
    if (length < kernel.size())
    {
      MITK_WARN << "Smoothing fails: intensity range size is smaller than kernel size " << length << " "
                << kernel.size();
      _SmoothingStrategy = m2::SmoothingType::None;
    }

    for (auto &source : p->GetSourceList())
    {
      // map all spectra to several threads for processing
      const unsigned long n = source._Spectra.size();
      const unsigned t = p->GetNumberOfThreads();

      m2::Process::Map(n, t, [&](auto /*id*/, auto a, auto b) {
        std::ifstream f(source._BinaryDataPath, std::iostream::binary);
        std::vector<IntensityType> ints(length);
        std::vector<IntensityType> baseline(length);
        auto s = std::next(std::begin(ints), padding_left);
        auto e = std::prev(std::end(ints), padding_right);

        const auto &_Spectra = source._Spectra;
        for (unsigned int i = a; i < b; ++i)
        {
          const auto &spectrum = _Spectra[i];
          if (maskAccess && maskAccess->GetPixelByIndex(spectrum.index + source._offset) == 0)
          {
            imageAccess.SetPixelByIndex(spectrum.index + source._offset, 0);
            continue;
          }

          // ints container represents a extended region over which a accumulation is performed
          binaryDataToVector(
            f, source._Spectra[i].intOffset + (subRes.first - padding_left) * sizeof(IntensityType), length, ints);

          // if a normalization image exist, apply normalization before any further calculations are performed
          if (_NormalizationStrategy != m2::NormalizationStrategyType::None)
          {
            IntensityType norm = normAccess.GetPixelByIndex(spectrum.index + source._offset);
            std::transform(std::begin(ints), std::end(ints), std::begin(ints), [&norm](auto &v) { return v / norm; });
          }

          switch (_SmoothingStrategy)
          {
            case m2::SmoothingType::SavitzkyGolay:
              m2::Smoothing::filter(ints, kernel, false);
              break;
            case m2::SmoothingType::Gaussian:
              break;
            default:
              break;
          }

          switch (_BaseLineCorrectionStrategy)
          {
            case m2::BaselineCorrectionType::TopHat:
              m2::Morphology::erosion(ints, _BaselineCorrectionHWS, baseline);
              m2::Morphology::dilation(baseline, _BaselineCorrectionHWS, baseline);
              std::transform(ints.begin(), ints.end(), baseline.begin(), ints.begin(), std::minus<>());
              break;
            case m2::BaselineCorrectionType::Median:
              m2::RunMedian::apply(ints, _BaselineCorrectionHWS, baseline);
              std::transform(ints.begin(), ints.end(), baseline.begin(), ints.begin(), std::minus<>());
              break;
            default:
              break;
          }

          auto val = accumulate(s, e);

          imageAccess.SetPixelByIndex(spectrum.index + source._offset, val);
        }
      });
    }
  }
  else if (any(source.ImportMode & (m2::ImzMLFormatType::ContinuousCentroid | m2::ImzMLFormatType::ProcessedCentroid)))
  {
    for (auto &source : p->GetSourceList())
    {
      // map all spectra to several threads for processing
      const unsigned long n = source._Spectra.size();
      const unsigned t = p->GetNumberOfThreads();
      m2::Process::Map(n, t, [&](auto /*id*/, auto a, auto b) {
        std::ifstream f(source._BinaryDataPath, std::iostream::binary);
        std::vector<IntensityType> ints;
        std::vector<MassAxisType> mzs;
        for (unsigned int i = a; i < b; ++i)
        {
          auto &spectrum = source._Spectra[i];
          if (maskAccess && maskAccess->GetPixelByIndex(spectrum.index + source._offset) == 0)
          {
            imageAccess.SetPixelByIndex(spectrum.index + source._offset, 0);
            continue;
          }
          binaryDataToVector(f, spectrum.mzOffset, spectrum.mzLength, mzs); // !! read mass axis for each spectrum
          auto subRes = m2::Peaks::Subrange(mzs, mz - tol, mz + tol);
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

          double val = accumulate(std::begin(ints), std::end(ints));
          imageAccess.SetPixelByIndex(spectrum.index + source._offset, val);
        }
      });
    }
  }
}

void m2::ImzMLMassSpecImage::InitializeProcessor()
{
  auto intensitiesDataTypeString = GetPropertyValue<std::string>("intensity array value type");
  auto mzValueTypeString = GetPropertyValue<std::string>("m/z array value type");
  if (mzValueTypeString.compare("32-bit float") == 0)
  {
    SetMzsInputType(m2::NumericType::Float);
    if (intensitiesDataTypeString.compare("32-bit float") == 0)
    {
      this->m_Processor.reset((m2::MSImageBase::ProcessorBase *)new ImzMLProcessor<float, float>(this));
      SetIntsInputType(m2::NumericType::Float);
    }
    else if (intensitiesDataTypeString.compare("64-bit float") == 0)
    {
      SetIntsInputType(m2::NumericType::Double);
      this->m_Processor.reset((m2::MSImageBase::ProcessorBase *)new ImzMLProcessor<float, double>(this));
    }
    else if (intensitiesDataTypeString.compare("32-bit integer") == 0)
    {
      this->m_Processor.reset((m2::MSImageBase::ProcessorBase *)new ImzMLProcessor<float, long int>(this));
      //SetIntsInputType(m2::NumericType::Double);
    }
    else if (intensitiesDataTypeString.compare("64-bit integer") == 0)
    {
      this->m_Processor.reset((m2::MSImageBase::ProcessorBase *)new ImzMLProcessor<float, long long int>(this));
      // SetIntsInputType(m2::NumericType::Double);
    }
  }
  else if (mzValueTypeString.compare("64-bit float") == 0)
  {
    SetMzsInputType(m2::NumericType::Double);
    if (intensitiesDataTypeString.compare("32-bit float") == 0)
    {
      SetIntsInputType(m2::NumericType::Float);
      this->m_Processor.reset((m2::MSImageBase::ProcessorBase *)new ImzMLProcessor<double, float>(this));
    }
    else if (intensitiesDataTypeString.compare("64-bit float") == 0)
    {
      SetIntsInputType(m2::NumericType::Double);
      this->m_Processor.reset((m2::MSImageBase::ProcessorBase *)new ImzMLProcessor<double, double>(this));
    }
    else if (intensitiesDataTypeString.compare("32-bit integer") == 0)
    {
      this->m_Processor.reset((m2::MSImageBase::ProcessorBase *)new ImzMLProcessor<double, long int>(this));
      // SetIntsInputType(m2::NumericType::Double);
    }
    else if (intensitiesDataTypeString.compare("64-bit integer") == 0)
    {
      this->m_Processor.reset((m2::MSImageBase::ProcessorBase *)new ImzMLProcessor<double, long long int>(this));
      // SetIntsInputType(m2::NumericType::Double);
    }
  }
}

void m2::ImzMLMassSpecImage::InitializeGeometry()
{
  if (!m_Processor)
    this->InitializeProcessor();
  this->m_Processor->InitializeGeometry();
  this->SetImageGeometryInitialized(true);
}

void m2::ImzMLMassSpecImage::InitializeImageAccess()
{
  this->m_Processor->InitializeImageAccess();
  this->SetImageAccessInitialized(true);
}

m2::ImzMLMassSpecImage::Pointer m2::ImzMLMassSpecImage::Combine(const m2::ImzMLMassSpecImage *A,
                                                                const m2::ImzMLMassSpecImage *B,
                                                                const char stackAxis)
{
  auto C = m2::ImzMLMassSpecImage::New();

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
    auto A_mass = A->MassAxis();
    auto B_mass = B->MassAxis();
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

  auto &C_sources = C->GetSourceList();

  const auto &A_sources = A->GetSourceList();
  const auto &B_sources = B->GetSourceList();

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

  // for (auto &s : C->GetSourceList())
  //{
  //  MITK_INFO << static_cast<unsigned>(s.ImportMode);
  //  MITK_INFO << s._BinaryDataPath;
  //  MITK_INFO << s._ImzMLDataPath;
  //  MITK_INFO << s._offset;
  //  MITK_INFO << s._Spectra.size();
  //}

  return C;
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLMassSpecImage::ImzMLProcessor<MassAxisType, IntensityType>::InitializeGeometry()
{
  auto &imageArtifacts = p->GetImageArtifacts();

  std::array<itk::SizeValueType, 3> imageSize = {p->GetPropertyValue<unsigned>("max count of pixel x"),
                                                 p->GetPropertyValue<unsigned>("max count of pixel y"),
                                                 p->GetPropertyValue<unsigned>("max count of pixel z")};

  std::array<double, 3> imageOrigin = {p->GetPropertyValue<double>("origin x") * 0.001,
                                       p->GetPropertyValue<double>("origin y") * 0.001,
                                       p->GetPropertyValue<double>("origin z") * 0.001};

  using ImageType = itk::Image<m2::IonImagePixelType, 3>;
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
    using LocalImageType = itk::Image<m2::IonImagePixelType, 3>;
    auto caster = itk::CastImageFilter<ImageType, LocalImageType>::New();
    caster->SetInput(itkIonImage);
    caster->Update();
    p->InitializeByItk(caster->GetOutput());

    mitk::ImagePixelWriteAccessor<m2::IonImagePixelType, 3> acc(p);
    std::memset(acc.GetData(), 0, imageSize[0] * imageSize[1] * imageSize[2] * sizeof(m2::IonImagePixelType));
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
    using LocalImageType = itk::Image<m2::MaskImagePixelType, 3>;
    auto caster = itk::CastImageFilter<ImageType, LocalImageType>::New();
    caster->SetInput(itkIonImage);
    caster->Update();
    auto maskImage = mitk::Image::New();
    imageArtifacts["mask"] = maskImage;
    maskImage->InitializeByItk(caster->GetOutput());

    mitk::ImagePixelWriteAccessor<m2::MaskImagePixelType, 3> acc(maskImage);
    std::memset(acc.GetData(), 0, imageSize[0] * imageSize[1] * imageSize[2] * sizeof(m2::MaskImagePixelType));
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

  mitk::ImagePixelWriteAccessor<m2::IonImagePixelType, 3> acc(p);
  auto max_dim0 = p->GetDimensions()[0];
  auto max_dim1 = p->GetDimensions()[1];
  acc.SetPixelByIndex({0, 0, 0}, 1);
  acc.SetPixelByIndex({0, max_dim1 - 1, 0}, max_dim1 / 2);
  acc.SetPixelByIndex({max_dim0 - 1, 0, 0}, max_dim0 / 2);
  acc.SetPixelByIndex({max_dim0 - 1, max_dim1 - 1, 0}, max_dim1 + max_dim0);
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLMassSpecImage::ImzMLProcessor<MassAxisType, IntensityType>::InitializeImageAccess()
{
  using namespace m2;

  auto accMask = std::make_shared<mitk::ImagePixelWriteAccessor<m2::MaskImagePixelType, 3>>(p->GetMaskImage());
  auto accIndex = std::make_shared<mitk::ImagePixelWriteAccessor<m2::IndexImagePixelType, 3>>(p->GetIndexImage());
  auto accNorm = std::make_shared<mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3>>(p->GetNormalizationImage());

  std::vector<MassAxisType> mzs;

  // ----- PreProcess -----

  // if the data are available as continuous data with equivalent mz axis for all
  // spectra, we can calculate the skyline, sum and mean spectrum over the image
  std::vector<std::vector<double>> skylineT;
  std::vector<std::vector<double>> sumT;

  // shortcuts
  const auto _NormalizationStrategy = p->GetNormalizationStrategy();
  const auto _SmoothingStrategy = p->GetSmoothingStrategy();
  const auto _BaslineCorrectionStrategy = p->GetBaselineCorrectionStrategy();
  const auto _BaselineCorrectionHalfWindowSize = p->m_BaseLinecorrectionHalfWindowSize;
  const bool _PreventInitMask = p->GetPreventMaskImageInitialization();
  const bool _PreventInitNorm = p->GetPreventNormalizationImageInitialization();

  // smoothing kernal is initialized once
  std::vector<double> kernel;
  if (_SmoothingStrategy != m2::SmoothingType::None)
    kernel = m2::Smoothing::savitzkyGolayKernel(p->m_SmoothingHalfWindowSize, 3);

  MITK_INFO << "Smoothing: " << (int)_SmoothingStrategy << " " << p->m_SmoothingHalfWindowSize;
  MITK_INFO << "Baseline Correction: " << (int)_BaslineCorrectionStrategy << " "
            << p->m_BaseLinecorrectionHalfWindowSize;
  bool _UseSubRange = false;
  unsigned long lenght;
  unsigned long long intsOffsetBytes = 0;

  const auto &source = p->GetSourceList().front();

  if (any(source.ImportMode & (m2::ImzMLFormatType::ContinuousProfile | m2::ImzMLFormatType::ContinuousCentroid)))
  {
    // shortcuts
    const auto &spectra = source._Spectra;
    std::ifstream f(source._BinaryDataPath, std::ios::binary);

    // load m/z axis
    binaryDataToVector(f, spectra[0].mzOffset, spectra[0].mzLength, mzs);

    auto &massAxis = p->MassAxis();
    massAxis.clear();
    std::copy(std::begin(mzs), std::end(mzs), std::back_inserter(massAxis));
    p->SetPropertyValue<unsigned>("spectral depth", mzs.size());
    p->SetPropertyValue<double>("min m/z", mzs.front());
    p->SetPropertyValue<double>("max m/z", mzs.back());
  }

  if (any(source.ImportMode & (m2::ImzMLFormatType::ContinuousProfile)))
  {
    // shortcuts
    const auto &spectra = source._Spectra;
    std::ifstream f(source._BinaryDataPath, std::ios::binary);

    // clean up the bounds
    if ((p->GetLowerMZBound() >= 0 || p->GetUpperMZBound() > 0) && (p->GetLowerMZBound() < p->GetUpperMZBound()))
      _UseSubRange = true;
    else
    {
      p->SetLowerMZBound(mzs.front());
      p->SetUpperMZBound(mzs.back());
      _UseSubRange = false;
    }

    if (_UseSubRange)
    {
      auto subRes = m2::Peaks::Subrange(mzs, p->m_LowerMZBound, p->m_UpperMZBound);
      intsOffsetBytes = subRes.first * sizeof(IntensityType);
      lenght = subRes.second;

      unsigned long long mzOffsetBytes = subRes.first * sizeof(MassAxisType);
      binaryDataToVector(f, spectra[0].mzOffset + mzOffsetBytes, lenght, mzs);
    }
    else
    {
      lenght = spectra[0].mzLength;
    }

    skylineT.resize(p->GetNumberOfThreads(), std::vector<double>(mzs.size(), 0));
    sumT.resize(p->GetNumberOfThreads(), std::vector<double>(mzs.size(), 0));
  }

  //////////---------------------------
  // profile spectra have to be continuous
  if (any(source.ImportMode & (m2::ImzMLFormatType::ProcessedProfile)))
  {
    mitkThrow() << R"(
		This ImzML file seems to contain profile spectra in a processed memory order. 
		This is not supported in M2aia! If there are really individual m/z axis for
		each spectrum, please resample the m/z axis and create one that is commonly
		used for all spectra. Save it as continuous ImzML!)";
  }
  MITK_INFO << "Initialize imzML common info:";
  MITK_INFO << "\tNormalizationStrategyType [" << static_cast<unsigned>(_NormalizationStrategy) << "]";
  if (any(source.ImportMode & (m2::ImzMLFormatType::ContinuousProfile)))
  {
    mitk::Timer t("Initialize image");
    for (const auto &source : p->GetSourceList())
    {
      MITK_INFO << "Initialize imzML source info:";
      MITK_INFO << "\tImportMode [" << static_cast<unsigned>(source.ImportMode) << "]";
      MITK_INFO << "\tBinaryDataPath [" << source._BinaryDataPath << "]";
      MITK_INFO << "\tImzMLDataPath [" << source._ImzMLDataPath << "]";
      MITK_INFO << "\t#Spectra [" << source._Spectra.size() << "]";
      MITK_INFO << "\tOffset [" << source._offset << "]";

      m2::Process::Map(
        source._Spectra.size(), p->GetNumberOfThreads(), [&](unsigned int t, unsigned int a, unsigned int b) {
          std::vector<IntensityType> ints(mzs.size(), 0);
          std::vector<IntensityType> baseline(mzs.size(), 0);

          const auto &spectra = source._Spectra;

          std::ifstream f(source._BinaryDataPath, std::ifstream::binary);

          IntensityType val = 1;
          const auto divides = [&val](const auto &a) { return a / val; };
          const auto maximum = [](const auto &a, const auto &b) { return a > b ? a : b; };
          const auto plus = std::plus<>();

          for (unsigned long int i = a; i < b; i++)
          {
            const auto &spectrum = spectra[i];

            // Read data from file ------------
            binaryDataToVector(f, spectrum.intOffset + intsOffsetBytes, lenght, ints);
            if (ints.front() == 0)
              ints[0] = ints[1];
            if (ints.back() == 0)
              ints.back() = *(ints.rbegin() + 1);

            // --------------------------------
            if (_NormalizationStrategy != NormalizationStrategyType::None)
            { // update only if any index accessing is required

              const auto n = ints.size();
              if (!_PreventInitNorm)
              {
                switch (_NormalizationStrategy)
                {
                  case NormalizationStrategyType::InFile:
                    val = spectrum.normalize;
                    std::transform(std::begin(ints), std::end(ints), std::begin(ints), divides);
                    break;
                  case NormalizationStrategyType::TIC:
                    val = m2::Calibration::TotalIonCurrent(std::begin(mzs), std::end(mzs), std::begin(ints));
                    std::transform(std::begin(ints), std::end(ints), std::begin(ints), divides);
                    break;
                  case NormalizationStrategyType::Sum:
                    val = std::accumulate(std::begin(ints), std::end(ints), (long double)(0));
                    std::transform(std::begin(ints), std::end(ints), std::begin(ints), divides);
                    break;

                  case NormalizationStrategyType::Median:

                    val = 1;
                    if ((n % 2) == 0)
                    {
                      std::nth_element(std::begin(ints), std::begin(ints) + n * 0.5, std::end(ints));
                      std::nth_element(std::begin(ints), std::begin(ints) + n * 0.5 + 1, std::end(ints));
                      val = 0.5 * (*std::next(std::begin(ints), n * 0.5) + *std::next(std::begin(ints), n * 0.5 + 1));
                    }
                    else
                    {
                      std::nth_element(std::begin(ints), std::begin(ints) + ((n + 1) * 0.5), std::end(ints));
                      val = 0.5 * (*std::next(std::begin(ints), n * 0.5) + *std::next(std::begin(ints), n * 0.5 + 1));
                    }

                    std::transform(std::begin(ints), std::end(ints), std::begin(ints), divides);
                    break;

                  case NormalizationStrategyType::None:
                    break;
                }
                accNorm->SetPixelByIndex(spectrum.index + source._offset, val); // Set normalization image pixel value
              }
              else
              {
                // Normalization-image content was set elsewhere
                val = accNorm->GetPixelByIndex(spectrum.index + source._offset);
                std::transform(std::begin(ints), std::end(ints), std::begin(ints), divides);
              }
            }

            if (_SmoothingStrategy != SmoothingType::None)
            {
              switch (_SmoothingStrategy)
              {
                case SmoothingType::SavitzkyGolay:
                  m2::Smoothing::filter(ints, kernel);
                  break;
                default:
                  break;
              }
            }

            if (_BaslineCorrectionStrategy != BaselineCorrectionType::None)
            {
              switch (_BaslineCorrectionStrategy)
              {
                case BaselineCorrectionType::TopHat:
                  m2::Morphology::erosion(ints, _BaselineCorrectionHalfWindowSize, baseline);
                  m2::Morphology::dilation(baseline, _BaselineCorrectionHalfWindowSize, baseline);
                  break;
                case BaselineCorrectionType::Median:
                  m2::RunMedian::apply(ints, _BaselineCorrectionHalfWindowSize, baseline);
                  break;
                default:
                  break;
              }

              std::transform(std::begin(ints), std::end(ints), baseline.begin(), std::begin(ints), std::minus<>());
            }

            std::transform(std::begin(ints), std::end(ints), sumT.at(t).begin(), sumT.at(t).begin(), plus);
            std::transform(std::begin(ints), std::end(ints), skylineT.at(t).begin(), skylineT.at(t).begin(), maximum);
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

    auto N = std::accumulate(
      std::begin(p->GetSourceList()), std::end(p->GetSourceList()), unsigned(0), [](const auto &a, const auto &source) {
        return a + source._Spectra.size();
      });

    for (unsigned int t = 0; t < p->GetNumberOfThreads(); ++t)
      std::transform(sumT[t].begin(), sumT[t].end(), sum.begin(), sum.begin(), [](auto &a, auto &b) { return a + b; });
    std::transform(sum.begin(), sum.end(), mean.begin(), [&](auto &a) { return a / double(N); });
  }
  //////////---------------------------
  else if (any(source.ImportMode & (m2::ImzMLFormatType::ProcessedCentroid)))
  {
    std::vector<std::list<m2::MassValue>> peaksT(p->GetNumberOfThreads());
    for (const auto &source : p->GetSourceList())
    {
      const auto &spectra = source._Spectra;

      m2::Process::Map(spectra.size(), p->GetNumberOfThreads(), [&](unsigned int t, unsigned int a, unsigned int b) {
        std::ifstream f;
        f.open(source._BinaryDataPath, std::ios::binary);

        std::vector<MassAxisType> mzs;
        std::vector<IntensityType> ints;

        auto iO = spectra[0].intOffset;
        auto iL = spectra[0].intLength;

        std::list<m2::MassValue> peaks, tempList;
        for (unsigned i = a; i < b; i++)
        {
          iL = spectra[i].intLength;
          auto mzO = spectra[i].mzOffset;
          auto mzL = spectra[i].mzLength;
          binaryDataToVector(f, mzO, mzL, mzs);

          iO = spectra[i].intOffset;
          binaryDataToVector(f, iO, iL, ints);
          unsigned int l = 0;

          std::transform(
            std::cbegin(mzs), std::cend(mzs), std::cbegin(ints), std::back_inserter(peaks), [&l](auto &a, auto &b) {
              return m2::MassValue{(double)a, (double)b, 1, l++};
            });

          peaksT.at(t).merge(peaks);
          tempList = std::move(peaksT.at(t));

          peaksT.at(t).clear();
          m2::Peaks::binPeaks(std::begin(tempList),
                              std::end(tempList),
                              std::back_inserter(peaksT.at(t)),
                              p->GetPeakPickingBinningTolerance() * 10e-6);
        }

        f.close();
      });
      std::list<m2::MassValue> mergeList;
      for (auto &&list : peaksT)
        mergeList.merge(list);

      peaksT.clear();

      std::list<m2::MassValue> finalPeaks;
      m2::Peaks::binPeaks(std::begin(mergeList),
                          std::end(mergeList),
                          std::back_inserter(finalPeaks),
                          p->GetPeakPickingBinningTolerance() * 10e-6);

      auto &mzAxis = p->MassAxis();
      auto &skyline = p->SkylineSpectrum();
      mzAxis.clear();
      skyline.clear();

      for (auto &p : finalPeaks)
      {
        mzAxis.emplace_back(p.mass);
        skyline.emplace_back(p.intensity);
      }

      p->SetPropertyValue<double>("min m/z", mzAxis.front());
      p->SetPropertyValue<double>("max m/z", mzAxis.back());
    }
  }

  else if (any(source.ImportMode & (m2::ImzMLFormatType::ContinuousCentroid)))
  {
    std::vector<std::vector<m2::MassValue>> peaksT(p->GetNumberOfThreads());

    for (const auto &source : p->GetSourceList())
    {
      const auto &spectra = source._Spectra;

      m2::Process::Map(spectra.size(), p->GetNumberOfThreads(), [&](unsigned int t, unsigned int a, unsigned int b) {
        std::ifstream f;
        f.open(source._BinaryDataPath, std::ios::binary);

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
          if (spectra[i].normalize != 0)
          {
            val = spectra[i].normalize;
            std::transform(std::begin(ints), std::end(ints), std::begin(ints), [&val](auto &v) { return v / val; });
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

      auto &mzAxis = p->MassAxis();
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
      for (const auto &s : p->GetSourceList())
        numberOfSpectra += s._Spectra.size();

      for (auto &peaks : peaksT)
      {
        const auto tol = p->GetPeakPickingBinningTolerance();
        std::vector<m2::MassValue> binPeaks;
        m2::Peaks::binPeaks(std::begin(peaks), std::end(peaks), std::back_inserter(binPeaks), tol * 10e-6);

        for (const auto &peak : binPeaks)
        {
          const auto &idx = peak.massAxisIndex;
          skyline[idx] = std::max(skyline[idx], peak.intensity);
          sum[idx] += peak.intensitySum;
        }
      }
      std::transform(std::cbegin(sum), std::cend(sum), std::begin(mean), [numberOfSpectra](const auto v) {
        return v / double(numberOfSpectra);
      });

      p->SetPropertyValue<double>("min m/z", mzAxis.front());
      p->SetPropertyValue<double>("max m/z", mzAxis.back());
    }
  }

  for (const auto &source : p->GetSourceList())
  {
    const auto &spectra = source._Spectra;
    m2::Process::Map(spectra.size(), p->GetNumberOfThreads(), [&](unsigned int /*t*/, unsigned int a, unsigned int b) {
      for (unsigned int i = a; i < b; i++)
      {
        const auto &spectrum = spectra[i];

        accIndex->SetPixelByIndex(spectrum.index + source._offset, i);

        // If mask content is generated elsewhere
        if (!_PreventInitMask)
          accMask->SetPixelByIndex(spectrum.index + source._offset, 1);

        // If it is a processed file, normalization maps are set to 1 - assuming that spectra were already processed
        if (any(source.ImportMode &
                (m2::ImzMLFormatType::ProcessedCentroid | m2::ImzMLFormatType::ProcessedMonoisotopicCentroid |
                 m2::ImzMLFormatType::ProcessedProfile)))
          accNorm->SetPixelByIndex(spectrum.index + source._offset, 1);
      }
    });
  }

  // reset prevention flags
  p->PreventMaskImageInitializationOff();
  p->PreventNormalizationImageInitializationOff();
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLMassSpecImage::ImzMLProcessor<MassAxisType, IntensityType>::GrabIntensityPrivate(
  unsigned long int index, std::vector<double> &ints, unsigned int sourceIndex) const
{
  mitk::ImagePixelReadAccessor<m2::NormImagePixelType, 3> normAcc(p->GetNormalizationImage());

  const auto &source = p->GetSourceList()[sourceIndex];
  const auto &spectrum = source._Spectra[index];
  const auto &intso = spectrum.intOffset;
  const auto &intsl = spectrum.intLength;
  const auto kernel = m2::Smoothing::savitzkyGolayKernel(p->m_SmoothingHalfWindowSize, 3);

  std::ifstream f;
  f.open(source._BinaryDataPath, std::ios::binary);

  std::vector<IntensityType> ints_get, baseline;
  binaryDataToVector(f, intso, intsl, ints_get);
  baseline.resize(intsl, 0);
  if (any(source.ImportMode & (m2::ImzMLFormatType::ContinuousProfile)))
  {
    if (p->GetNormalizationStrategy() != m2::NormalizationStrategyType::None)
    {
      const auto normalization = normAcc.GetPixelByIndex(spectrum.index + source._offset);
      std::transform(ints_get.begin(), ints_get.end(), ints_get.begin(), [&normalization](auto &val) {
        return val / normalization;
      });
    }

    if (p->GetSmoothingStrategy() != SmoothingType::None)
    {
      switch (p->GetSmoothingStrategy())
      {
        case SmoothingType::SavitzkyGolay:
          m2::Smoothing::filter(ints_get, kernel);
          break;
        default:
          break;
      }
    }

    if (p->GetBaselineCorrectionStrategy() != BaselineCorrectionType::None)
    {
      switch (p->GetBaselineCorrectionStrategy())
      {
        case BaselineCorrectionType::TopHat:
          m2::Morphology::erosion(ints_get, p->GetBaseLinecorrectionHalfWindowSize(), baseline);
          m2::Morphology::dilation(baseline, p->GetBaseLinecorrectionHalfWindowSize(), baseline);
          break;
        case BaselineCorrectionType::Median:
          m2::RunMedian::apply(ints_get, p->GetBaseLinecorrectionHalfWindowSize(), baseline);
          break;
        default:
          break;
      }

      std::transform(std::begin(ints_get), std::end(ints_get), baseline.begin(), std::begin(ints_get), std::minus<>());
    }
  }

  ints.clear();
  std::copy(std::begin(ints_get), std::end(ints_get), std::back_inserter(ints));
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLMassSpecImage::ImzMLProcessor<MassAxisType, IntensityType>::GrabMassPrivate(
  unsigned long int index, std::vector<double> &mzs, unsigned int sourceIndex) const
{
  const auto &source = p->GetSourceList()[sourceIndex];
  const auto &spectrum = source._Spectra[index];
  const auto &mzo = spectrum.mzOffset;
  const auto &mzl = spectrum.mzLength;

  std::ifstream f;
  f.open(source._BinaryDataPath, std::ios::binary);

  std::vector<MassAxisType> mzs_get;
  binaryDataToVector(f, mzo, mzl, mzs_get);
  mzs.clear();

  std::copy(std::begin(mzs_get), std::end(mzs_get), std::back_inserter(mzs));
}

m2::ImzMLMassSpecImage::~ImzMLMassSpecImage()
{
  MITK_INFO << GetStaticNameOfClass() << " destroyed!";
}

m2::ImzMLMassSpecImage::ImzMLMassSpecImage()
{
  MITK_INFO << GetStaticNameOfClass() << " created!";

  this->SetExportMode(m2::ImzMLFormatType::ContinuousProfile);
}
