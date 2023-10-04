/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <M2aiaCoreExports.h>
#include <itksys/SystemTools.hxx>
#include <m2ImzMLImageIO.h>
#include <m2ImzMLSpectrumImage.h>
#include <mitkCoreServices.h>
#include <mitkIPreferencesService.h>
#include <mitkImageReadAccessor.h>

// PYTHON EXPORT
namespace m2
{
  namespace sys
  {
    // This class holds smartpointer and additional data the handling of an ImzMlMassSpecImage object.
    class ImageHandle
    {
    public:
      m2::ImzMLSpectrumImage::Pointer Image;
    };

  } // namespace sys

} // namespace m2

extern "C"
{
  unsigned int GetNumberOfPixelsInImage(m2::sys::ImageHandle *handle)
  {
    auto dims = handle->Image->GetDimensions();
    auto validDims = handle->Image->GetDimension();
    auto N = std::accumulate(dims, dims + validDims, 1, std::multiplies<>());
    return N;
  }

  M2AIACORE_EXPORT void GetImageArrayFloat32(m2::sys::ImageHandle *handle, double mz, double tol, float *data)
  {
    handle->Image->GetImage(mz, tol, nullptr, handle->Image);
    auto N = GetNumberOfPixelsInImage(handle);
    mitk::ImageReadAccessor acc(handle->Image.GetPointer());
    std::copy((m2::DisplayImagePixelType *)(acc.GetData()), (m2::DisplayImagePixelType *)(acc.GetData()) + N, data);
  }

  M2AIACORE_EXPORT void GetImageArrayFloat64(m2::sys::ImageHandle *handle, double mz, double tol, double *data)
  {
    handle->Image->GetImage(mz, tol, nullptr, handle->Image);
    auto N = GetNumberOfPixelsInImage(handle);
    mitk::ImageReadAccessor acc(handle->Image.GetPointer());
    std::copy((m2::DisplayImagePixelType *)(acc.GetData()), (m2::DisplayImagePixelType *)(acc.GetData()) + N, data);
  }

  M2AIACORE_EXPORT void GetMaskArray(m2::sys::ImageHandle *handle, mitk::Label::PixelType *data)
  {
    auto mask = handle->Image->GetMaskImage();
    auto N = GetNumberOfPixelsInImage(handle);
    mitk::ImageReadAccessor acc(mask);
    std::copy((mitk::Label::PixelType *)(acc.GetData()), (mitk::Label::PixelType *)(acc.GetData()) + N, data);
  }

  M2AIACORE_EXPORT void GetIndexArray(m2::sys::ImageHandle *handle, unsigned int *data)
  {
    auto indexImage = handle->Image->GetIndexImage();
    auto N = GetNumberOfPixelsInImage(handle);
    mitk::ImageReadAccessor acc(indexImage);
    std::copy((unsigned int *)(acc.GetData()), (unsigned int *)(acc.GetData()) + N, data);
  }

  M2AIACORE_EXPORT void GetNormalizationArray(m2::sys::ImageHandle *handle,
                                              const char *typeName,
                                              m2::NormImagePixelType *data)
  {
    auto type = static_cast<m2::NormalizationStrategyType>(m2::NORMALIZATION_MAPPINGS.at(typeName));

    if (!handle->Image->GetNormalizationImageStatus(type))
      handle->Image->InitializeNormalizationImage(type);

    auto normImage = handle->Image->GetNormalizationImage(type);
    auto N = GetNumberOfPixelsInImage(handle);
    mitk::ImageReadAccessor acc(normImage);
    std::copy((m2::NormImagePixelType *)(acc.GetData()), (m2::NormImagePixelType *)(acc.GetData()) + N, data);
  }

  M2AIACORE_EXPORT unsigned int GetSpectrumType(m2::sys::ImageHandle *handle)
  {
    return (unsigned int)(handle->Image->GetSpectrumType().Format);
  }

  M2AIACORE_EXPORT unsigned int GetSizeInBytesOfYAxisType(m2::sys::ImageHandle *handle)
  {
    return m2::to_bytes(handle->Image->GetSpectrumType().YAxisType);
  }

  M2AIACORE_EXPORT void GetXAxis(m2::sys::ImageHandle *handle, double *data)
  {
    auto &xAxis = handle->Image->GetXAxis();
    std::copy(xAxis.begin(), xAxis.end(), data);
  }

  M2AIACORE_EXPORT unsigned int GetXAxisDepth(m2::sys::ImageHandle *handle)
  {
    auto &xAxis = handle->Image->GetXAxis();
    return xAxis.size();
  }

  M2AIACORE_EXPORT float GetTolerance(m2::sys::ImageHandle *handle)
  {
    return handle->Image->GetTolerance();
  }

  M2AIACORE_EXPORT void SetTolerance(m2::sys::ImageHandle *handle, float tol)
  {
    handle->Image->SetTolerance(tol);
  }

  M2AIACORE_EXPORT unsigned int GetYDataTypeSizeInBytes(m2::sys::ImageHandle *handle)
  {
    auto id = handle->Image->GetSpectrumType().YAxisType;
    if (id == m2::NumericType::Float)
    {
      return sizeof(float);
    }
    else if (id == m2::NumericType::Double)
    {
      return sizeof(double);
    }
    else
    {
      return 0;
    }
  }

  M2AIACORE_EXPORT void GetSpectrum(m2::sys::ImageHandle *handle, unsigned int id, float *xd, float *yd)
  {
    std::vector<float> xs, ys;
    handle->Image->GetSpectrumFloat(id, xs, ys);
    std::copy(xs.begin(), xs.end(), xd);
    std::copy(ys.begin(), ys.end(), yd);
  }

  M2AIACORE_EXPORT void GetSpectra(m2::sys::ImageHandle *handle, unsigned int *id, unsigned int N, float *yd)
  {
    std::vector<float> ys;
    for (unsigned int i = 0; i < N; ++i)
    {
      handle->Image->GetIntensitiesFloat(id[i], ys);
      std::copy(ys.begin(), ys.end(), yd + i * ys.size());
    }
  }

  M2AIACORE_EXPORT void GetIntensities(m2::sys::ImageHandle *handle, unsigned int id, float *yd)
  {
    std::vector<float> ys;
    handle->Image->GetIntensitiesFloat(id, ys);
    std::copy(ys.begin(), ys.end(), yd);
  }

  M2AIACORE_EXPORT unsigned int GetSpectrumDepth(m2::sys::ImageHandle *handle, unsigned int id)
  {
    const auto &spectrum = handle->Image->GetSpectra()[id];
    return spectrum.mzLength;
  }

  M2AIACORE_EXPORT void GetSpectrumPosition(m2::sys::ImageHandle *handle,
                                            unsigned int spectrumId,
                                            unsigned int *pixelPosition)
  {
    auto &spectrum = handle->Image->GetSpectra()[spectrumId];
    auto *p = spectrum.index.GetIndex();
    std::copy(p, p + 3, pixelPosition);
  }

  M2AIACORE_EXPORT unsigned int GetNumberOfSpectra(m2::sys::ImageHandle *handle)
  {
    return handle->Image->GetSpectra().size();
  }

  M2AIACORE_EXPORT void GetMeanSpectrum(m2::sys::ImageHandle *handle, double *data)
  {
    auto &yMean = handle->Image->GetMeanSpectrum();
    std::copy(yMean.begin(), yMean.end(), data);
  }

  M2AIACORE_EXPORT void GetMaxSpectrum(m2::sys::ImageHandle *handle, double *data)
  {
    auto &yMean = handle->Image->GetSkylineSpectrum();
    std::copy(yMean.begin(), yMean.end(), data);
  }

  M2AIACORE_EXPORT void GetSize(m2::sys::ImageHandle *handle, uint32_t *data)
  {
    auto p = handle->Image->GetDimensions();
    std::copy(p, p + 3, data);
  }

  M2AIACORE_EXPORT void GetSpacing(m2::sys::ImageHandle *handle, double *data)
  {
    auto v = handle->Image->GetGeometry()->GetSpacing();
    auto p = v.GetDataPointer();
    std::copy(p, p + 3, data);
  }

  M2AIACORE_EXPORT void GetOrigin(m2::sys::ImageHandle *handle, double *data)
  {
    auto v = handle->Image->GetGeometry()->GetOrigin();
    auto p = v.GetDataPointer();
    std::copy(p, p + 3, data);
  }

  M2AIACORE_EXPORT void SetSmoothing(m2::sys::ImageHandle *handle, const char *sm_s, unsigned int hws)
  {
    handle->Image->SetSmoothingStrategy(static_cast<m2::SmoothingType>(m2::SMOOTHING_MAPPINGS.at(sm_s)));
    handle->Image->SetSmoothingHalfWindowSize(hws);
  }

  M2AIACORE_EXPORT void SetBaselineCorrection(m2::sys::ImageHandle *handle, const char *sm_s, unsigned int hws)
  {    
    handle->Image->SetBaselineCorrectionStrategy(
      static_cast<m2::BaselineCorrectionType>(m2::BASECOR_MAPPINGS.at(sm_s)));
    handle->Image->SetBaseLineCorrectionHalfWindowSize(hws);
  }

  M2AIACORE_EXPORT void SetPooling(m2::sys::ImageHandle *handle, const char *sm_s)
  {

    handle->Image->SetRangePoolingStrategy(static_cast<m2::RangePoolingStrategyType>(m2::POOLING_MAPPINGS.at(sm_s)));
  }

  M2AIACORE_EXPORT void SetNormalization(m2::sys::ImageHandle *handle, const char *sm_s)
  {

    handle->Image->SetNormalizationStrategy(
      static_cast<m2::NormalizationStrategyType>(m2::NORMALIZATION_MAPPINGS.at(sm_s)));
  }

  M2AIACORE_EXPORT void SetIntensityTransformation(m2::sys::ImageHandle *handle, const char *sm_s)
  {

    handle->Image->SetIntensityTransformationStrategy(
      static_cast<m2::IntensityTransformationType>(m2::INTENSITYTRANSFORMATION_MAPPINGS.at(sm_s)));
  }

  M2AIACORE_EXPORT char *GetMetaDataDictionary(m2::sys::ImageHandle *handle)
  {
    auto v = handle->Image->GetPropertyList();
    std::string s;
    auto keys = v->GetPropertyKeys();
    for (auto k : keys)
    {
      auto prop = v->GetProperty(k.c_str());
      s += k;
      s += "\t" + prop->GetValueAsString();
      s += "\n";
    }
    auto data = new char[s.length() + 1];

    std::copy(s.begin(), s.end(), data);
    data[s.length()] = '\0';
    return data;
  }

  M2AIACORE_EXPORT void DestroyCharBuffer(std::string *p)
  {
    delete p;
  }

  M2AIACORE_EXPORT void WriteContinuousCentroidImzML(m2::sys::ImageHandle *handle,
                                                     const char *s,
                                                     double *centroids,
                                                     int n)
  {
    m2::ImzMLImageIO io;
    mitk::AbstractFileWriter *writer = &io;
    auto intervals = m2::IntervalVector::New();

    for (int i = 0; i < n; ++i)
      intervals->GetIntervals().push_back(m2::Interval(centroids[i], 0));

    writer->SetInput(handle->Image);
    writer->SetOutputLocation(std::string(s));
    io.SetIntervalVector(intervals);
    io.SetSpectrumFormat(m2::SpectrumFormat::ContinuousCentroid);
    io.Write();
  }

  M2AIACORE_EXPORT void Update(m2::sys::ImageHandle *p)
  {
    // p->Image->InitializeProcessor();
    // p->Image->InitializeGeometry();
    p->Image->InitializeImageAccess();
  }

  M2AIACORE_EXPORT m2::sys::ImageHandle *CreateImageHandle(const char *path)
  {
    if (auto *preferencesService = mitk::CoreServices::GetPreferencesService())
      if (!preferencesService->GetSystemPreferences())
      {
        mitk::CoreServicePointer preferencesService(mitk::CoreServices::GetPreferencesService());
        preferencesService->InitializeStorage(".pym2aia_prefs.xml");
      }

    if (!itksys::SystemTools::FileExists(path))
    {
      MITK_ERROR << "The given path does not exist!\n\t" << std::string(path);
      return nullptr;
    }

    // std::string parameters;
    // if(itksys::SystemTools::PathExists(s) && itksys::SystemTools::FileExists(s)){
    //   auto ifs = std::ifstream(s);
    //   parameters = std::string(std::istreambuf_iterator<char>{ifs}, {});
    // }else{
    //   parameters = s;
    // }

    // using namespace std::string_literals;
    // std::map<std::string, std::string> pMap;
    // const auto bsc_s = m2::Find(parameters, "baseline-correction", "None"s, pMap);
    // const auto bsc_hw = m2::Find(parameters, "baseline-correction-hw", int(50), pMap);
    // const auto sm_s = m2::Find(parameters, "smoothing", "None"s, pMap);
    // const auto sm_hw = m2::Find(parameters, "smoothing-hw", int(2), pMap);
    // const auto norm = m2::Find(parameters, "normalization", "None"s, pMap);
    // const auto pool = m2::Find(parameters, "pooling", "Maximum"s, pMap);
    // const auto tol = m2::Find(parameters, "tolerance", double(0), pMap);
    // const auto intTransform = m2::Find(parameters, "transform", "None"s, pMap);
    // const auto threads = m2::Find(parameters, "threads",
    // int(itk::MultiThreaderBase::GetGlobalDefaultNumberOfThreads()), pMap);

    // MITK_INFO << "---------------------";
    // MITK_INFO << "Image initialization:";
    // MITK_INFO << "---------------------";
    // for (auto &p : pMap)
    //    MITK_INFO << "\t" << p.first << " " << p.second;

    m2::ImzMLImageIO io;
    mitk::AbstractFileReader *reader = &io;
    reader->SetInput(std::string(path));
    // MITK_INFO << "ImzML Path " << path;
    auto data = io.DoRead();
    auto storage = new m2::sys::ImageHandle();
    auto sImage = storage->Image = dynamic_cast<m2::ImzMLSpectrumImage *>(data.front().GetPointer());

    return storage;
  }

  M2AIACORE_EXPORT void DestroyImageHandle(m2::sys::ImageHandle *p)
  {
    delete p;
  }
}