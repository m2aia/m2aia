/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "PluginActivator.h"
#include "QmitkMoleculaRView.h"
#include <mitkDockerImageManager.h>
#include <usModuleInitialization.h>

US_INITIALIZE_MODULE

void PluginActivator::start(ctkPluginContext* context)
{
  BERRY_REGISTER_EXTENSION_CLASS(QmitkMoleculaRView, context)
  
  // Register the molecular Docker image to plugin storage
  mitk::DockerImageManager imageManager;
  imageManager.LoadFromPreferences();
  
  mitk::DockerImageManager::DockerImage molecularImage(
    "ghcr.io/m2aia/molecular",
    "latest",
    "https://github.com/m2aia/molecular",
    "MoleculaR - Molecular spatial mapping analysis tool"
  );
  
  if (!imageManager.HasImage(molecularImage.imageName))
  {
    imageManager.AddImage(molecularImage, true); // true = plugin image
    imageManager.SaveToPreferences();
  }
}

void PluginActivator::stop(ctkPluginContext*)
{
}
