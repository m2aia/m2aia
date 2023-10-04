/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkDataNodeSliceWiseNormalizationAction.h"
#include <m2SpectrumImageStack.h>


QmitkDataNodeSliceWiseNormalizationAction::QmitkDataNodeSliceWiseNormalizationAction(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchPartSite): 
  QAction(parent),
  QmitkAbstractDataNodeAction(workbenchPartSite)
{
  InitializeAction();
}

QmitkDataNodeSliceWiseNormalizationAction::QmitkDataNodeSliceWiseNormalizationAction(QWidget* parent, berry::IWorkbenchPartSite* workbenchPartSite): 
  QAction(parent),
  QmitkAbstractDataNodeAction(berry::IWorkbenchPartSite::Pointer(workbenchPartSite))
{
  InitializeAction();
}

void QmitkDataNodeSliceWiseNormalizationAction::InitializeAction()
{
  setCheckable(true);
  setText(tr("Slice-wise Maximum Normalization"));
  setToolTip(tr("Rescale intensities by dividing by the maximum of each slice."));
  
}

void QmitkDataNodeSliceWiseNormalizationAction::InitializeWithDataNode(const mitk::DataNode* node){
  if(auto image = dynamic_cast<m2::SpectrumImageStack *>(node->GetData())){
    setChecked(image->GetUseSliceWiseMaximumNormalization());
    
    connect(this, &QAction::triggered, this, [image](bool v){
      image->SetUseSliceWiseMaximumNormalization(v);
    });
    
  }
}


