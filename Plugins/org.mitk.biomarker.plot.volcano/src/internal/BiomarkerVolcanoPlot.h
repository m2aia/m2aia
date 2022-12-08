/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/


#ifndef BiomarkerVolcanoPlot_h
#define BiomarkerVolcanoPlot_h

#include <berryISelectionListener.h>

#include <QmitkAbstractView.h>

#include "ui_BiomarkerVolcanoPlotControls.h"

class BiomarkerVolcanoPlot : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;

protected:
  virtual void SetFocus() override; 
  virtual void CreateQtPartControl(QWidget *parent) override;
  void GenerateVolcanoPlot();

  Ui::BiomarkerVolcanoPlotControls m_Controls;
};

#endif // BiomarkerVolcanoPlot_h
