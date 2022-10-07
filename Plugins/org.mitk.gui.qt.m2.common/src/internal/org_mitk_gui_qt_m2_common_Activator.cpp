/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "org_mitk_gui_qt_m2_common_Activator.h"

#include "QmitkExportComponentAction.h"
#include "m2BrowserPreferencesPage.h"
#include <QmitkNodeDescriptorManager.h>
#include <m2UIUtils.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateOr.h>
#include <mitkNodePredicateProperty.h>

void org_mitk_gui_qt_m2_common_Activator::start(ctkPluginContext *context)
{
  BERRY_REGISTER_EXTENSION_CLASS(m2BrowserPreferencesPage, context)

  auto descriptorManager = QmitkNodeDescriptorManager::GetInstance();

  auto imageDataType = mitk::TNodePredicateDataType<mitk::Image>::New();
  auto a = mitk::NodePredicateDataType::New("SpectrumImageStack");
  auto b = mitk::NodePredicateDataType::New("ImzMLSpectrumImage");
  auto spectrumImageDataType = mitk::NodePredicateOr::New(a, b);
  auto spectrumImageDescriptorPredicate = mitk::NodePredicateAnd::New(spectrumImageDataType, imageDataType);
  descriptorManager->AddDescriptor(new QmitkNodeDescriptor(
    tr("SpectrumImage"), QString(":/QmitkM2aiaCore/SpectrumImage_48.png"), spectrumImageDescriptorPredicate, this));

  auto mcImageDescriptorPredicate =
    mitk::NodePredicateAnd::New(mitk::NodePredicateProperty::New("Image.Displayed Component"), imageDataType);
  descriptorManager->AddDescriptor(new QmitkNodeDescriptor(tr("MultiComponentImage"),
                                                           QString(":/QmitkM2aiaCore/MultiComponentImages_48.png"),
                                                           mcImageDescriptorPredicate,
                                                           this));
}

void org_mitk_gui_qt_m2_common_Activator::stop(ctkPluginContext *)
{
  m2::UIUtils::Instance()->disconnect();
}
