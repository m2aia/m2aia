/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef MultiModalMaskExport_h
#define MultiModalMaskExport_h

#include <berryISelectionListener.h>

#include <QmitkAbstractView.h>
#include <QmitkMultiNodeSelectionWidget.h>
#include <QmitkSingleNodeSelectionWidget.h>

#include "ui_MultiModalMaskExportControls.h"

/**
  \brief MultiModalMaskExport

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class MultiModalMaskExport : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:
  static const std::string VIEW_ID;

protected:
  virtual void CreateQtPartControl(QWidget *parent) override;

  virtual void SetFocus() override;

  /// \brief called by QmitkFunctionality when DataManager's selection has changed
  virtual void OnSelectionChanged(berry::IWorkbenchPart::Pointer source,
                                  const QList<mitk::DataNode::Pointer> &nodes) override;

  Ui::MultiModalMaskExportControls m_Controls;

  std::vector<mitk::DataNode::Pointer> m_SelectedImzlNodes;
  QmitkMultiNodeSelectionWidget *m_MassSpecDataNodeSelectionWidget;
  QmitkMultiNodeSelectionWidget *m_MaskImageSelectionWidget;
  QPushButton *m_ExportButton;

  void Export();
  void SetSelectionVisibility(bool);
};

#endif // MultiModalMaskExport_h
