/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "PluginActivator.h"
#include "QmitkPeakLearningView.h"
#include <usModuleInitialization.h>

US_INITIALIZE_MODULE

void PluginActivator::start(ctkPluginContext* context)
{
  BERRY_REGISTER_EXTENSION_CLASS(QmitkPeakLearningView, context)
}

void PluginActivator::stop(ctkPluginContext*)
{
}
