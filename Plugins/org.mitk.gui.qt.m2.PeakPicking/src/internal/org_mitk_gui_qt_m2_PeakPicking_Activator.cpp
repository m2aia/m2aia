/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/


#include "org_mitk_gui_qt_m2_PeakPicking_Activator.h"
#include "m2PeakPickingView.h"
#include "QmitkExportComponentAction.h"

namespace mitk
{
  void org_mitk_gui_qt_m2_PeakPicking_Activator::start(ctkPluginContext *context)
  {
    BERRY_REGISTER_EXTENSION_CLASS(m2PeakPickingView, context)
    BERRY_REGISTER_EXTENSION_CLASS(QmitkExportComponentAction, context)
  }

  void org_mitk_gui_qt_m2_PeakPicking_Activator::stop(ctkPluginContext *context) { Q_UNUSED(context) }
}
