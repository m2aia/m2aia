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

#include "MitkQtWidgetsExtExports.h"
#include <qdialog.h>
#include <qobject.h>
#include <ui_QmitkM2aiaAboutDialog.h>

class QmitkM2aiaAboutDialog : public QDialog
{
  Q_OBJECT

public:
  QmitkM2aiaAboutDialog(QWidget *parent = nullptr,
                        Qt::WindowFlags f = Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
  ~QmitkM2aiaAboutDialog() override;

  QString GetAboutText() const;
  QString GetCaptionText() const;

  void SetAboutText(const QString &text);
  void SetCaptionText(const QString &text);

protected slots:
  void ShowModules();

private:
  Ui::QmitkM2aiaAboutDialog m_GUI;
};
