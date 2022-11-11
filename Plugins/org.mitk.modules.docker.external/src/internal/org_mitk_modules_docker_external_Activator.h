/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#ifndef org_mitk_modules_docker_external_Activator_h
#define org_mitk_modules_docker_external_Activator_h

#include <ctkPluginActivator.h>

namespace mitk
{
  class org_mitk_modules_docker_external_Activator : public QObject, public ctkPluginActivator
  {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org_mitk_modules_docker_external")
    Q_INTERFACES(ctkPluginActivator)

  public:
    void start(ctkPluginContext *context);
    void stop(ctkPluginContext *context);

  }; // org_mitk_modules_docker_external_Activator
}

#endif // org_mitk_modules_docker_external_Activator_h
