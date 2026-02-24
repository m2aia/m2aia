/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "PluginActivator.h"
#include "QmitkVolcanoView.h"
#include <mitkDockerImageManager.h>
#include <usModuleInitialization.h>

US_INITIALIZE_MODULE

void PluginActivator::start(ctkPluginContext* context)
{
  BERRY_REGISTER_EXTENSION_CLASS(QmitkVolcanoView, context)
  
  // Register the Volcano Docker image to plugin storage
  mitk::DockerImageManager imageManager;
  imageManager.LoadFromPreferences();
  
  mitk::DockerImageManager::DockerImage volcanoImage(
    "ghcr.io/cemos-mannheim/volcanodocker",
    "main",
    "https://github.com/cemos-mannheim/volcanodocker",
    "Volcano Analysis - Region-based differential expression for MSI data"
  );
  
  if (!imageManager.HasImage(volcanoImage.imageName))
  {
    imageManager.AddImage(volcanoImage, true); // true = plugin image
    imageManager.SaveToPreferences();
  }
}

void PluginActivator::stop(ctkPluginContext*)
{
}
