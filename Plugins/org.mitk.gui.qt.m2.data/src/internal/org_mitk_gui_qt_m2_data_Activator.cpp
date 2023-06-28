/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "org_mitk_gui_qt_m2_data_Activator.h"
#include "m2Data.h"
#include "m2DataTools.h"
#include "m2DataStatistics.h"

#include <usModuleInitialization.h>

US_INITIALIZE_MODULE

namespace mitk
{
  void org_mitk_gui_qt_m2_Data_Activator::start(ctkPluginContext *context)
  {
    BERRY_REGISTER_EXTENSION_CLASS(m2Data, context)
    BERRY_REGISTER_EXTENSION_CLASS(m2DataTools, context)
    BERRY_REGISTER_EXTENSION_CLASS(m2DataStatistics, context)

  }

  void org_mitk_gui_qt_m2_Data_Activator::stop(ctkPluginContext *context) { Q_UNUSED(context) }
} // namespace mitk
