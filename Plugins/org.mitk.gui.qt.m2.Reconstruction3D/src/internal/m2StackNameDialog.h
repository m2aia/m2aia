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
#include "ui_m2StackNameDialogControls.h"
#include <QDialog>

class m2StackNameDialog : public QDialog
{
  Q_OBJECT

public:
  explicit m2StackNameDialog(QWidget *parent = nullptr) : QDialog(parent)
  {
    m_Controls.setupUi(this);
    this->setSizeGripEnabled(true);
  }

  int exec() override
  {
    return QDialog::exec();
  }

  std::string GetName() { return m_Controls.lineEdit->text().toStdString(); }
  void SetName(std::string v) { return m_Controls.lineEdit->setText(v.c_str()); }
  
private:
  
  Ui::StackNameDialog m_Controls;
 
  
};
