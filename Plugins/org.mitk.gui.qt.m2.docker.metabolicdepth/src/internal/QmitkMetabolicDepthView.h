/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef QmitkMetabolicDepthView_h
#define QmitkMetabolicDepthView_h

#include <berryISelectionListener.h>
#include <QmitkAbstractView.h>
#include <QmitkSingleNodeSelectionWidget.h>
#include <QFutureWatcher>

#include <ui_MetabolicDepthViewControls.h>

class QmitkMetabolicDepthView : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;

  void CreateQtPartControl(QWidget* parent) override;

private slots:
  void OnStartMetabolicDepthProcessing();
  void OnMetabolicDepthProcessingFinished();
  void OnMetabolicDepthProcessingCanceled();
  void OnImageSelectionChanged(QmitkSingleNodeSelectionWidget::NodeList nodes);
  void OnCancelProcessing();

private:
  void SetFocus() override;
  void EnableWidgets(bool enable);
  void UpdateStatusLabel(const QString& message, const QString& styleSheet = "");

  Ui::MetabolicDepthViewControls m_Controls;
  
  // Structure to hold processing results
  struct MetabolicDepthResult {
    mitk::BaseData::Pointer depthImage;
    mitk::BaseData::Pointer contourImage;
    mitk::DataNode::ConstPointer referenceNode;
    mitk::DataNode::ConstPointer maskNode;
  };
  
  // Future watcher for async Docker execution
  QFutureWatcher<MetabolicDepthResult> m_DockerProcessWatcher;
};

#endif
