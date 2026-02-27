/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef PluginActivator_h
#define PluginActivator_h

#include <ctkPluginActivator.h>

class PluginActivator : public QObject, public ctkPluginActivator
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "org_mitk_gui_qt_m2_connectedcomponents")
  Q_INTERFACES(ctkPluginActivator)

public:
  void start(ctkPluginContext* context);
  void stop(ctkPluginContext* context);
};

#endif
