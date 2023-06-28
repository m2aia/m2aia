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
#include "ui_m2DataStatistics.h"
#include <QmitkAbstractView.h>





class m2DataStatistics : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:
  static const std::string VIEW_ID;
  
  virtual void CreateQtPartControl(QWidget *) override;
  virtual void SetFocus() override {}

private:
  Ui::m2DataStatistics m_Controls;
  QWidget * m_Parent = nullptr;
  void UpdateTable();

};
