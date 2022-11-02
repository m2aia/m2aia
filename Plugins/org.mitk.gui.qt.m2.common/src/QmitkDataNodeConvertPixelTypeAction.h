/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#pragma once
#include <org_mitk_gui_qt_m2_common_Export.h>

#include "QmitkAbstractDataNodeAction.h"
#include "mitkIContextMenuActionProvider.h"

// mitk core
#include <mitkDataNode.h>
#include <mitkImage.h>

// qt
#include <QAction>

class MITK_M2_CORE_HELPER_EXPORT QmitkDataNodeConvertPixelTypeAction : public QAction, public QmitkAbstractDataNodeAction
{
  Q_OBJECT

public:

  QmitkDataNodeConvertPixelTypeAction(QWidget* parent, berry::IWorkbenchPartSite* workbenchPartSite, berry::IConfigurationElement * configElement =nullptr);
  static mitk::Image::Pointer OnApplyCastImage(mitk::Image::Pointer image, itk::IOComponentEnum type);

private Q_SLOTS:

  void OnMenuAboutShow();
  void OnActionTriggered(bool);

protected:

  void InitializeAction() override;

};

QMITK_DECLARE_CONTEXT_MENU_ACTION_PROVIDER(QmitkDataNodeConvertPixelTypeAction, MITK_M2_CORE_HELPER_EXPORT)


