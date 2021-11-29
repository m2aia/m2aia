/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "QmitkM2aiaApplicationPlugin.h"
#include "src/QmitkM2aiaAppApplication.h"

#include "src/internal/Perspectives/QmitkSpectrumImagingPerspective.h"
//#include "src/internal/QmitkM2aiaAppIntroPart.h"

#include <mitkVersion.h>
#include <mitkLogMacros.h>

#include <service/cm/ctkConfigurationAdmin.h>
#include <service/cm/ctkConfiguration.h>

#include <QFileInfo>
#include <QDateTime>

QmitkM2aiaApplicationPlugin* QmitkM2aiaApplicationPlugin::inst = nullptr;

QmitkM2aiaApplicationPlugin::QmitkM2aiaApplicationPlugin()
{
  inst = this;
}

QmitkM2aiaApplicationPlugin::~QmitkM2aiaApplicationPlugin()
{
}

QmitkM2aiaApplicationPlugin* QmitkM2aiaApplicationPlugin::GetDefault()
{
  return inst;
}

void QmitkM2aiaApplicationPlugin::start(ctkPluginContext* context)
{
  berry::AbstractUICTKPlugin::start(context);

  this->context = context;

  BERRY_REGISTER_EXTENSION_CLASS(QmitkSpectrumImagingPerspective, context)
  BERRY_REGISTER_EXTENSION_CLASS(QmitkM2aiaAppApplication, context)



  ctkServiceReference cmRef = context->getServiceReference<ctkConfigurationAdmin>();
  ctkConfigurationAdmin* configAdmin = nullptr;
  if (cmRef)
  {
    configAdmin = context->getService<ctkConfigurationAdmin>(cmRef);
  }

  // Use the CTK Configuration Admin service to configure the BlueBerry help system

  if (configAdmin)
  {
    ctkConfigurationPtr conf = configAdmin->getConfiguration("org.blueberry.services.help", QString());
    ctkDictionary helpProps;
    helpProps.insert("homePage", "qthelp://org.mitk.qt.m2.application/bundle/index.html");
    conf->update(helpProps);
    context->ungetService(cmRef);
  }
  else
  {
    MITK_WARN << "Configuration Admin service unavailable, cannot set home page url.";
  }
}

ctkPluginContext* QmitkM2aiaApplicationPlugin::GetPluginContext() const
{
  return context;
}
