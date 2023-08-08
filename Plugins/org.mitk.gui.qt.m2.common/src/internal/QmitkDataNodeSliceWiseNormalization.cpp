/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkDataNodeSliceWiseNormalization.h"
#include <m2SpectrumImageStack.h>

#include <QCheckBox>



QmitkDataNodeSliceWiseNormalization::QmitkDataNodeSliceWiseNormalization(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchPartSite): 
  QWidgetAction(parent),
  QmitkAbstractDataNodeAction(workbenchPartSite)
{
  InitializeAction();
}

QmitkDataNodeSliceWiseNormalization::QmitkDataNodeSliceWiseNormalization(QWidget* parent, berry::IWorkbenchPartSite* workbenchPartSite): 
  QWidgetAction(parent),
  QmitkAbstractDataNodeAction(berry::IWorkbenchPartSite::Pointer(workbenchPartSite))
{
  InitializeAction();
}

void QmitkDataNodeSliceWiseNormalization::InitializeAction()
{
  auto checkBox = new QCheckBox(static_cast<QWidget*>(this->parent()));
  checkBox->setText(tr("Slice-wise Maximum Normalization"));
  checkBox->setToolTip(tr("Rescale intensities by dividing by the maximum of each slice."));
  setDefaultWidget(checkBox);
}

void QmitkDataNodeSliceWiseNormalization::InitializeWithDataNode(const mitk::DataNode* node){
  if(auto image = dynamic_cast<m2::SpectrumImageStack *>(node->GetData())){
    auto checkBox = dynamic_cast<QCheckBox *>(defaultWidget());
    checkBox->setChecked(image->GetUseSliceWiseMaximumNormalization());
    connect(checkBox, &QCheckBox::stateChanged, this, [image](int v){
      image->SetUseSliceWiseMaximumNormalization(v);
    });
  }
}


