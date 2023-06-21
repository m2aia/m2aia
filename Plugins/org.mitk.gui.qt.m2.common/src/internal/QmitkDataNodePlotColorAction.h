/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#pragma once

#include "QmitkAbstractDataNodeAction.h"

// qt
#include <QPushButton>
#include <QWidgetAction>

class QmitkDataNodePlotColorAction : public QWidgetAction, public QmitkAbstractDataNodeAction
{
  Q_OBJECT

public:

  QmitkDataNodePlotColorAction(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchPartSite);
  QmitkDataNodePlotColorAction(QWidget* parent, berry::IWorkbenchPartSite* workbenchPartSite);

  void InitializeWithDataNode(const mitk::DataNode* dataNode) override;

private Q_SLOTS:

  void OnPlotColorChanged();
  void OnMarkerColorChanged();
  void OnActionChanged();

protected:

  void InitializeAction() override;

private:

  QPushButton* m_ColorButton;
  QPushButton* m_ColorButtonMarker;

};
