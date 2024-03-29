/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#pragma once

#include <M2aiaCoreExports.h>
#include <itkCastImageFilter.h>
#include <m2ISpectrumImageSource.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2Timer.h>
#include <m2Process.hpp>
#include <mitkCoreServices.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>
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
namespace m2
{
  class ImzMLSpectrumImage;

  template <class MassAxisType, class IntensityType>
  class ImzMLSpectrumImageSource : public m2::ISpectrumImageSource
  {
  private:
    m2::ImzMLSpectrumImage *p;

  public:
    explicit ImzMLSpectrumImageSource(m2::ImzMLSpectrumImage *owner) : p(owner) {}
    virtual void GetImagePrivate(double mz, double tol, const mitk::Image *mask, mitk::Image *image);
    // void GetSpectrumPrivate(unsigned int, std::vector<float> &, std::vector<float> &, unsigned int) override {}

    void InitializeImageAccess() override;
    void InitializeGeometry() override;
    
    
    virtual void InitializeImageAccessContinuousProfile();

    /**
     * @brief Provides optimized access to centroid data.
     * No binning is applied. Normalization factors are provided.
     */
    virtual void InitializeImageAccessContinuousCentroid();

    /**
     * @brief See InitializeImageAccessProcessedData()
     */
    virtual void InitializeImageAccessProcessedProfile();

    /**
     * @brief See InitializeImageAccessProcessedData()
     */
    virtual void InitializeImageAccessProcessedCentroid();

    /**
     * @brief Provides access to processed centroid and processed profile spectra.
     * It includes binning for visualization purposes of the overview spectra
     * and invokes the calculation of normalization factors, but no further
     * signal-processing is supported here.
     */
    virtual void InitializeImageAccessProcessedData();

    /**
     * @brief Calculate and store the normalization image
    */
    void InitializeNormalizationImage(m2::NormalizationStrategyType type) override;

    /**
     * @brief Convert binary data to a vector.
     * @tparam OffsetType Type of the offset.
     * @tparam LengthType Type of the length.
     * @tparam DataType Type of the data.
     * @param f Input file stream.
     * @param offset The offset to start reading from.
     * @param length The length of data to read.
     * @param vec Pointer to the vector to store the data.
     */
    template <class OffsetType, class LengthType, class DataType>
    static void binaryDataToVector(std::ifstream &f, OffsetType offset, LengthType length, DataType *vec) noexcept
    {
      f.seekg(offset);
      f.read((char *)vec, length * sizeof(DataType));
    }

    template <class ItXFirst, class ItXLast, class ItYFirst, class ItYLast>
    static inline double GetNormalizationFactor(
      m2::NormalizationStrategyType strategy, ItXFirst xFirst, ItXLast xLast, ItYFirst yFirst, ItYLast yLast)
    {
      using namespace std;
      switch (strategy)
      {
        case m2::NormalizationStrategyType::TIC:
          return m2::Signal::TotalIonCurrent(xFirst, xLast, yFirst);
        case m2::NormalizationStrategyType::Sum:
          return accumulate(yFirst, yLast, double(0.0));
        case m2::NormalizationStrategyType::Mean:
          return accumulate(yFirst, yLast, double(0.0)) / double(std::distance(yFirst, yLast));
        case m2::NormalizationStrategyType::Max:
          return *max_element(yFirst, yLast);
        case m2::NormalizationStrategyType::RMS:
          return m2::Signal::RootMeanSquare(yFirst, yLast);
        case m2::NormalizationStrategyType::None:
        case m2::NormalizationStrategyType::Internal:
        case m2::NormalizationStrategyType::External:
        default:
          return 1;
      }
    }

    using XIteratorType = typename std::vector<MassAxisType>::iterator;
    using YIteratorType = typename std::vector<IntensityType>::iterator;
    m2::Signal::SmoothingFunctor<IntensityType> m_Smoother;
    m2::Signal::BaselineFunctor<IntensityType> m_BaselineSubtractor;
    m2::Signal::IntensityTransformationFunctor<IntensityType> m_Transformer;

    virtual void GetYValues(unsigned int id, std::vector<float> &yd) { GetYValues<float>(id, yd); }
    virtual void GetYValues(unsigned int id, std::vector<double> &yd) { GetYValues<double>(id, yd); }
    virtual void GetXValues(unsigned int id, std::vector<float> &yd) { GetXValues<float>(id, yd); }
    virtual void GetXValues(unsigned int id, std::vector<double> &yd) { GetXValues<double>(id, yd); }

  private:
    template <class OutputType>
    void GetYValues(unsigned int id, std::vector<OutputType> &yd);
    template <class OutputType>
    void GetXValues(unsigned int id, std::vector<OutputType> &xd);
  };

} // namespace m2


template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImageSource<MassAxisType, IntensityType>::InitializeNormalizationImage(m2::NormalizationStrategyType type){

  // initialize the normalization iamge
  auto image = p->GetNormalizationImage(type);
  
  
  // create a write accessor
  using WriteAccessorType = mitk::ImagePixelWriteAccessor<NormImagePixelType, 3>;
  auto accNorm = std::make_shared<WriteAccessorType>(image);

  // get individual spectrum meta data
  auto &spectra = p->GetSpectra();
  int threads = p->GetNumberOfThreads();
  using namespace std;

  // split image in individual regions and process in parallel each spectrum
  Process::Map(
    spectra.size(),
    threads,
    [&](unsigned int /*thread*/, unsigned int a, unsigned int b)
    {
      
      
      ifstream f(p->GetBinaryDataPath(), ifstream::binary);
      vector<MassAxisType> mzs;
      vector<IntensityType> ints;

      for (unsigned long int i = a; i < b; i++)
      {
        auto &spectrum = spectra[i];

        mzs.resize(spectrum.mzLength);
        ints.resize(spectrum.intLength);
        binaryDataToVector(f, spectrum.mzOffset, spectrum.mzLength, mzs.data());
        binaryDataToVector(f, spectrum.intOffset, spectrum.intLength, ints.data());

        double v;       
        if(type == NormalizationStrategyType::Internal)
          v = spectrum.inFileNormalizationFactor;
        else
           v = GetNormalizationFactor(type, begin(mzs), end(mzs), begin(ints), end(ints));
        
        accNorm->SetPixelByIndex(spectrum.index, v); 
      }
    });

}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImageSource<MassAxisType, IntensityType>::GetImagePrivate(double xRangeCenter,
                                                                                double xRangeTol,
                                                                                const mitk::Image *mask,
                                                                                mitk::Image *destImage)
{

  // Check normalization strategy
  bool useNormalization = p->GetNormalizationStrategy() != m2::NormalizationStrategyType::None;

  // check if the normalization iamge was already initialized for this type of normalization
  // and initialize the image if necessary
  auto currentType = p->GetNormalizationStrategy();
  if(!p->GetNormalizationImageStatus(currentType)){
    InitializeNormalizationImage(currentType);
    p->SetNormalizationImageStatus(currentType, true);
  } 

  AccessByItk(destImage, [](auto itkImg) { itkImg->FillBuffer(0); });
  using namespace m2;
  // accessors
  mitk::ImagePixelWriteAccessor<DisplayImagePixelType, 3> imageAccess(destImage);
  mitk::ImagePixelReadAccessor<NormImagePixelType, 3> normAccess(p->GetNormalizationImage());
  std::shared_ptr<mitk::ImagePixelReadAccessor<mitk::LabelSetImage::PixelType, 3>> maskAccess;

  if (mask)
    maskAccess.reset(new mitk::ImagePixelReadAccessor<mitk::LabelSetImage::PixelType, 3>(mask));

  p->SetProperty("m2aia.xs.selection.center", mitk::DoubleProperty::New(xRangeCenter));
  p->SetProperty("m2aia.xs.selection.tolerance", mitk::DoubleProperty::New(xRangeTol));
  
  // Profile (continuous) spectrum
  const auto spectrumType = p->GetSpectrumType();
  const unsigned threads = p->GetNumberOfThreads();


  if (spectrumType.Format == m2::SpectrumFormat::ContinuousProfile)
  {
    // xRangeCenter subrange
    const auto mzs = p->GetXAxis();
    const auto _BaselineCorrectionHWS = p->GetBaseLineCorrectionHalfWindowSize();
    const auto _BaseLineCorrectionStrategy = p->GetBaselineCorrectionStrategy();

    // Image generation is based on range operations on the full spectrum for each pixel.
    // We are not interested in values outside of the range.
    // We can read only values of interest from the *.ibd file.
    // For this we have to manipulate the byte offset position.
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

    const auto &spectra = p->GetSpectra();
    m2::Process::Map(spectra.size(),
                     threads,
                     [&](auto /*id*/, auto a, auto b)
                     {
                       std::ifstream f(p->GetBinaryDataPath(), std::iostream::binary);
                       std::vector<IntensityType> ints(newLength);
                       std::vector<IntensityType> baseline(newLength);
                       // 5) (For a specific thread), save the true range positions '(' and ')'
                       // for pooling in the data vector. Continue at 6.
                       // |>>>>>>>>>[^^^^^(********c********)^^^^^]<<<<<<<<<<<<<<<<<<<<<<<<<|
                       auto s = std::next(std::begin(ints), padding_left);
                       auto e = std::prev(std::end(ints), padding_right);

                       for (unsigned int i = a; i < b; ++i)
                       {
                         const auto &spectrum = spectra[i];

                         // check if outside of mask
                         if (maskAccess && maskAccess->GetPixelByIndex(spectrum.index) == 0)
                         {
                           imageAccess.SetPixelByIndex(spectrum.index, 0);
                           continue;
                         }
                         // 6) (For a specific pixel) recalculate the new offset
                         // |>>>>>>>>>[^^^^^(********c********)^^^^^]<<<<<<<<<<<<<<<<<<<<<<<<<|
                         const auto newOffset = spectrum.intOffset + newOffsetModifier; // new offset
                         binaryDataToVector(f, newOffset, newLength, ints.data());

                         // ----- Normalization
                         if (useNormalization)
                         { // check if it is not NormalizationStrategy::None.
                           IntensityType norm = normAccess.GetPixelByIndex(spectrum.index);
                           std::transform(
                             std::begin(ints), std::end(ints), std::begin(ints), [&norm](auto &v) { return v / norm; });
                         }
                        
                         // ----- Smoothing
                         m_Smoother(std::begin(ints), std::end(ints));

                         // ----- Baseline Substraction
                         m_BaselineSubtractor(std::begin(ints), std::end(ints), std::begin(baseline));

                         // ----- Intensity Transformation
                         m_Transformer(std::begin(ints), std::end(ints));

                         // ----- Pool the range
                         const auto val = Signal::RangePooling<IntensityType>(s, e, p->GetRangePoolingStrategy());

                         // finally set the pixel value
                         imageAccess.SetPixelByIndex(spectrum.index, val);
                       }
                     });
  }

  else if (any(spectrumType.Format & (m2::SpectrumFormat::ContinuousCentroid | m2::SpectrumFormat::ProcessedCentroid |
                                      m2::SpectrumFormat::ProcessedProfile)))
  {
    const auto &spectra = p->GetSpectra();
    m2::Process::Map(
      spectra.size(),
      threads,
      [&](auto /*id*/, auto a, auto b)
      {
        std::ifstream f(p->GetBinaryDataPath(), std::iostream::binary);
        std::vector<IntensityType> ints;
        std::vector<MassAxisType> mzs;
        for (unsigned int i = a; i < b; ++i)
        {
          auto &spectrum = spectra[i];
          if (maskAccess && maskAccess->GetPixelByIndex(spectrum.index) == 0)
          {
            imageAccess.SetPixelByIndex(spectrum.index, 0);
            continue;
          }

          mzs.resize(spectrum.mzLength);
          binaryDataToVector(
            f, spectrum.mzOffset, spectrum.mzLength, mzs.data()); // !! read mass axis for each spectrum

          auto subRes = m2::Signal::Subrange(mzs, xRangeCenter - xRangeTol, xRangeCenter + xRangeTol);
          if (subRes.second == 0)
          {
            imageAccess.SetPixelByIndex(spectrum.index, 0);
            continue;
          }

          ints.resize(subRes.second);
          binaryDataToVector(f, spectrum.intOffset + subRes.first * sizeof(IntensityType), subRes.second, ints.data());

          // TODO: Is it useful to normalize centroid data?
          if (useNormalization)
          {
            IntensityType norm = normAccess.GetPixelByIndex(spectrum.index);
            std::transform(std::begin(ints),
                           std::end(ints),
                           std::begin(ints),
                           [&norm](auto &v) { return v / norm; });
          }

          auto val =
            Signal::RangePooling<IntensityType>(std::begin(ints), std::end(ints), p->GetRangePoolingStrategy());
          imageAccess.SetPixelByIndex(spectrum.index, val);
        }
      });
  }
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImageSource<MassAxisType, IntensityType>::InitializeGeometry()
{
  std::array<itk::SizeValueType, 3> imageSize = {p->GetPropertyValue<unsigned int>("[IMS:1000042] max count of pixels x"),
                                                 p->GetPropertyValue<unsigned int>("[IMS:1000043] max count of pixels y"),
                                                 p->GetPropertyValue<unsigned int>("max count of pixels z")};

  std::array<double, 3> imageOrigin = {p->GetPropertyValue<double>("[IMS:1000053] absolute position offset x"),
                                       p->GetPropertyValue<double>("[IMS:1000054] absolute position offset y"),
                                       p->GetPropertyValue<double>("absolute position offset z")};

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
  s[0] = p->GetPropertyValue<double>("[IMS:1000046] pixel size x");
  s[1] = p->GetPropertyValue<double>("[IMS:1000047] pixel size y");
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
    p->SetIndexImage(indexImage);
    indexImage->InitializeByItk(caster->GetOutput());

    mitk::ImagePixelWriteAccessor<m2::IndexImagePixelType, 3> acc(indexImage);
    std::memset(acc.GetData(), 0, imageSize[0] * imageSize[1] * imageSize[2] * sizeof(m2::IndexImagePixelType));
  }

  {
    auto image = mitk::LabelSetImage::New();
    p->SetMaskImage(image.GetPointer());
    image->Initialize((mitk::Image *)p);
    auto ls = image->GetActiveLabelSet();

    mitk::Color color;
    color.Set(0.0, 1, 0.0);
    auto label = mitk::Label::New();
    label->SetColor(color);
    label->SetName("Valid");
    label->SetOpacity(0.0);
    label->SetLocked(true);
    label->SetValue(1);
    ls->AddLabel(label);
  }

  auto max_dim0 = p->GetDimensions()[0];
  auto max_dim1 = p->GetDimensions()[1];
  std::vector<std::thread> threads;
  for(auto type : m2::NormalizationStrategyTypeList)
  {
    m2::Timer t("Initialization of the Normalization images took");
    t.printIf = [](m2::Timer::Duration d) -> bool{return d.count() > 1.0;};

    using LocalImageType = itk::Image<m2::NormImagePixelType, 3>;
    auto caster = itk::CastImageFilter<ImageType, LocalImageType>::New();
    caster->SetInput(itkIonImage);
    caster->Update();
    auto normImage = mitk::Image::New();
    p->SetNormalizationImage(normImage, type);
    normImage->InitializeByItk(caster->GetOutput());

    {
      mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3> acc(normImage);
      std::memset(acc.GetData(), 1, imageSize[0] * imageSize[1] * imageSize[2] * sizeof(m2::NormImagePixelType));
    }
    
    
      
    p->InitializeNormalizationImage(type);
    p->SetNormalizationImageStatus(type,true);
  

  
  }

  mitk::ImagePixelWriteAccessor<m2::DisplayImagePixelType, 3> acc(p);
  acc.SetPixelByIndex({0, 0, 0}, 1);
  acc.SetPixelByIndex({0, max_dim1 - 1, 0}, max_dim1 / 2);
  acc.SetPixelByIndex({max_dim0 - 1, 0, 0}, max_dim0 / 2);
  acc.SetPixelByIndex({max_dim0 - 1, max_dim1 - 1, 0}, max_dim1 + max_dim0);
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImageSource<MassAxisType, IntensityType>::InitializeImageAccess()
{
  p->SetImageAccessInitialized(false);
  //////////---------------------------
  const auto spectrumType = p->GetSpectrumType();

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

  const auto &spectra = p->GetSpectra();
  m2::Process::Map(spectra.size(),
                   p->GetNumberOfThreads(),
                   [&](unsigned int /*t*/, unsigned int a, unsigned int b)
                   {
                     for (unsigned int i = a; i < b; i++)
                     {
                       const auto &spectrum = spectra[i];

                       accIndex->SetPixelByIndex(spectrum.index, i);
                       accMask->SetPixelByIndex(spectrum.index, 1);

                       // If it is a processed file, normalization maps are set to 1 - assuming that spectra were
                       // already processed if (any(importMode & (m2::SpectrumFormatType::ProcessedCentroid |
                       // m2::SpectrumFormatType::ProcessedProfile)))
                       //   accNorm->SetPixelByIndex(spectrum.index + source.m_Offset, 1);
                     }
                   });
  p->SetNumberOfValidPixels(spectra.size());
  p->SetImageAccessInitialized(true);
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImageSource<MassAxisType, IntensityType>::InitializeImageAccessProcessedProfile()
{
  // MITK_INFO("m2::ImzMLSpectrumImage") << "Start InitializeImageAccessProcessedProfile";
  InitializeImageAccessProcessedData();
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImageSource<MassAxisType, IntensityType>::InitializeImageAccessContinuousProfile()
{

  std::vector<std::vector<double>> skylineT;
  std::vector<std::vector<double>> sumT;
  std::vector<MassAxisType> mzs;
  

  unsigned int threads = p->GetNumberOfThreads();

  // load m/z axis
  {
    const auto &spectra = p->GetSpectra();
    std::ifstream f(p->GetBinaryDataPath(), std::ios::binary);
    mzs.resize(spectra[0].mzLength);
    binaryDataToVector(f, spectra[0].mzOffset, spectra[0].mzLength, mzs.data());
    auto &massAxis = p->GetXAxis();
    massAxis.clear();
    std::copy(std::begin(mzs), std::end(mzs), std::back_inserter(massAxis));
    p->SetPropertyValue<unsigned>("m2aia.xs.n", mzs.size());
    p->SetPropertyValue<double>("m2aia.xs.min", mzs.front());
    p->SetPropertyValue<double>("m2aia.xs.max", mzs.back());
  }

  skylineT.resize(threads, std::vector<double>(mzs.size(), 0));
  sumT.resize(threads, std::vector<double>(mzs.size(), 0));

  const auto Maximum = [](const auto &a, const auto &b) { return a > b ? a : b; };
  const auto plus = std::plus<>();

  // initialize normalization image accessor
  auto currentType = p->GetNormalizationStrategy();
  using NormImageReadAccess = mitk::ImagePixelReadAccessor<NormImagePixelType, 3>;
  NormImageReadAccess accNorm(p->GetNormalizationImage(currentType)); 

  m_Smoother.Initialize(p->GetSmoothingStrategy(), p->GetSmoothingHalfWindowSize());
  m_BaselineSubtractor.Initialize(p->GetBaselineCorrectionStrategy(), p->GetBaseLineCorrectionHalfWindowSize());
  m_Transformer.Initialize(p->GetIntensityTransformationStrategy());

  auto &spectra = p->GetSpectra();

  m2::Process::Map(
    spectra.size(),
    threads,
    [&](unsigned int t, unsigned int a, unsigned int b)
    {
      std::vector<IntensityType> ints(mzs.size(), 0);
      std::vector<IntensityType> baseline(mzs.size(), 0);
      std::ifstream f(p->GetBinaryDataPath(), std::ifstream::binary);

      for (unsigned long int i = a; i < b; i++)
      {
        auto &spectrum = spectra[i];

        // Read data from file ------------
        binaryDataToVector(f, spectrum.intOffset, spectrum.intLength, ints.data());
        const auto nFac = accNorm.GetPixelByIndex(spectrum.index);
        
        std::transform(std::begin(ints),
                       std::end(ints),
                       std::begin(ints),
                       [&nFac](const auto &a) { return a / nFac; });
        
        m_Smoother(std::begin(ints), std::end(ints));
        m_BaselineSubtractor(std::begin(ints), std::end(ints), std::begin(baseline));
        m_Transformer(std::begin(ints), std::end(ints));

        std::transform(std::begin(ints), std::end(ints), sumT.at(t).begin(), sumT.at(t).begin(), plus);
        std::transform(std::begin(ints), std::end(ints), skylineT.at(t).begin(), skylineT.at(t).begin(), Maximum);
      }
    });

  auto &skyline = p->GetSkylineSpectrum();
  skyline.clear();
  skyline.resize(mzs.size(), 0);
  for (unsigned int t = 0; t < threads; ++t)
    std::transform(skylineT[t].begin(), skylineT[t].end(), skyline.begin(), skyline.begin(), Maximum);

  auto &mean = p->GetMeanSpectrum();
  auto &sum = p->GetSumSpectrum();

  mean.clear();
  mean.resize(mzs.size(), 0);

  sum.clear();
  sum.resize(mzs.size(), 0);


  auto N = p->GetSpectra().size();

  for (unsigned int t = 0; t < threads; ++t)
    std::transform(sumT[t].begin(), sumT[t].end(), sum.begin(), sum.begin(), plus);
  std::transform(sum.begin(), sum.end(), mean.begin(), [&](auto &a) { return a / double(N); });
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImageSource<MassAxisType, IntensityType>::InitializeImageAccessContinuousCentroid()
{
  std::vector<MassAxisType> mzs;

  { // load continuous x axis
    const auto &spectra = p->GetSpectra();
    mzs.resize(spectra[0].mzLength);

    std::ifstream f(p->GetBinaryDataPath(), std::ios::binary);
    binaryDataToVector(f, spectra[0].mzOffset, spectra[0].mzLength, mzs.data());

    auto &massAxis = p->GetXAxis();
    massAxis.clear();
    std::copy(std::begin(mzs), std::end(mzs), std::back_inserter(massAxis));
    p->SetPropertyValue<unsigned>("m2aia.xs.n", mzs.size());
    p->SetPropertyValue<double>("m2aia.xs.min", mzs.front());
    p->SetPropertyValue<double>("m2aia.xs.max", mzs.back());
  }

  // each thread pixelwise accumulate peaks
  std::vector<std::vector<m2::Interval>> peaksT(p->GetNumberOfThreads());
  for (auto &peaks : peaksT)
    peaks.resize(mzs.size());

  // initialize normalization image accessor
  auto currentType = p->GetNormalizationStrategy();
  using NormImageReadAccess = mitk::ImagePixelReadAccessor<NormImagePixelType, 3>;
  NormImageReadAccess accNorm(p->GetNormalizationImage(currentType)); 

  auto &spectra = p->GetSpectra();

  m2::Process::Map(spectra.size(),
                   p->GetNumberOfThreads(),
                   [&](unsigned int t, unsigned int a, unsigned int b)
                   {
                     std::ifstream f;
                     f.open(p->GetBinaryDataPath(), std::ios::binary);

                     std::vector<IntensityType> ints;
                     auto iO = spectra[0].intOffset;
                     auto iL = spectra[0].intLength;
                     ints.resize(iL);

                     for (unsigned i = a; i < b; i++)
                     {
                       auto &spectrum = spectra[i];
                       iO = spectrum.intOffset;
                       binaryDataToVector(f, iO, iL, ints.data());

                      auto nFac = accNorm.GetPixelByIndex(spectrum.index);
                      std::transform(std::begin(ints),
                                    std::end(ints),
                                    std::begin(ints),
                                    [&nFac](auto &v) { return v / nFac; });
                       

                       for (size_t i = 0; i < mzs.size(); ++i)
                       {
                         //  peaksT[t][i].index(i);
                         peaksT[t][i].x(mzs[i]);
                         peaksT[t][i].y(ints[i]);
                       }
                     }

                     f.close();
                   });

  auto &skyline = p->GetSkylineSpectrum();
  auto &sum = p->GetSumSpectrum();
  auto &mean = p->GetMeanSpectrum();
  skyline.clear();
  sum.clear();
  mean.clear();

  // merge all peaks
  std::vector<m2::Interval> finalPeaks(mzs.size());
  for (auto &peaks : peaksT)
    for (size_t i = 0; i < peaks.size(); ++i)
    {
      finalPeaks[i] += peaks[i];
    }

  for (const auto &peak : finalPeaks)
  {
    skyline.push_back(peak.y.max());
    sum.push_back(peak.y.sum());
    mean.push_back(peak.y.mean());
  }
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImageSource<MassAxisType, IntensityType>::InitializeImageAccessProcessedCentroid()
{
  // MITK_INFO("m2::ImzMLSpectrumImage") << "Start InitializeImageAccessProcessedCentroid";
  InitializeImageAccessProcessedData();
}

template <class MassAxisType, class IntensityType>
void m2::ImzMLSpectrumImageSource<MassAxisType, IntensityType>::InitializeImageAccessProcessedData()
{
  std::vector<std::list<m2::Interval>> peaksT(p->GetNumberOfThreads());


  int binsN;
  if(auto *preferencesService = mitk::CoreServices::GetPreferencesService())
    if(auto *preferences = preferencesService->GetSystemPreferences())
      binsN = preferences->GetInt("m2aia.view.spectrum.bins", 1500);

  auto &spectra = p->GetSpectra();
  const auto &T = p->GetNumberOfThreads();
  std::vector<double> xMin(T, std::numeric_limits<double>::max());
  std::vector<double> xMax(T, std::numeric_limits<double>::min());

  std::vector<std::vector<double>> yT(T, std::vector<double>(binsN, 0));
  std::vector<std::vector<double>> yMaxT(T, std::vector<double>(binsN, 0));
  std::vector<std::vector<unsigned int>> hT(T, std::vector<unsigned int>(binsN, 0));
  std::vector<std::vector<double>> xT(T, std::vector<double>(binsN, 0));

  // initialize normalization image accessor
  auto currentType = p->GetNormalizationStrategy();
  using NormImageReadAccess = mitk::ImagePixelReadAccessor<NormImagePixelType, 3>;
  NormImageReadAccess accNorm(p->GetNormalizationImage(currentType)); 
  
  // Find min max x values
  m2::Process::Map(spectra.size(),
                   T,
                   [&](unsigned int t, unsigned int a, unsigned int b)
                   {
                     std::ifstream f(p->GetBinaryDataPath(), std::ios::binary);
                     std::vector<MassAxisType> mzs;
                     std::list<m2::Interval> peaks, tempList;
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
                     std::ifstream f(p->GetBinaryDataPath(), std::ios::binary);
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
                       double nFac = accNorm.GetPixelByIndex(spectrum.index); 
                        
                        std::transform(std::begin(ints),
                                      std::end(ints),
                                      std::begin(ints),
                                      [&nFac](auto &v) { return v / nFac; });
                       

                       for (unsigned int k = 0; k < mzs.size(); ++k)
                       {
                         // find index of the bin for the k'th m/z value of the pixel
                         auto j = (long)((mzs[k] - min) / binSize);

                         if (j >= binsN)
                           j = binsN - 1;
                         else if (j < 0)
                           j = 0;

                         xT[t][j] += mzs[k];                                   // mass sum
                         yT[t][j] += ints[k] < 10e-256 ? 0 : ints[k];          // intensitiy sum
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
  //     MITK_INFO << xT[0][i]/(double)hT[0][i] << " " <<  yT[0][i] << " max(" << yMaxT[0][i] << ") [" << hT[0][i] <<
  //     "]";
  //   }
  // }

  auto &mzAxis = p->GetXAxis();
  auto &sum = p->GetSumSpectrum();
  auto &mean = p->GetMeanSpectrum();
  auto &skyline = p->GetSkylineSpectrum();

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

  p->SetPropertyValue<double>("m2aia.xs.min", mzAxis.front());
  p->SetPropertyValue<double>("m2aia.xs.max", mzAxis.back());
  p->SetPropertyValue<unsigned>("m2aia.xs.n", mzAxis.size());
}

template <class MassAxisType, class IntensityType>
template <class OutputType>
void m2::ImzMLSpectrumImageSource<MassAxisType, IntensityType>::GetXValues(unsigned int id, std::vector<OutputType> &xd)
{
  std::ifstream f(p->GetBinaryDataPath(), std::ios::binary);

  const auto &spectrum = p->GetSpectra()[id];
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
void m2::ImzMLSpectrumImageSource<MassAxisType, IntensityType>::GetYValues(unsigned int id, std::vector<OutputType> &yd)
{
  std::ifstream f(p->GetBinaryDataPath(), std::ios::binary);

  const auto &spectrum = p->GetSpectra()[id];
  const auto &length = spectrum.intLength;
  const auto &offset = spectrum.intOffset;

  mitk::ImagePixelReadAccessor<m2::NormImagePixelType, 3> normAccess(p->GetNormalizationImage());

  {
    std::vector<IntensityType> ys;
    ys.resize(length);
    binaryDataToVector(f, offset, length, ys.data());
    if (p->GetNormalizationStrategy() != m2::NormalizationStrategyType::None)
    { // check if it is not NormalizationStrategy::None.
      IntensityType norm = normAccess.GetPixelByIndex(spectrum.index);
      std::transform(std::begin(ys), std::end(ys), std::begin(ys), [&norm](auto &v) { return v / norm; });
    }

    // ----- Smoothing
    m_Smoother(std::begin(ys), std::end(ys));

    // ----- Baseline Substraction
    std::vector<IntensityType> baseline(length);
    m_BaselineSubtractor(std::begin(ys), std::end(ys), std::begin(baseline));

    // ----- Intensity Transformation
    m_Transformer(std::begin(ys), std::end(ys));

    // copy and convert
    yd.resize(length);
    std::copy(std::begin(ys), std::end(ys), std::begin(yd));
  }
}
