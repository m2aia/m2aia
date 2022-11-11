/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/


// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include "ExternalDockerModules.h"

// Qt
#include <QMessageBox>

// mitk image
#include <mitkImage.h>

const std::string ExternalDockerModules::VIEW_ID = "org.mitk.views.externaldockermodules";

void ExternalDockerModules::SetFocus()
{
  m_Controls.moduleName->setFocus();
}

void ExternalDockerModules::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  m_Controls.labelWarning->setVisible(false);
  connect(m_Controls.btnExecute, &QPushButton::clicked, this, &ExternalDockerModules::ExecuteModule);
}

void ExternalDockerModules::ExecuteModule()
{
  std::string moduleName = m_Controls.moduleName->text().toStdString();
  std::string moduleParams = m_Controls.moduleParams->text().toStdString();
  //ensure that the module is indeed present
  {
    std::ifstream file{this->m_pathToModuleDirectory + moduleName};
    if (!file) 
    { 
      m_Controls.labelWarning->setText(QString(("Module " + this->m_pathToModuleDirectory + moduleName + " not found.").c_str()));
      m_Controls.labelWarning->setStyleSheet("color: orange; font-weight: bold;");
      m_Controls.labelWarning->setVisible(true);
      return; 
    }
  }
  m_Controls.labelWarning->setVisible(false);
  // execute module with the specified parameters

}