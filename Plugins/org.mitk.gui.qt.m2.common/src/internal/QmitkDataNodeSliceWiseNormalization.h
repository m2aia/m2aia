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
#include <QWidgetAction>


class QmitkDataNodeSliceWiseNormalization : public QWidgetAction, public QmitkAbstractDataNodeAction
{
  Q_OBJECT

public:

  QmitkDataNodeSliceWiseNormalization(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchPartSite);
  QmitkDataNodeSliceWiseNormalization(QWidget* parent = nullptr, berry::IWorkbenchPartSite* workbenchPartSite = nullptr);
  
protected:

  void InitializeAction() override;
  void InitializeWithDataNode(const mitk::DataNode*) override;

};

