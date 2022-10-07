/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "org_mitk_gui_qt_m2_Data_Activator.h"
#include "m2Data.h"
#include "m2DataTools.h"
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateOr.h>
#include <QmitkNodeDescriptorManager.h>

namespace mitk
{
  void org_mitk_gui_qt_m2_Data_Activator::start(ctkPluginContext *context)
  {
    BERRY_REGISTER_EXTENSION_CLASS(m2Data, context)
    BERRY_REGISTER_EXTENSION_CLASS(m2DataTools, context)

    auto descriptorManager = QmitkNodeDescriptorManager::GetInstance();
    auto a = mitk::NodePredicateDataType::New("SpectrumImageStack");
    auto b = mitk::NodePredicateDataType::New("ImzMLSpectrumImage");
    auto c = mitk::NodePredicateOr::New(a, b);
    auto f = mitk::TNodePredicateDataType<mitk::Image>::New();
    auto g = mitk::NodePredicateAnd::New(c, f);
    descriptorManager->AddDescriptor( new QmitkNodeDescriptor(tr("SpectrumImage"), QString(":/Qmitk/LabelSetImage_48.png"), g, this));



    // Adding "PlanarLine"
    mitk::NodePredicateDataType::Pointer isPlanarLine = mitk::NodePredicateDataType::New("PlanarLine");
    descriptorManager->AddDescriptor(new QmitkNodeDescriptor(QObject::tr("PlanarLine"), QString(":/QtWidgetsExt/PlanarLine_48.png"), isPlanarLine, descriptorManager));
  }

  void org_mitk_gui_qt_m2_Data_Activator::stop(ctkPluginContext *context) { Q_UNUSED(context) }
} // namespace mitk
