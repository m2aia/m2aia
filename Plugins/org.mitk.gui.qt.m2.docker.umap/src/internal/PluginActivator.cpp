/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "PluginActivator.h"
#include "QmitkUMAPView.h"
#include <mitkDockerImageManager.h>
#include <usModuleInitialization.h>

US_INITIALIZE_MODULE

void PluginActivator::start(ctkPluginContext* context)
{
  BERRY_REGISTER_EXTENSION_CLASS(QmitkUMAPView, context)
  
  // Register the UMAP Docker image to plugin storage
  mitk::DockerImageManager imageManager;
  imageManager.LoadFromPreferences();
  
  mitk::DockerImageManager::DockerImage umapImage(
    "ghcr.io/m2aia/umap",
    "latest",
    "https://github.com/m2aia/docker-umap",
    "UMAP - Uniform Manifold Approximation and Projection for dimension reduction"
  );
  
  if (!imageManager.HasImage(umapImage.imageName))
  {
    imageManager.AddImage(umapImage, true);
    imageManager.SaveToPreferences();
  }
}

void PluginActivator::stop(ctkPluginContext*)
{
}
