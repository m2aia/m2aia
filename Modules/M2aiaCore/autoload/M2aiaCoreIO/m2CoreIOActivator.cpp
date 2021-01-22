/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <usGetModuleContext.h>
#include <usModule.h>
#include <usModuleActivator.h>
#include <usModuleContext.h>

#include <m2ImzMLImageIO.h>

#include <m2CoreObjectFactory.h>


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
		//m_FileIOs.push_back(new IACovisImageIO());
		m_FileIOs.push_back(new ImzMLImageIO());
    }
    void Unload(us::ModuleContext *) override
    {
      for (auto &elem : m_FileIOs)
      {
        delete elem;
      }
    }
  };
}

US_EXPORT_MODULE_ACTIVATOR(m2::CoreIOActivator)
