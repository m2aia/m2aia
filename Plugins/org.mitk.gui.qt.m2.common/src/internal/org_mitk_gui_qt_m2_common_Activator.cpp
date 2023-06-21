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

#include "m2BrowserPreferencesPage.h"
#include <QmitkNodeDescriptorManager.h>

#include <m2UIUtils.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateOr.h>
#include <mitkNodePredicateProperty.h>

// #include "QmitkDataNodeColorAction.h"
#include "QmitkDataNodePlotColorAction.h"
#include "QmitkDataNodeColorMapAction.h"
#include "QmitkDataNodeTextureInterpolationAction.h"
#include "QmitkDataNodeExportComponentAction.h"
#include "QmitkDataNodeConvertPixelTypeAction.h"



void org_mitk_gui_qt_m2_common_Activator::start(ctkPluginContext *context)
{
  BERRY_REGISTER_EXTENSION_CLASS(m2BrowserPreferencesPage, context)
  // BERRY_REGISTER_EXTENSION_CLASS(QmitkDataNodeExportComponentActionProvider, context)
  // BERRY_REGISTER_EXTENSION_CLASS(QmitkDataNodeColorMapActionProvider, context)
  // BERRY_REGISTER_EXTENSION_CLASS(QmitkDataNodeColorActionProvider, context)
  // BERRY_REGISTER_EXTENSION_CLASS(QmitkDataNodeTextureInterpolationActionProvider, context)
  // BERRY_REGISTER_EXTENSION_CLASS(QmitkDataNodeConvertPixelTypeActionProvider, context)

  auto descriptorManager = QmitkNodeDescriptorManager::GetInstance();

  auto imageDataType = mitk::TNodePredicateDataType<mitk::Image>::New();
  auto a = mitk::NodePredicateDataType::New("SpectrumImageStack");
  auto b = mitk::NodePredicateDataType::New("ImzMLSpectrumImage");
  auto spectrumImageDataType = mitk::NodePredicateOr::New(a, b);
  auto spectrumImageDescriptorPredicate = mitk::NodePredicateAnd::New(spectrumImageDataType, imageDataType);
  
  
  descriptorManager->AddDescriptor(new QmitkNodeDescriptor(
    tr("SpectrumImage"), QString(":/QmitkM2aiaCore/SpectrumImage_48.png"), spectrumImageDescriptorPredicate, this));

  auto desc = descriptorManager->GetDescriptor("SpectrumImage");
  desc->AddAction(new QmitkDataNodeConvertPixelTypeAction(nullptr,nullptr), false);
  desc->AddAction(new QmitkDataNodeColorMapAction(nullptr,nullptr), false);
  desc->AddAction(new QmitkDataNodeTextureInterpolationAction(nullptr,nullptr), false);
  // desc->AddAction(new QmitkDataNodeColorAction(nullptr,nullptr), false);
  desc->AddAction(new QmitkDataNodePlotColorAction(nullptr,nullptr), false);

  desc = descriptorManager->GetDescriptor("MultiComponentImage");
  desc->AddAction(new QmitkDataNodeExportComponentAction(nullptr,nullptr), false);


}

void org_mitk_gui_qt_m2_common_Activator::stop(ctkPluginContext *)
{
  m2::UIUtils::Instance()->disconnect();
}
