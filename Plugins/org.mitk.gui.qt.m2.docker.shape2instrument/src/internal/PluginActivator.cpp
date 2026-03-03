/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "PluginActivator.h"
#include "QmitkShape2InstrumentView.h"
#include <mitkDockerImageManager.h>
#include <usModuleInitialization.h>

US_INITIALIZE_MODULE

void PluginActivator::start(ctkPluginContext* context)
{
  BERRY_REGISTER_EXTENSION_CLASS(QmitkShape2InstrumentView, context)

  // Register the Shape2Instrument Docker image in the plugin storage
  mitk::DockerImageManager imageManager;
  imageManager.LoadFromPreferences();

  mitk::DockerImageManager::DockerImage s2iImage(
    "ghcr.io/cemos-mannheim/shape2instrument",
    "latest",
    "https://github.com/CeMOS-Mannheim/shape2instrument",
    "Shape2Instrument – converts segmented mask + calibration points to Leica XML or MMI CSV"
  );

  if (!imageManager.HasImage(s2iImage.imageName))
  {
    imageManager.AddImage(s2iImage, true);
    imageManager.SaveToPreferences();
  }
}

void PluginActivator::stop(ctkPluginContext*)
{
}
