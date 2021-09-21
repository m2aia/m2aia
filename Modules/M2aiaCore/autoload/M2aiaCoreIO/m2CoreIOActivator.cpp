/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <itksys/SystemTools.hxx>
#include <m2CoreCommon.h>
#include <m2CoreObjectFactory.h>
#include <m2FSMImageIO.h>
#include <m2ImzMLImageIO.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2OpenSlideIO.h>
#include <usGetModuleContext.h>
#include <usModule.h>
#include <usModuleActivator.h>
#include <usModuleContext.h>

namespace m2
{
  /**
  \brief Registers services for mass spectrometry module.
  */
  class CoreIOActivator : public us::ModuleActivator
  {
    std::vector<mitk::AbstractFileIO *> m_FileIOs;

  public:
    void Load(us::ModuleContext * /*context*/) override
    {
      // m_FileIOs.push_back(new IACovisImageIO());
      m_FileIOs.push_back(new ImzMLImageIO());
      m_FileIOs.push_back(new OpenSlideIO());
      m_FileIOs.push_back(new FSMImageIO());
    }
    void Unload(us::ModuleContext *) override
    {
      for (auto &elem : m_FileIOs)
      {
        delete elem;
      }
    }
  };
} // namespace m2

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
    };

  } // namespace sys
} // namespace m2

extern "C"
{
  M2AIACOREIO_EXPORT void GetImageArray(m2::sys::ImageHandle *handle, double mz, double tol, double *data)
  {
    handle->m_Image->UpdateImage(mz, tol, nullptr, handle->m_Image);
    auto w = handle->m_Image->GetDimension(0);
    auto h = handle->m_Image->GetDimension(1);
    std::copy((m2::DisplayImagePixelType *)(handle->m_Image->GetData()),
              (m2::DisplayImagePixelType *)(handle->m_Image->GetData()) + w * h,
              data);
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

    for(auto& p : pMap){
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

    sImage->SetBaselineCorrectionStrategy(static_cast<m2::BaselineCorrectionType>(m2::SIGNAL_MAPPINGS.at(bsc_s)));
    sImage->SetBaseLineCorrectionHalfWindowSize(bsc_hw);

    sImage->SetSmoothingStrategy(static_cast<m2::SmoothingType>(m2::SIGNAL_MAPPINGS.at(sm_s)));
    sImage->SetSmoothingHalfWindowSize(sm_hw);

    sImage->SetNormalizationStrategy(static_cast<m2::NormalizationStrategyType>(m2::SIGNAL_MAPPINGS.at(norm)));
    sImage->SetRangePoolingStrategy(static_cast<m2::RangePoolingStrategyType>(m2::SIGNAL_MAPPINGS.at(pool)));

    sImage->SetNumberOfThreads(threads);
    sImage->InitializeImageAccess();
    sImage->SetTolerance(tol);

    return storage;
  }

  M2AIACOREIO_EXPORT void DestroyImageHandle(m2::sys::ImageHandle *p) { delete p; }
}

US_EXPORT_MODULE_ACTIVATOR(m2::CoreIOActivator)
