/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2SpectrumContainerImage.h>
#include <m2Process.hpp>
#include <m2Timer.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkLabelSetImage.h>
#include <mitkProperties.h>
#include <signal/m2Baseline.h>
#include <signal/m2Normalization.h>
#include <signal/m2PeakDetection.h>
#include <signal/m2SpatialNormalization.h>
#include <signal/m2Pooling.h>
#include <signal/m2RunningMedian.h>
#include <signal/m2Smoothing.h>

void m2::SpectrumContainerImage::GetImage(double x, double tol, const mitk::Image *mask, mitk::Image *destImage) const
{

  AccessByItk(destImage, [](auto itkImg) { itkImg->FillBuffer(0); });
  using namespace m2;
  // accessors
  mitk::ImagePixelWriteAccessor<DisplayImagePixelType, 3> imageAccess(destImage);

  std::shared_ptr<mitk::ImagePixelReadAccessor<mitk::Label::PixelType, 3>> maskAccess;

  if (mask)
    maskAccess.reset(new mitk::ImagePixelReadAccessor<mitk::Label::PixelType, 3>(mask));
  
  GetPropertyList()->SetProperty("cm¯¹", mitk::DoubleProperty::New(x));
  GetPropertyList()->SetProperty("tol", mitk::DoubleProperty::New(tol));
  
  const auto &xs = GetXAxis();
  std::vector<double> kernel;

  // Profile (continuous) spectrum

  const auto subRes = m2::Signal::Subrange(xs, x - tol, x + tol);
  const unsigned long n = m_Spectra.size();
  // map all spectra to several threads for processing
  const unsigned int t = m2::SpectrumImage::GetNumberOfThreads();
  

  m2::Process::Map(n,
                   t,
                   [&](auto /*id*/, auto a, auto b)
                   {
                     for (unsigned int i = a; i < b; ++i)
                     {
                       auto &spectrum = m_Spectra[i];
                       auto &ys = spectrum.data;
                       auto s = std::next(std::begin(ys), subRes.first);
                       auto e = std::next(std::begin(ys), subRes.first + subRes.second);


                      if(std::distance(s,e)>=2){
                        

                        auto mean_derivative = std::inner_product(
                          std::next(s, 1), e, s,
                          0.0,
                          std::plus<>(),
                          [](auto a, auto b) { return a - b; }
                        ) / double(std::distance(s, e) - 1);
                        imageAccess.SetPixelByIndex(spectrum.index, mean_derivative);
                      }else
                        imageAccess.SetPixelByIndex(spectrum.index, Signal::RangePooling<float>(s, e, GetRangePoolingStrategy()));
                     }
                   });


    // Spatial image normalization
    const auto bufferN = std::accumulate(destImage->GetDimensions(), destImage->GetDimensions() + 3, 1, std::multiplies<>());
    switch(GetImageNormalizationStrategy()){
      case m2::ImageNormalizationStrategyType::zScore:
      {
        m2::Signal::StandardizeImage(imageAccess.GetData(), imageAccess.GetData()+bufferN, maskAccess->GetData(), imageAccess.GetData()); 
        break;
      }
      case m2::ImageNormalizationStrategyType::MinMax:
      {
        m2::Signal::MinMaxNormalizeImage(imageAccess.GetData(), imageAccess.GetData()+bufferN, maskAccess->GetData(), imageAccess.GetData());
        break;
      }
      case m2::ImageNormalizationStrategyType::ParetoScaling:
      {
        m2::Signal::ParetoScaling(imageAccess.GetData(), imageAccess.GetData()+bufferN, maskAccess->GetData(), imageAccess.GetData()); 
        break;
      }
      case m2::ImageNormalizationStrategyType::VastScaling:
        m2::Signal::VastScaling(imageAccess.GetData(), imageAccess.GetData()+bufferN, maskAccess->GetData(), imageAccess.GetData()); 
        break;
      case m2::ImageNormalizationStrategyType::RangeScaling:
        m2::Signal::RangeScaling(imageAccess.GetData(), imageAccess.GetData()+bufferN, maskAccess->GetData(), imageAccess.GetData()); 
        break;
      case m2::ImageNormalizationStrategyType::None:
      default:
      {
        break;
      }
    }
}

void m2::SpectrumContainerImage::InitializeProcessor()
{
  // this->m_Processor.reset((m2::ISpectrumImageSource *)new FsmProcessor(this));
}

void m2::SpectrumContainerImage::InitializeGeometry()
{
  
  std::array<itk::SizeValueType, 3> imageSize = {GetPropertyValue<unsigned>("dim_x"), // n_x
                                                 GetPropertyValue<unsigned>("dim_y"), // n_y
                                                 GetPropertyValue<unsigned>("dim_z")};

  std::array<double, 3> imageOrigin = {
    GetPropertyValue<double>("[IMS:1000053] absolute position offset x") * 0.001, // x_init
    GetPropertyValue<double>("[IMS:1000054] absolute position offset y") * 0.001, // y_init
    GetPropertyValue<double>("absolute position offset z") * 0.001};

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
  s[0] = GetPropertyValue<double>("spacing_x"); // x_delta
  s[1] = GetPropertyValue<double>("spacing_y"); // y_delta
  s[2] = GetPropertyValue<double>("spacing_z");

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
    InitializeByItk(caster->GetOutput());

    mitk::ImagePixelWriteAccessor<m2::DisplayImagePixelType, 3> acc(this);
    std::memset(acc.GetData(), 0, imageSize[0] * imageSize[1] * imageSize[2] * sizeof(m2::DisplayImagePixelType));
  }

  {
    using LocalImageType = itk::Image<m2::IndexImagePixelType, 3>;
    auto caster = itk::CastImageFilter<ImageType, LocalImageType>::New();
    caster->SetInput(itkIonImage);
    caster->Update();
    auto indexImage = mitk::Image::New();
    SetIndexImage(indexImage);
    indexImage->InitializeByItk(caster->GetOutput());

    mitk::ImagePixelWriteAccessor<m2::IndexImagePixelType, 3> acc(indexImage);
    std::memset(acc.GetData(), 0, imageSize[0] * imageSize[1] * imageSize[2] * sizeof(m2::IndexImagePixelType));
  }

  {
    mitk::MultiLabelSegmentation::Pointer image = mitk::MultiLabelSegmentation::New();
    image->Initialize((mitk::Image *)this);

    mitk::Color color;
    color.Set(0, 1, 0);
    auto label = mitk::Label::New();
    label->SetColor(color);
    label->SetName("Valid Spectrum");
    label->SetOpacity(0.0);
    label->SetLocked(true);
    label->SetValue(1);
    image->AddLabel(label,0);
    
    // Clone the group image to keep it alive after the MultiLabelSegmentation goes out of scope
    SetMultilabelSegmentation(image);
  }

  for( auto type : m2::NormalizationStrategyTypeList){
  
    using LocalImageType = itk::Image<m2::NormImagePixelType, 3>;
    auto caster = itk::CastImageFilter<ImageType, LocalImageType>::New();
    caster->SetInput(itkIonImage);
    caster->Update();

    auto normImage = mitk::Image::New();
    normImage->InitializeByItk(caster->GetOutput());

    mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3> acc(normImage);
    std::memset(acc.GetData(), 0, imageSize[0] * imageSize[1] * imageSize[2] * sizeof(m2::NormImagePixelType));
    this->SetNormalizationImage(normImage, type);
  }


  mitk::ImagePixelWriteAccessor<m2::DisplayImagePixelType, 3> acc(this);
  auto max_dim0 = GetDimensions()[0];
  auto max_dim1 = GetDimensions()[1];
  acc.SetPixelByIndex({0, 0, 0}, 1);
  acc.SetPixelByIndex({0, max_dim1 - 1, 0}, max_dim1 / 2);
  acc.SetPixelByIndex({max_dim0 - 1, 0, 0}, max_dim0 / 2);
  acc.SetPixelByIndex({max_dim0 - 1, max_dim1 - 1, 0}, max_dim1 + max_dim0);

  this->SetImageGeometryInitialized(true);
}

void m2::SpectrumContainerImage::InitializeNormalizationImage(m2::NormalizationStrategyType type){
  if(GetNormalizationImageStatus(type)){
    MITK_WARN << "The normalization image is already initialized. " << "type " << m2::to_string(type);
    return;
  }
  // initialize the normalization iamge
  auto image = GetNormalizationImage(type);
  // auto normImageRef = p->GetNormalizationImage(m2::NormalizationStrategyType::None);
  // image->Initialize(normImageRef);
  
  // create a write accessor
  using WriteAccessorType = mitk::ImagePixelWriteAccessor<NormImagePixelType, 3>;
  auto accNorm = std::make_shared<WriteAccessorType>(image);

  // get individual spectrum meta data
  auto &spectra = GetSpectra();
  int threads = GetNumberOfThreads();
  using namespace std;

  // split image in individual regions and process in parallel each spectrum
  Process::Map(spectra.size(),
               threads,
               [&](unsigned int /*thread*/, unsigned int a, unsigned int b)
               {
                
                 vector<double> mzs = GetXAxis();

                 for (unsigned long int i = a; i < b; i++)
                 {
                   auto &spectrum = spectra[i];

                   double v;
                   if (type == NormalizationStrategyType::Internal)
                     v = 1;
                   else
                     v = m2::Signal::GetNormalizationFactor(type, begin(mzs), end(mzs), begin(spectrum.data), end(spectrum.data));

                   accNorm->SetPixelByIndex(spectrum.index, v);
                 }
               });
  SetNormalizationImageStatus(type, true);
}

void m2::SpectrumContainerImage::InitializeImageAccess()
{
  using namespace m2;


  auto accMask = std::make_shared<mitk::ImagePixelWriteAccessor<mitk::Label::PixelType, 3>>(GetMultilabelSegmentation()->GetGroupImage(0));
  auto accIndex = std::make_shared<mitk::ImagePixelWriteAccessor<m2::IndexImagePixelType, 3>>(GetIndexImage());

    const auto currentType = GetNormalizationStrategy();
  if (!GetNormalizationImageStatus(currentType))
    InitializeNormalizationImage(currentType);

  auto accNorm = std::make_shared<mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3>>(GetNormalizationImage(currentType));

  auto &xs = GetXAxis();

  // ----- PreProcess -----

  // if the data are available as continuous data with equivalent mz axis for all
  // spectra, we can calculate the skyline, sum and mean spectrum over the image
  std::vector<std::vector<double>> skylineT;
  std::vector<std::vector<double>> sumT;

  SetPropertyValue<unsigned>("m2aia.xs.n", xs.size());
  SetPropertyValue<double>("m2aia.xs.min", xs.front());
  SetPropertyValue<double>("m2aia.xs.max", xs.back());

  skylineT.resize(GetNumberOfThreads(), std::vector<double>(xs.size(), 0));
  sumT.resize(GetNumberOfThreads(), std::vector<double>(xs.size(), 0));

  // m2::Timer t("Initialize image");

  m2::Process::Map(
    GetSpectra().size(),
    GetNumberOfThreads(),
    [&](unsigned int t, unsigned int a, unsigned int b)
    {
      m2::Signal::SmoothingFunctor<float> Smoother;
      Smoother.Initialize(GetSmoothingStrategy(), GetSmoothingHalfWindowSize());

      m2::Signal::BaselineFunctor<float> BaselineSubtractor;
      BaselineSubtractor.Initialize(GetBaselineCorrectionStrategy(), GetBaseLineCorrectionHalfWindowSize());

      std::vector<float> baseline(xs.size());

      auto &spectra = GetSpectra();

      // const auto divides = [&val](const auto &a) { return a / val; };
      const auto maximum = [](const auto &a, const auto &b) { return a > b ? a : b; };
      const auto plus = std::plus<>();

      for (unsigned long int i = a; i < b; i++)
      {
        auto &spectrum = spectra[i];
        auto &ys = spectrum.data;
        
        std::transform(std::begin(ys), std::end(ys), std::begin(ys), [](double absorbance) {
          return std::pow(10.0, -absorbance) * 100.0;
        });
        
        // double t0 = 0.01;
        // auto trans =  [t0](double x){return std::pow(10.0,-std::log10((x/t0)/100.0))*100.0;};

        // std::transform(std::begin(ys), std::end(ys), std::begin(ys), trans);
        
        accNorm->SetPixelByIndex(spectrum.index, std::accumulate(std::begin(ys),
                                                                 std::end(ys),
                                                                 0.0,
                                                                 plus));

        Smoother(std::begin(ys), std::end(ys));
        BaselineSubtractor(std::begin(ys), std::end(ys), std::begin(baseline));

        std::transform(std::begin(ys), std::end(ys), sumT.at(t).begin(), sumT.at(t).begin(), plus);
        std::transform(std::begin(ys), std::end(ys), skylineT.at(t).begin(), skylineT.at(t).begin(), maximum);



      }
    });

  const auto &spectra = GetSpectra();
  m2::Process::Map(spectra.size(),
                   GetNumberOfThreads(),
                   [&](unsigned int /*t*/, unsigned int a, unsigned int b)
                   {
                     for (unsigned int i = a; i < b; i++)
                     {
                       const auto &spectrum = spectra[i];
                       accIndex->SetPixelByIndex(spectrum.index, i);
                       accMask->SetPixelByIndex(spectrum.index, 1);
                     }
                   });

  auto &skyline = GetSkylineSpectrum();
  skyline.resize(xs.size(), 0);
  for (unsigned int t = 0; t < GetNumberOfThreads(); ++t)
    std::transform(skylineT[t].begin(),
                   skylineT[t].end(),
                   skyline.begin(),
                   skyline.begin(),
                   [](auto &a, auto &b) { return a > b ? a : b; });

  auto &mean = GetMeanSpectrum();
  auto &sum = GetSumSpectrum();

  mean.resize(xs.size(), 0);
  sum.resize(xs.size(), 0);

  // accumulate valid spectra defined by mask image
  auto N = std::accumulate(accMask->GetData(),
                           accMask->GetData() + GetSpectra().size(),
                           mitk::Label::PixelType(0),
                           [](const auto &a, const auto &b) -> mitk::Label::PixelType { return a + (b > 0); });

  for (unsigned int t = 0; t < GetNumberOfThreads(); ++t)
    std::transform(
      sumT[t].begin(), sumT[t].end(), sum.begin(), sum.begin(), [](const auto &a, const auto &b) { return a + b; });
  std::transform(sum.begin(), sum.end(), mean.begin(), [&N](const auto &a) { return a / double(N); });


  this->SetImageAccessInitialized(true);
}


m2::SpectrumContainerImage::~SpectrumContainerImage()
{
  MITK_INFO << GetStaticNameOfClass() << " destroyed!";
}

m2::SpectrumContainerImage::SpectrumContainerImage()
{
  MITK_INFO << GetStaticNameOfClass() << " created!";

  m_SpectrumType.XAxisLabel = "cm¯¹";
  m_SpectrumType.Format = m2::SpectrumFormat::ContinuousProfile;
}
