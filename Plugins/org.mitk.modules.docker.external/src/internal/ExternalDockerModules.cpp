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
  {
    MITK_INFO << "[split method] " << temp;
    vec.push_back(temp);
  }
  return vec;
}

void ExternalDockerModules::InitializePaths(const std::string& fileExtension)
{
  if (fileExtension == "py")
  {
    this->m_pathBin = m_pathDocker + "M2aiaContainerPython/bin/";
    this->m_imageName += "-python";
    this->m_containerName += "-python";
    this->m_interpreter = "python";
    return;
  }
  if (fileExtension == "r" || fileExtension == "R")
  {
    this->m_pathBin = m_pathDocker + "M2aiaContainerR/bin/";
    this->m_imageName += "-r";
    this->m_containerName += "-r";
    this->m_interpreter = "Rscript";
    return;
  }
  this->m_pathBin =  m_pathDocker + "M2aiaContainer/bin/";
  this->m_imageName += "-base";
  this->m_containerName += "-base";
  // no interpreter
}

void ExternalDockerModules::ExecuteModule()
{
  std::string moduleName = m_Controls.moduleName->text().toStdString();
  std::string moduleParams = m_Controls.moduleParams->text().toStdString();
  //get file extension
  auto vec = split(moduleName, '.');
  std::string fileExtension = vec[vec.size()-1];
  //ensure that the module is indeed present
  {
    // set bin path
    InitializePaths(fileExtension);
    std::ifstream file{this->m_pathBin + moduleName};
    if (!file) 
    { 
      m_Controls.labelWarning->setText(QString(("Module " + this->m_pathBin + moduleName + " not found.").c_str()));
      m_Controls.labelWarning->setStyleSheet("color: orange; font-weight: bold;");
      m_Controls.labelWarning->setVisible(true);
      return; 
    }
  }
  m_Controls.labelWarning->setVisible(false);
  // get the args  
  Poco::Process::Args processArgs;
  auto strvec = split(moduleParams, ' ');
  processArgs.insert(processArgs.end(), strvec.begin(), strvec.end());
  // forge command
  std::string command = "docker run -it --name=" + m_containerName + 
    " --mount 'type=bind,source=" + m_pathSharedFolder + ",destination=/m2aia-share/' " + 
    m_imageName + " " + m_interpreter + " " + moduleName;
  // launch the process
  Poco::Process p;
  MITK_INFO << "launching process with command '" << command << "'.";
  auto handle = p.launch(command, processArgs);
  int code = handle.wait();
  MITK_INFO << "finished processing with code " << code;
}