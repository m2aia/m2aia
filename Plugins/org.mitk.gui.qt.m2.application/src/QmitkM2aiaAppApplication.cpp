/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "QmitkM2aiaAppApplication.h"

#include <berryPlatformUI.h>

#include "QmitkM2aiaAppWorkbenchAdvisor.h"

#include <usModuleInitialization.h>

US_INITIALIZE_MODULE

QVariant QmitkM2aiaAppApplication::Start(berry::IApplicationContext*)
{
  berry::Display* display = berry::PlatformUI::CreateDisplay();

  int code = berry::PlatformUI::CreateAndRunWorkbench(display, new QmitkM2aiaAppWorkbenchAdvisor());

  // exit the application with an appropriate return code
  return code == berry::PlatformUI::RETURN_RESTART
              ? EXIT_RESTART : EXIT_OK;
}

void QmitkM2aiaAppApplication::Stop()
{
  
}
