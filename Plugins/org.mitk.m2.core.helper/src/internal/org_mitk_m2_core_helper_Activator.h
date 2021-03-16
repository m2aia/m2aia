/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef org_mitk_m2_core_helper_Activator_h
#define org_mitk_m2_core_helper_Activator_h

#include <berryAbstractUICTKPlugin.h>
#include <m2SelectionT.hxx>
#include <m2SpectrumImageBase.h>

class org_mitk_m2_core_helper_Activator :
    public berry::AbstractUICTKPlugin
{
    Q_OBJECT
        Q_PLUGIN_METADATA(IID "org_mitk_m2_core_helper")
        Q_INTERFACES(ctkPluginActivator)

public:

	m2::SelectionT<m2::IonImageReference::Pointer>::Pointer IonImageReferenceSelection;

    org_mitk_m2_core_helper_Activator();
    ~org_mitk_m2_core_helper_Activator() override;

    void start(ctkPluginContext* context) override;
    void stop(ctkPluginContext* context) override;

    static org_mitk_m2_core_helper_Activator* getDefault();

    static ctkPluginContext* getContext();

private:

    static ctkPluginContext* m_Context;
    static org_mitk_m2_core_helper_Activator* m_Instance;

}; // org_mitk_gui_qt_algorithmcontrol_Activator

#endif // org_mitk_gui_qt_algorithmcontrol_Activator_h
