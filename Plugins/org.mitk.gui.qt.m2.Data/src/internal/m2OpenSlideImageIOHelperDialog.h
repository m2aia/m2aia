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
#include "ui_m2OpenSlideImageIOHelperDialog.h"
#include <QDialog>
#include <m2OpenSlideImageIOHelperObject.h>

class m2OpenSlideImageIOHelperDialog : public QDialog
{
  Q_OBJECT

public:
  explicit m2OpenSlideImageIOHelperDialog(QWidget *parent = nullptr) : QDialog(parent)
  {
    m_Controls.setupUi(this);
    this->setSizeGripEnabled(true);
  }

  void SetOpenSlideImageIOHelperObject(m2::OpenSlideImageIOHelperObject *helper);
  void UpdateImageInformation();

  int exec() override
  {
    UpdateImageInformation();
    return QDialog::exec();
  }

  int GetSelectedLevel() { return m_SelectedLevel; }

private:
  m2::OpenSlideImageIOHelperObject::Pointer m_Helper;
  Ui::OpenSlideImageIOHelperDialog m_Controls;
  int m_SelectedLevel = -1;
};
