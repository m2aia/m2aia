/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/


#ifndef QMITKM2aiaAPPLICATIONPLUGIN_H_
#define QMITKM2aiaAPPLICATIONPLUGIN_H_

#include <berryAbstractUICTKPlugin.h>

#include <QString>

class QmitkM2aiaApplicationPlugin : public berry::AbstractUICTKPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "org_mitk_gui_qt_m2_application")
  Q_INTERFACES(ctkPluginActivator)

public:

  QmitkM2aiaApplicationPlugin();
  ~QmitkM2aiaApplicationPlugin();

  static QmitkM2aiaApplicationPlugin* GetDefault();

  ctkPluginContext* GetPluginContext() const;

  void start(ctkPluginContext*) override;

  QString GetQtHelpCollectionFile() const;

private:

  static QmitkM2aiaApplicationPlugin* inst;

  ctkPluginContext* context;
};

#endif /* QMITKM2aiaAPPLICATIONPLUGIN_H_ */
