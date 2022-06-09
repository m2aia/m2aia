/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "QmitkM2aiaAboutDialog.h"
#include "QmitkModulesDialog.h"
#include <qdialogbuttonbox.h>
#include <QPushButton>
#include <itkConfigure.h>
#include <mitkVersion.h>
#include <vtkVersionMacros.h>
#include <mitkM2aiaVersion.h>

QmitkM2aiaAboutDialog::QmitkM2aiaAboutDialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
  m_GUI.setupUi(this);

  QString mitkRevision(MITKM2AIA_REVISION);
  QString mitkRevisionDescription(MITKM2AIA_REVISION_DESC);
  
 

  QPushButton *btnModules = new QPushButton(QIcon(":/M2aiaApplication/ModuleView.png"), "Modules");
  m_GUI.m_ButtonBox->addButton(btnModules, QDialogButtonBox::ActionRole);

  connect(btnModules, SIGNAL(clicked()), this, SLOT(ShowModules()));
  connect(m_GUI.m_ButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

QmitkM2aiaAboutDialog::~QmitkM2aiaAboutDialog()
{
}

void QmitkM2aiaAboutDialog::ShowModules()
{
  QmitkModulesDialog dialog(this);
  dialog.exec();
}

QString QmitkM2aiaAboutDialog::GetAboutText() const
{
  return m_GUI.m_AboutLabel->text();
}

QString QmitkM2aiaAboutDialog::GetCaptionText() const
{
  //return m_GUI.m_CaptionLabel->text();
  return "";
}


void QmitkM2aiaAboutDialog::SetAboutText(const QString &text)
{
  m_GUI.m_AboutLabel->setText(text);
}

void QmitkM2aiaAboutDialog::SetCaptionText(const QString &/*text*/)
{
  //m_GUI.m_CaptionLabel->setText(text);
}

