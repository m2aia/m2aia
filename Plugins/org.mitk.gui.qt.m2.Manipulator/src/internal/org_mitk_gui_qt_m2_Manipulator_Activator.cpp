/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/


#include "org_mitk_gui_qt_m2_Manipulator_Activator.h"

#include "m2Manipulator.h"

ctkPluginContext* org_mitk_gui_qt_m2_Manipulator_Activator::m_Context = nullptr;

void org_mitk_gui_qt_m2_Manipulator_Activator::start(
    ctkPluginContext* context)
{
	BERRY_REGISTER_EXTENSION_CLASS(m2Manipulator, context)

	m_Context = context;
}

void org_mitk_gui_qt_m2_Manipulator_Activator::stop(
    ctkPluginContext* context)
{
	Q_UNUSED(context)

	m_Context = nullptr;
}

ctkPluginContext* org_mitk_gui_qt_m2_Manipulator_Activator::GetContext()
{
	return m_Context;
}
