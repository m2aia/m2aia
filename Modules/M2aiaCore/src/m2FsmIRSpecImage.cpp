/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2Baseline.h>
#include <m2FsmIRSpecImage.h>
#include <m2Normalization.h>
#include <m2PeakDetection.h>
#include <m2Pooling.h>
#include <m2Process.hpp>
#include <m2RunningMedian.h>
#include <m2Smoothing.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkProperties.h>
#include <mitkTimer.h>

void m2::FsmIRSpecImage::GenerateImageData(double mz, double tol, const mitk::Image *mask, mitk::Image *img) const
{
  m2::SpectrumImageBase::GenerateImageData(mz, tol, mask, img);
}

template <class XAxisType, class IntensityType>
void m2::FsmIRSpecImage::FsmProcessor<XAxisType, IntensityType>::GrabIonImagePrivate(double cmInv,
                                                                                     double tol,
                                                                                     const mitk::Image *mask,
                                                                                     mitk::Image *destImage) const
{
  AccessByItk(destImage, [](auto itkImg) { itkImg->FillBuffer(0); });
  using namespace m2;
  // accessors
  mitk::ImagePixelWriteAccessor<DisplayImagePixelType, 3> imageAccess(destImage);
  mitk::ImagePixelReadAccessor<NormImagePixelType, 3> normAccess(p->GetNormalizationImage());
  std::shared_ptr<mitk::ImagePixelReadAccessor<MaskImagePixelType, 3>> maskAccess;
  
  const auto _NormalizationStrategy = p->GetNormalizationStrategy();
  
  if (mask)
  {
    maskAccess.reset(new mitk::ImagePixelReadAccessor<MaskImagePixelType, 3>(mask));
    MITK_INFO << "> Use mask image";
  }
  p->SetProperty("cm^-1", mitk::DoubleProperty::New(cmInv));
  p->SetProperty("tol", mitk::DoubleProperty::New(tol));
  auto mdMz = itk::MetaDataObject<double>::New();
  mdMz->SetMetaDataObjectValue(cmInv);
  auto mdTol = itk::MetaDataObject<double>::New();
  mdTol->SetMetaDataObjectValue(tol);
  p->GetMetaDataDictionary()["cm^-1"] = mdMz;
  p->GetMetaDataDictionary()["tol"] = mdTol;

  std::vector<XAxisType> wavelenght = p->GetXAxis();
  std::vector<double> kernel;

  // Profile (continuous) spectrum
  const auto &source = p->GetSpectrumImageSource();
  if (any(source.ImportMode & SpectrumFormatType::ContinuousProfile))
  {
    const auto subRes = m2::Signal::Subrange(wavelenght, cmInv - tol, cmInv + tol);

    for (auto &source : p->GetSpectrumImageSourceList())
    {
      // map all spectra to several threads for processing
      const unsigned long n = source._Spectra.size();
      const unsigned t = p->GetNumberOfThreads();

      m2::Process::Map(n, t, [&](auto /*id*/, auto a, auto b) {
        const auto Smoother =
          m2::Signal::CreateSmoother<IntensityType>(p->GetSmoothingStrategy(), p->GetSmoothingHalfWindowSize(), false);

        auto BaselineSubstractor = m2::Signal::CreateSubstractBaselineConverter<IntensityType>(
          p->GetBaselineCorrectionStrategy(), p->GetBaseLineCorrectionHalfWindowSize());

        const auto Normalizor = m2::Signal::CreateNormalizor<IntensityType, XAxisType>(p->GetNormalizationStrategy());

        std::vector<IntensityType> ints(wavelenght.size());
        std::vector<IntensityType> baseline(wavelenght.size());

        const auto &_Spectra = source._Spectra;
        for (unsigned int i = a; i < b; ++i)
        {
          const auto &spectrum = _Spectra[i];
          std::copy(std::cbegin(spectrum.data), std::cend(spectrum.data), std::begin(ints));

          auto s = std::next(std::begin(ints), subRes.first);
          auto e = std::next(std::begin(ints), subRes.first + subRes.second);

          if (maskAccess && maskAccess->GetPixelByIndex(spectrum.index + source._offset) == 0)
          {
            imageAccess.SetPixelByIndex(spectrum.index + source._offset, 0);
            continue;
          }

          // if a normalization image exist, apply normalization before any further calculations are performed
          if (_NormalizationStrategy != m2::NormalizationStrategyType::None)
          {
            IntensityType norm = normAccess.GetPixelByIndex(spectrum.index + source._offset);
            std::transform(std::begin(ints), std::end(ints), std::begin(ints), [&norm](auto &v) { return v / norm; });
          }

          Smoother(ints);
          BaselineSubstractor(ints);

          auto val = Signal::RangePooling<IntensityType>(s, e, p->GetRangePoolingStrategy());

          imageAccess.SetPixelByIndex(spectrum.index + source._offset, val);
        }
      });
    }
  }
}

void m2::FsmIRSpecImage::InitializeProcessor()
{
  this->m_Processor.reset((m2::SpectrumImageBase::ProcessorBase *)new FsmProcessor<double, float>(this));
}

void m2::FsmIRSpecImage::InitializeGeometry()
{
  if (!m_Processor)
    this->InitializeProcessor();
  this->m_Processor->InitializeGeometry();
  this->SetImageGeometryInitialized(true);
}

void m2::FsmIRSpecImage::InitializeImageAccess()
{
  this->m_Processor->InitializeImageAccess();
  this->SetImageAccessInitialized(true);
}

template <class XAxisType, class IntensityType>
void m2::FsmIRSpecImage::FsmProcessor<XAxisType, IntensityType>::InitializeGeometry()
{
  auto &imageArtifacts = p->GetImageArtifacts();

  std::array<itk::SizeValueType, 3> imageSize = {p->GetPropertyValue<unsigned>("dim_x"), // n_x
                                                 p->GetPropertyValue<unsigned>("dim_y"), // n_y
                                                 p->GetPropertyValue<unsigned>("dim_z")};

  std::array<double, 3> imageOrigin = {p->GetPropertyValue<double>("origin x") * 0.001, // x_init
                                       p->GetPropertyValue<double>("origin y") * 0.001, // y_init
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
  s[0] = p->GetPropertyValue<double>("spacing_x"); // x_delta
  s[1] = p->GetPropertyValue<double>("spacing_y"); // y_delta
  s[2] = p->GetPropertyValue<double>("spacing_z");

  auto d = itkIonImage->GetDirection();
  d[0][0] = -1;

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

  mitk::ImagePixelWriteAccessor<m2::DisplayImagePixelType, 3> acc(p);
  auto max_dim0 = p->GetDimensions()[0];
  auto max_dim1 = p->GetDimensions()[1];
  acc.SetPixelByIndex({0, 0, 0}, 1);
  acc.SetPixelByIndex({0, max_dim1 - 1, 0}, max_dim1 / 2);
  acc.SetPixelByIndex({max_dim0 - 1, 0, 0}, max_dim0 / 2);
  acc.SetPixelByIndex({max_dim0 - 1, max_dim1 - 1, 0}, max_dim1 + max_dim0);
}

template <class XAxisType, class IntensityType>
void m2::FsmIRSpecImage::FsmProcessor<XAxisType, IntensityType>::InitializeImageAccess()
{
  using namespace m2;

  auto accMask = std::make_shared<mitk::ImagePixelWriteAccessor<m2::MaskImagePixelType, 3>>(p->GetMaskImage());
  auto accIndex = std::make_shared<mitk::ImagePixelWriteAccessor<m2::IndexImagePixelType, 3>>(p->GetIndexImage());
  auto accNorm = std::make_shared<mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3>>(p->GetNormalizationImage());

  std::vector<XAxisType> wavelength = p->GetXAxis();

  // ----- PreProcess -----

  // if the data are available as continuous data with equivalent mz axis for all
  // spectra, we can calculate the skyline, sum and mean spectrum over the image
  std::vector<std::vector<double>> skylineT;
  std::vector<std::vector<double>> sumT;

  // shortcuts
  const auto _NormalizationStrategy = p->GetNormalizationStrategy();

  
  const auto &source = p->GetSpectrumImageSourceList().front();

  if (any(source.ImportMode & m2::SpectrumFormatType::ContinuousProfile))
  {
    p->SetPropertyValue<unsigned>("spectral depth", wavelength.size());
    p->SetPropertyValue<double>("min cm^-1", wavelength.front());
    p->SetPropertyValue<double>("max cm^-1", wavelength.back());

    skylineT.resize(p->GetNumberOfThreads(), std::vector<double>(wavelength.size(), 0));
    sumT.resize(p->GetNumberOfThreads(), std::vector<double>(wavelength.size(), 0));

    mitk::Timer t("Initialize image");
    for (const auto &source : p->GetSpectrumImageSourceList())
    {
      m2::Process::Map(
        source._Spectra.size(), p->GetNumberOfThreads(), [&](unsigned int t, unsigned int a, unsigned int b) {
          const auto Smoother = m2::Signal::CreateSmoother<IntensityType>(
            p->GetSmoothingStrategy(), p->GetSmoothingHalfWindowSize(), false);

          auto BaselineSubstractor = m2::Signal::CreateSubstractBaselineConverter<IntensityType>(
            p->GetBaselineCorrectionStrategy(), p->GetBaseLineCorrectionHalfWindowSize());

          const auto Normalizor = m2::Signal::CreateNormalizor<XAxisType, IntensityType>(p->GetNormalizationStrategy());

          IntensityType val = 1;
          std::vector<IntensityType> ints(wavelength.size(), 0);
          std::vector<IntensityType> baseline(wavelength.size(), 0);

          const auto &spectra = source._Spectra;

          const auto divides = [&val](const auto &a) { return a / val; };
          const auto maximum = [](const auto &a, const auto &b) { return a > b ? a : b; };
          const auto plus = std::plus<>();

          for (unsigned long int i = a; i < b; i++)
          {
            const auto &spectrum = spectra[i];
            std::copy(std::begin(spectra[i].data), std::end(spectra[i].data), std::begin(ints));

            if (_NormalizationStrategy != NormalizationStrategyType::None)
            {
              val = Normalizor(wavelength, ints, accNorm->GetPixelByIndex(spectrum.index + source._offset));
              accNorm->SetPixelByIndex(spectrum.index + source._offset, val); // Set normalization image pixel value
            }
            else
            {
              // Normalization-image content was set elsewhere
              val = accNorm->GetPixelByIndex(spectrum.index + source._offset);
              std::transform(std::begin(ints), std::end(ints), std::begin(ints), divides);
            }

            Smoother(ints);
            BaselineSubstractor(ints);

            std::transform(std::begin(ints), std::end(ints), sumT.at(t).begin(), sumT.at(t).begin(), plus);
            std::transform(std::begin(ints), std::end(ints), skylineT.at(t).begin(), skylineT.at(t).begin(), maximum);
          }
        });
    }
    auto &skyline = p->SkylineSpectrum();
    skyline.resize(wavelength.size(), 0);
    for (unsigned int t = 0; t < p->GetNumberOfThreads(); ++t)
      std::transform(skylineT[t].begin(), skylineT[t].end(), skyline.begin(), skyline.begin(), [](auto &a, auto &b) {
        return a > b ? a : b;
      });

    auto &mean = p->MeanSpectrum();
    auto &sum = p->SumSpectrum();

    mean.resize(wavelength.size(), 0);
    sum.resize(wavelength.size(), 0);

    auto N = std::accumulate(std::begin(p->GetSpectrumImageSourceList()),
                             std::end(p->GetSpectrumImageSourceList()),
                             unsigned(0),
                             [](const auto &a, const auto &source) { return a + source._Spectra.size(); });

    for (unsigned int t = 0; t < p->GetNumberOfThreads(); ++t)
      std::transform(sumT[t].begin(), sumT[t].end(), sum.begin(), sum.begin(), [](auto &a, auto &b) { return a + b; });
    std::transform(sum.begin(), sum.end(), mean.begin(), [&](auto &a) { return a / double(N); });
  }
  //////////---------------------------
  for (const auto &source : p->GetSpectrumImageSourceList())
  {
    const auto &spectra = source._Spectra;
    m2::Process::Map(spectra.size(), p->GetNumberOfThreads(), [&](unsigned int /*t*/, unsigned int a, unsigned int b) {
      for (unsigned int i = a; i < b; i++)
      {
        const auto &spectrum = spectra[i];

        accIndex->SetPixelByIndex(spectrum.index + source._offset, i);
        accMask->SetPixelByIndex(spectrum.index + source._offset, 1);
      }
    });
  }
}

template <class XAxisType, class IntensityType>
void m2::FsmIRSpecImage::FsmProcessor<XAxisType, IntensityType>::GrabIntensityPrivate(unsigned long int index,
                                                                                      std::vector<double> &ints,
                                                                                      unsigned int sourceIndex) const
{
  mitk::ImagePixelReadAccessor<m2::NormImagePixelType, 3> normAcc(p->GetNormalizationImage());

  const auto &source = p->GetSpectrumImageSourceList()[sourceIndex];
  const auto &spectrum = source._Spectra[index];
  // const auto kernel = m2::Signal::savitzkyGolayKernel(p->m_SmoothingHalfWindowSize, 3);

  ints.clear();
  std::copy(std::begin(spectrum.data), std::end(spectrum.data), std::back_inserter(ints));

  double d;
  auto divide = [&d](auto &val) { return val / d; };
  auto Smoother = m2::Signal::CreateSmoother<double>(p->GetSmoothingStrategy(), p->GetSmoothingHalfWindowSize(), false);
  auto BaselineSubstractor = m2::Signal::CreateSubstractBaselineConverter<double>(
    p->GetBaselineCorrectionStrategy(), p->GetBaseLineCorrectionHalfWindowSize());

  if (any(source.ImportMode & (m2::SpectrumFormatType::ContinuousProfile)))
  {
    if (p->GetNormalizationStrategy() != m2::NormalizationStrategyType::None)
    {
      d = normAcc.GetPixelByIndex(spectrum.index + source._offset);
      std::transform(ints.begin(), ints.end(), ints.begin(), divide);
    }

    Smoother(ints);
    BaselineSubstractor(ints);
  }
}

template <class XAxisType, class IntensityType>
void m2::FsmIRSpecImage::FsmProcessor<XAxisType, IntensityType>::GrabMassPrivate(unsigned long int,
                                                                                 std::vector<double> &mzs,
                                                                                 unsigned int) const
{
  mzs.clear();
  std::copy(std::begin(p->GetXAxis()), std::end(p->GetXAxis()), std::back_inserter(mzs));
}

m2::FsmIRSpecImage::~FsmIRSpecImage()
{
  MITK_INFO << GetStaticNameOfClass() << " destroyed!";
}

m2::FsmIRSpecImage::FsmIRSpecImage()
{
  MITK_INFO << GetStaticNameOfClass() << " created!";
}
