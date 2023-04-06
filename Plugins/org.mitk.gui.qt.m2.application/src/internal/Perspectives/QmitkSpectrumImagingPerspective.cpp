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
  
  auto folder = layout->CreateFolder("right", berry::IPageLayout::RIGHT, 0.75f, editorArea);
  folder->AddView("org.mitk.views.m2.Data");
  folder->AddView("org.mitk.views.viewnavigator");

  layout->AddStandaloneView("org.mitk.views.m2.Spectrum", false, berry::IPageLayout::BOTTOM, 0.68f, editorArea);
  
  folder = layout->CreateFolder("left", berry::IPageLayout::LEFT, 0.29f, editorArea);
  folder->AddView("org.mitk.views.datamanager");
  
  
  folder = layout->CreateFolder("bottomleft", berry::IPageLayout::BOTTOM, 0.72f, "left");
  folder->AddView("org.mitk.views.imagenavigator");
  folder->AddView("org.mitk.views.pixelvalue");


  auto lo = layout->GetViewLayout("org.mitk.views.datamanager");
  lo->SetCloseable(false);
  
  lo = layout->GetViewLayout("org.mitk.views.m2.Data");
  lo->SetCloseable(false);

  lo = layout->GetViewLayout("org.mitk.views.viewnavigator");
  lo->SetCloseable(false);

  lo = layout->GetViewLayout("org.mitk.views.m2.Spectrum");
  lo->SetCloseable(false);

}