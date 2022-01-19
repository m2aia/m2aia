/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "QmitkSpectrumImagingPerspective.h"


void QmitkSpectrumImagingPerspective::CreateInitialLayout(berry::IPageLayout::Pointer layout)
{
  /////////////////////////////////////////////////////
  // all di-app perspectives should have the following:
  /////////////////////////////////////////////////////
  QString editorArea = layout->GetEditorArea();
  
  layout->AddView("org.mitk.views.m2.Data", berry::IPageLayout::RIGHT, 0.61f, editorArea);
  auto lo = layout->GetViewLayout("org.mitk.views.m2.Data");
  lo->SetCloseable(false);

  auto folder = layout->GetFolderForView("org.mitk.views.m2.Data");
  folder->AddPlaceholder("org.mitk.views.m2.DataTools");
  folder->AddPlaceholder("org.mitk.views.m2.Reconstruction3D");
  folder->AddPlaceholder("org.mitk.views.m2.PeakPicking");
  folder->AddPlaceholder("org.mitk.views.m2.Registration");
  folder->AddPlaceholder("org.mitk.views.volumevisualization");
  folder->AddPlaceholder("org.mitk.views.viewnavigatorview");

  layout->AddView("org.mitk.views.datamanager", berry::IPageLayout::BOTTOM, 0.61f, "org.mitk.views.m2.Data");
  lo = layout->GetViewLayout("org.mitk.views.datamanager");
  lo->SetCloseable(false);

  layout->AddStandaloneView("org.mitk.views.m2.Spectrum", false, berry::IPageLayout::BOTTOM, 0.61f, editorArea);
  lo = layout->GetViewLayout("org.mitk.views.m2.Spectrum");
  lo->SetCloseable(false);
  
  layout->AddShowViewShortcut("org.mitk.views.m2.Data");
  layout->AddShowViewShortcut("org.mitk.views.m2.Ions");
  layout->AddShowViewShortcut("org.mitk.views.m2.Spectrum");
  layout->AddShowViewShortcut("org.mitk.views.m2.Reconstruction3D");
  layout->AddShowViewShortcut("org.mitk.views.m2.CombineImages");
  layout->AddShowViewShortcut("org.mitk.views.m2.ImzMLExport");
  layout->AddShowViewShortcut("org.mitk.views.m2.PeakPicking");
  layout->AddShowViewShortcut("org.mitk.views.m2.Registration");
  layout->AddShowViewShortcut("org.mitk.gui.qt.matchpoint.evaluator");

}