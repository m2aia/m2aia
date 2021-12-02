/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#pragma once
#include "ui_m2DataTools.h"
#include <QThreadPool>
#include <QmitkAbstractView.h>

#include <mitkColorBarAnnotation.h>
#include <mitkTextAnnotation2D.h>
#include <m2UIUtils.h>


class m2DataTools : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:
  static const std::string VIEW_ID;
  Ui::imsDataToolsControls *Controls() { return &m_Controls; }

public slots:
  void OnEqualizeLW();
  void OnApplyTiling();
  void OnResetTiling();  
  void UpdateColorBarAndRenderWindows();
  
  
protected:

  void UpdateLevelWindow(const mitk::DataNode *node);
  virtual void CreateQtPartControl(QWidget *parent) override;
  virtual void SetFocus() override {}

  
  Ui::imsDataToolsControls m_Controls;
  QWidget * m_Parent = nullptr;
  QThreadPool m_pool;
  
  std::vector<mitk::ColorBarAnnotation::Pointer> m_ColorBarAnnotations;
};
