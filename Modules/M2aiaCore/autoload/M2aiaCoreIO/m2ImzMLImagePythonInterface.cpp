/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <M2aiaCoreIOExports.h>
#include <itksys/SystemTools.hxx>
#include <m2ImzMLImageIO.h>
#include <m2ImzMLSpectrumImage.h>
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
      m2::ImzMLSpectrumImage::Pointer m_Image;
      std::ifstream m_SpectrumIOFileHandle;
    };

  } // namespace sys

} // namespace m2

extern "C"
{
  M2AIACOREIO_EXPORT void GetImageArrayFloat32(m2::sys::ImageHandle *handle, double mz, double tol, float *data)
  {
    handle->m_Image->GetImage(mz, tol, nullptr, handle->m_Image);
    auto w = handle->m_Image->GetDimension(0);
    auto h = handle->m_Image->GetDimension(1);
    mitk::ImageReadAccessor acc(handle->m_Image);
    std::copy((m2::DisplayImagePixelType *)(acc.GetData()),
              (m2::DisplayImagePixelType *)(acc.GetData()) + w * h,
              data);
  }

  M2AIACOREIO_EXPORT void GetImageArrayFloat64(m2::sys::ImageHandle *handle, double mz, double tol, double *data)
  {
    handle->m_Image->GetImage(mz, tol, nullptr, handle->m_Image);
    auto w = handle->m_Image->GetDimension(0);
    auto h = handle->m_Image->GetDimension(1);
    mitk::ImageReadAccessor acc(handle->m_Image);
    std::copy((m2::DisplayImagePixelType *)(acc.GetData()),
              (m2::DisplayImagePixelType *)(acc.GetData()) + w * h,
              data);
  }

  M2AIACOREIO_EXPORT unsigned int GetSpectrumType(m2::sys::ImageHandle *handle)
  {
    return (unsigned int)(handle->m_Image->GetSpectrumType().Format);
  }

  M2AIACOREIO_EXPORT unsigned int GetSpectrumDepth(m2::sys::ImageHandle *handle)
  {
    return (unsigned int)(handle->m_Image->GetXAxis().size());
  }

  M2AIACOREIO_EXPORT void GetXAxis(m2::sys::ImageHandle *handle, double *data)
  {
    auto &xAxis = handle->m_Image->GetXAxis();
    std::copy(xAxis.begin(), xAxis.end(), data);
  }

  M2AIACOREIO_EXPORT unsigned int GetYDataTypeSizeInBytes(m2::sys::ImageHandle *handle)
  {
    auto id = handle->m_Image->GetSpectrumType().YAxisType;
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

  M2AIACOREIO_EXPORT void GetSpectrum(m2::sys::ImageHandle *handle, unsigned int id, float *xd, float *yd)
  {
    std::vector<float> xs, ys;
    handle->m_Image->GetSpectrum(id, xs, ys);
    std::copy(xs.begin(), xs.end(), xd);
    std::copy(ys.begin(), ys.end(), yd);
  }

  M2AIACOREIO_EXPORT void GetPixelPosition(m2::sys::ImageHandle *handle, unsigned int spectrumId, int *pixelPosition)
  {
    auto &spectrum = handle->m_Image->GetImzMLSpectrumImageSource().m_Spectra[spectrumId];
    auto *p = spectrum.index.GetIndex();
    std::copy(p, p + 3, pixelPosition);
  }

  M2AIACOREIO_EXPORT unsigned int GetNumberOfSpectra(m2::sys::ImageHandle *handle)
  {
    return handle->m_Image->GetImzMLSpectrumImageSource().m_Spectra.size();
  }

  M2AIACOREIO_EXPORT void GetMeanSpectrum(m2::sys::ImageHandle *handle, double *data)
  {
    auto &yMean = handle->m_Image->MeanSpectrum();
    std::copy(yMean.begin(), yMean.end(), data);
  }

  M2AIACOREIO_EXPORT void GetMaxSpectrum(m2::sys::ImageHandle *handle, double *data)
  {
    auto &yMean = handle->m_Image->SkylineSpectrum();
    std::copy(yMean.begin(), yMean.end(), data);
  }

  M2AIACOREIO_EXPORT void GetSize(m2::sys::ImageHandle *handle, uint32_t *data)
  {
    auto p = handle->m_Image->GetDimensions();
    std::copy(p, p + 3, data);
  }

  M2AIACOREIO_EXPORT void GetSpacing(m2::sys::ImageHandle *handle, double *data)
  {
    auto v = handle->m_Image->GetGeometry()->GetSpacing();
    auto p = v.GetDataPointer();
    std::copy(p, p + 3, data);
  }

  M2AIACOREIO_EXPORT void GetOrigin(m2::sys::ImageHandle *handle, double *data)
  {
    auto v = handle->m_Image->GetGeometry()->GetOrigin();
    auto p = v.GetDataPointer();
    std::copy(p, p + 3, data);
  }

  M2AIACOREIO_EXPORT m2::sys::ImageHandle *CreateImageHandle(const char *path, const char *paramsPath, bool writeDummy)
  {
    auto ifs = std::ifstream(paramsPath);
    auto params = std::string(std::istreambuf_iterator<char>{ifs}, {});
    using namespace std::string_literals;

    std::map<std::string, std::string> pMap;
    const auto bsc_s = m2::Find(params, "baseline-correction", "None"s, pMap);
    const auto bsc_hw = m2::Find(params, "baseline-correction-hw", int(50), pMap);
    const auto sm_s = m2::Find(params, "smoothing", "None"s, pMap);
    const auto sm_hw = m2::Find(params, "smoothing-hw", int(2), pMap);
    const auto norm = m2::Find(params, "normalization", "None"s, pMap);
    const auto pool = m2::Find(params, "pooling", "Maximum"s, pMap);
    const auto tol = m2::Find(params, "tolerance", double(0), pMap);
    const auto threads = m2::Find(params, "threads", int(itk::MultiThreader::GetGlobalDefaultNumberOfThreads()), pMap);

    for (auto &p : pMap)
    {
      MITK_INFO << p.first << " " << p.second;
    }

    if (writeDummy)
    {
      using namespace itksys;
      auto cwd = SystemTools::GetCurrentWorkingDirectory();
      auto path = SystemTools::ConvertToOutputPath(SystemTools::JoinPath({cwd, "/m2PeakPicking.txt.sample"}));
      std::ofstream ofs(path);
      for (auto kv : pMap)
      {
        ofs << "(" << kv.first << " " << kv.second << ")\n";
      }
      MITK_INFO << "A dummy parameter file was written to " << path;
      return nullptr;
    }

    m2::ImzMLImageIO io;
    mitk::AbstractFileReader *reader = &io;
    reader->SetInput(std::string(path));
    auto data = io.DoRead();
    auto storage = new m2::sys::ImageHandle();
    auto sImage = storage->m_Image = dynamic_cast<m2::ImzMLSpectrumImage *>(data.front().GetPointer());

    sImage->SetBaselineCorrectionStrategy(static_cast<m2::BaselineCorrectionType>(m2::BASECOR_MAPPINGS.at(bsc_s)));
    sImage->SetBaseLineCorrectionHalfWindowSize(bsc_hw);

    sImage->SetSmoothingStrategy(static_cast<m2::SmoothingType>(m2::SMOOTHING_MAPPINGS.at(sm_s)));
    sImage->SetSmoothingHalfWindowSize(sm_hw);

    sImage->SetNormalizationStrategy(static_cast<m2::NormalizationStrategyType>(m2::NORMALIZATION_MAPPINGS.at(norm)));
    sImage->SetRangePoolingStrategy(static_cast<m2::RangePoolingStrategyType>(m2::POOLING_MAPPINGS.at(pool)));

    sImage->SetNumberOfThreads(threads);
    sImage->InitializeImageAccess();
    sImage->SetTolerance(tol);
    const auto &source = sImage->GetImzMLSpectrumImageSourceList().front();
    storage->m_SpectrumIOFileHandle.open(source.m_BinaryDataPath, std::ios::binary);

    return storage;
  }

  M2AIACOREIO_EXPORT void DestroyImageHandle(m2::sys::ImageHandle *p) { delete p; }
}