/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "PluginActivator.h"
#include "QmitkSpatialKMeansView.h"
#include <mitkDockerImageManager.h>
#include <usModuleInitialization.h>

US_INITIALIZE_MODULE

void PluginActivator::start(ctkPluginContext* context)
{
  BERRY_REGISTER_EXTENSION_CLASS(QmitkSpatialKMeansView, context)
  
  // Register the Spatial K-Means Docker image to plugin storage
  mitk::DockerImageManager imageManager;
  imageManager.LoadFromPreferences();
  
  mitk::DockerImageManager::DockerImage spatialKMeansImage(
    "ghcr.io/cemos-mannheim/spatialkmeansdocker",
    "latest",
    "https://github.com/cemos-mannheim/spatialkmeansdocker",
    "Spatial K-Means - Spatially-constrained clustering for MSI data"
  );
  
  if (!imageManager.HasImage(spatialKMeansImage.imageName))
  {
    imageManager.AddImage(spatialKMeansImage, true); // true = plugin image
    imageManager.SaveToPreferences();
  }
}

void PluginActivator::stop(ctkPluginContext*)
{
}
