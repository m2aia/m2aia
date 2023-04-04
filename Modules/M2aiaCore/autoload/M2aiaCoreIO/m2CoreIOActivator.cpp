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
#include <m2MicroscopyTiffImageIO.h>
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
      m_FileIOs.push_back(new MicroscopyTiffImageIO());
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



US_EXPORT_MODULE_ACTIVATOR(m2::CoreIOActivator)
