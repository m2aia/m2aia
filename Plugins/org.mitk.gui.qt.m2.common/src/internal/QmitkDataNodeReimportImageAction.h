
/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#pragma once
#include "QmitkAbstractDataNodeAction.h"

// mitk core
#include <mitkDataNode.h>
#include <mitkImage.h>

// qt
#include <QAction>


class QmitkDataNodeReimportImageAction : public QAction, public QmitkAbstractDataNodeAction
{
  Q_OBJECT

public:

  QmitkDataNodeReimportImageAction(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchPartSite);
  QmitkDataNodeReimportImageAction(QWidget* parent = nullptr, berry::IWorkbenchPartSite* workbenchPartSite = nullptr);
  
protected:

  void InitializeAction() override;
  void InitializeWithDataNode(const mitk::DataNode*) override;

  mitk::DataNode::ConstPointer m_DataNode;


private Q_SLOTS:
  void OnActionChanged();

};

