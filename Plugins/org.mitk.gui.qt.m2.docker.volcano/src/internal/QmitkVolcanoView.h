/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef QmitkVolcanoView_h
#define QmitkVolcanoView_h

#include <berryISelectionListener.h>
#include <QmitkAbstractView.h>
#include <QmitkSingleNodeSelectionWidget.h>

#include <ui_VolcanoViewControls.h>

class QmitkVolcanoView : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;

  void CreateQtPartControl(QWidget* parent) override;

private slots:
  void OnStartVolcano();
  void OnImageSelectionChanged(QmitkSingleNodeSelectionWidget::NodeList nodes);
  void OnMaskSelectionChanged(QmitkSingleNodeSelectionWidget::NodeList nodes);
  void UpdateGroupComboBoxes();

private:
  void SetFocus() override;
  void EnableWidgets(bool enable);

  Ui::VolcanoViewControls m_Controls;
};

#endif
