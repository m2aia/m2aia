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

// Poco
#include <Poco/Process.h>

// std
#include <sstream>
#include <vector>
#include <iostream>

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

std::vector<std::string> split(const std::string& str, const char delimiter)
{
  std::stringstream sstream(str);
  std::string temp;
  std::vector<std::string> vec;
  while(getline(sstream, temp, delimiter))
    vec.push_back(temp);
  return vec;
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
  Poco::Process::Args a;
  auto strvec = split(m_Controls.moduleParams->text().toStdString(), ' ');
  a.insert(a.end(), strvec.begin(), strvec.end());
  Poco::Process p;
  auto handle = p.launch("docker run -it --name=m2aia-container-r --mount 'type=bind,source=/home/maia/Documents/maia/Docker/m2aia-share,destination=/m2aia-share' m2aia-docker-python", a);
  int code = handle.wait();
  MITK_INFO << "finished processing with code " << code;
}