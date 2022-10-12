/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/


#ifndef BiomarkerRoc_h
#define BiomarkerRoc_h

#include <berryISelectionListener.h>
#include <QmitkAbstractView.h>
#include <QmitkSingleNodeSelectionWidget.h>
#include <QFile>
#include <vector>
#include <utility>
#include "ui_BiomarkerRocControls.h"

QT_CHARTS_USE_NAMESPACE
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

  /// \brief Called when the user clicks the Calculate button
  void OnButtonCalcPressed();
  Ui::BiomarkerRocControls m_Controls;
private:
  void AddToTable(const double&, const double&);

  void RenderChart();

  double CalculateAUC(const std::vector<double>&, const std::vector<double>&);

  void RocAnalysis(const double& mz, std::vector<double>&, std::vector<double>&);
  
  static const int renderGranularity;
  
  //TODO: efficiency update so one calculation of ROC wont take 100ms
  //double* maskedData;
  //mitk::Label::PixelType* selectionData;
};

#endif // BiomarkerRoc_h
