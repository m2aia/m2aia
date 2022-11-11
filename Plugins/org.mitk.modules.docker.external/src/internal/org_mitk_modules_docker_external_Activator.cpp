/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/


#include "org_mitk_modules_docker_external_Activator.h"
#include "ExternalDockerModules.h"

namespace mitk
{
  void org_mitk_modules_docker_external_Activator::start(ctkPluginContext *context)
  {
    BERRY_REGISTER_EXTENSION_CLASS(ExternalDockerModules, context)
  }

  void org_mitk_modules_docker_external_Activator::stop(ctkPluginContext *context) { Q_UNUSED(context) }
}
