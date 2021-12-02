/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef org_mitk_gui_qt_m2_common_Activator_h
#define org_mitk_gui_qt_m2_common_Activator_h

#include <berryAbstractUICTKPlugin.h>

class org_mitk_gui_qt_m2_common_Activator :
    public berry::AbstractUICTKPlugin
{
    Q_OBJECT
        Q_PLUGIN_METADATA(IID "org_mitk_gui_qt_m2_common")
        Q_INTERFACES(ctkPluginActivator)

public:

    void start(ctkPluginContext* context) override;
    void stop(ctkPluginContext* context) override;

    

private:

}; // org_mitk_gui_qt_algorithmcontrol_Activator

#endif // org_mitk_gui_qt_algorithmcontrol_Activator_h
