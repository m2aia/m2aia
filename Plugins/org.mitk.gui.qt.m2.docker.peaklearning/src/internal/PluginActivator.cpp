/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "PluginActivator.h"
#include "QmitkPeakLearningView.h"
#include <mitkDockerImageManager.h>
#include <usModuleInitialization.h>

US_INITIALIZE_MODULE

void PluginActivator::start(ctkPluginContext* context)
{
  BERRY_REGISTER_EXTENSION_CLASS(QmitkPeakLearningView, context)
  
  // Register the MSIPL Docker image to plugin storage
  mitk::DockerImageManager imageManager;
  imageManager.LoadFromPreferences();
  
  mitk::DockerImageManager::DockerImage msiplImage(
    "ghcr.io/m2aia/msipl",
    "latest",
    "https://github.com/m2aia/msipl",
    "MSIPL - Mass spectrometry imaging peak learning"
  );
  
  if (!imageManager.HasImage(msiplImage.imageName))
  {
    imageManager.AddImage(msiplImage, true);
    imageManager.SaveToPreferences();
  }
}

void PluginActivator::stop(ctkPluginContext*)
{
}
