/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/


#ifndef BiomarkerRoc_h
#define BiomarkerRoc_h

#include <berryISelectionListener.h>

#include <QmitkAbstractView.h>

#include <QFile>

#include <vector>
#include <utility>

//#include "chart.h"
#include "ui_BiomarkerRocControls.h"

/**
  \brief BiomarkerRoc
  This class is the main class of the view containing the ROC curve.
  It has two buttons (select file and calculate) and the field for the actual curve
*/
class BiomarkerRoc : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;
  BiomarkerRoc();
protected:
  virtual void CreateQtPartControl(QWidget *parent) override;
  virtual void SetFocus() override;

  /// \brief called by QmitkFunctionality when DataManager's selection has changed
  virtual void OnSelectionChanged(berry::IWorkbenchPart::Pointer source,
                                  const QList<mitk::DataNode::Pointer> &nodes) override;

  /// \brief Called when the user clicks the Choose File button
  void OpenFileChooseDialog();
  /// \brief Called when the user clicks the Calculate button
  void CalculateRocCurve();

  Ui::BiomarkerRocControls m_Controls;
private:
  //Chart* chart;
};

#endif // BiomarkerRoc_h
