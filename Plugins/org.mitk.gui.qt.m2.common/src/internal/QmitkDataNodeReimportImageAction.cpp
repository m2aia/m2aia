/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkDataNodeReimportImageAction.h"

#include <QCheckBox>
#include <m2SpectrumImageStack.h>

QmitkDataNodeReimportImageAction::QmitkDataNodeReimportImageAction(QWidget *parent,
                                                                   berry::IWorkbenchPartSite::Pointer workbenchPartSite)
  : QAction(parent), QmitkAbstractDataNodeAction(workbenchPartSite)
{
  InitializeAction();
}

QmitkDataNodeReimportImageAction::QmitkDataNodeReimportImageAction(QWidget *parent,
                                                                   berry::IWorkbenchPartSite *workbenchPartSite)
  : QAction(parent), QmitkAbstractDataNodeAction(berry::IWorkbenchPartSite::Pointer(workbenchPartSite))
{
  InitializeAction();
}

void QmitkDataNodeReimportImageAction::InitializeAction()
{
  setText(tr("Re-import"));
  setToolTip(
    tr("Removes the image from the Data Manager and loads it using the current signal processing selections."));
}

void QmitkDataNodeReimportImageAction::InitializeWithDataNode(const mitk::DataNode *node)
{
  connect(this, &QAction::triggered, this, [this, node](){
      auto ds = m_DataStorage.Lock();
      ds->Remove(ds->GetDerivations(node));
      ds->Remove(node);
      ds->Add(const_cast<mitk::DataNode *>(node));

  });
};
