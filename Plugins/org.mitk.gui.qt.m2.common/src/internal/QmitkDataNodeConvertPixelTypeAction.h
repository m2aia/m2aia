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


class QmitkDataNodeConvertPixelTypeAction : public QAction, public QmitkAbstractDataNodeAction
{
  Q_OBJECT

public:

  QmitkDataNodeConvertPixelTypeAction(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchPartSite);
  QmitkDataNodeConvertPixelTypeAction(QWidget* parent = nullptr, berry::IWorkbenchPartSite* workbenchPartSite = nullptr);
  mitk::Image::Pointer OnApplyCastImage(mitk::Image::Pointer image, itk::IOComponentEnum type);
  
private Q_SLOTS:

  void OnMenuAboutShow();

protected:

  void InitializeAction() override;

};

