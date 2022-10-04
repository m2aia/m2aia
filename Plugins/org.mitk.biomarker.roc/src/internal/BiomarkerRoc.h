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

//#include "chart.h"
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
  void RenderChart(std::vector<double>, std::vector<double>);
  //Chart* chart;
};

#endif // BiomarkerRoc_h
