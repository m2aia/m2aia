/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#pragma once

#include <org_mitk_gui_qt_m2_common_Export.h>

#include <mitkIContextMenuActionProvider.h>
#include <QmitkAbstractDataNodeAction.h>
#include <QAction>
#include <mitkImage.h>

class MITK_M2_CORE_HELPER_EXPORT QmitkDataNodeExportComponentAction : public QAction, public QmitkAbstractDataNodeAction
{
  Q_OBJECT

public:

  QmitkDataNodeExportComponentAction(QWidget* parent, berry::IWorkbenchPartSite* workbenchPartSite, berry::IConfigurationElement * configElement = nullptr);
  static mitk::Image::Pointer ExportComponentImage(const mitk::Image * img, unsigned int i);
  

private Q_SLOTS:

  void OnActionTriggered(bool);

protected:

  void InitializeAction() override;
};

QMITK_DECLARE_CONTEXT_MENU_ACTION_PROVIDER(QmitkDataNodeExportComponentAction, MITK_M2_CORE_HELPER_EXPORT)
