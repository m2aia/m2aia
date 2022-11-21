/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include "ExternalDockerModules.h"

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include <QmitkAbstractNodeSelectionWidget.h>
#include <QmitkSingleNodeSelectionWidget.h>

// Qt
#include <QMessageBox>

// Poco
#include <Poco/Process.h>

// std
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <regex>

// mitk image
#include <mitkImage.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkIOUtil.h>

// m2aia
#include <m2ImzMLSpectrumImage.h>
#include <m2SpectrumImageBase.h>

const std::string ExternalDockerModules::VIEW_ID = "org.mitk.views.externaldockermodules";

void ExternalDockerModules::SetFocus()
{
  m_Controls.inputImage->setFocus();
}

void ExternalDockerModules::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  m_Controls.inputImage->SetDataStorage(GetDataStorage());
  m_Controls.inputImage->SetNodePredicate(
  mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::SpectrumImageBase>::New(),
                          mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.inputImage->SetSelectionIsOptional(false);
  m_Controls.inputImage->SetInvalidInfo("Choose image");
  m_Controls.inputImage->SetAutoSelectNewNodes(true);
  m_Controls.inputImage->SetPopUpTitel("Select image");
  m_Controls.inputImage->SetPopUpHint("Select the image you want to work with. This can be any opened image (*.imzML).");
  connect(m_Controls.btnExecute, &QPushButton::clicked, this, &ExternalDockerModules::ExecuteModule);

  // get data to put inside combobox EXPERIMENTAL
  if (false)
  {
    std::string comboboxCommand = "bash";
    std::vector<std::string> comboboxArguments = { "-c", "'\
    for img in $(docker images | grep m2aia/docker | tr -s \" \" | cut -d\" \" -f 3); do \
      docker inspect $img | grep \"m2aia\\.docker\" | uniq | cut -d\" \" -f 18 >> docker_imgs.txt; \
    done'" };
    Poco::Process getDockerImgs;
    Poco::Process::Args getDockerImgsArgs;
    getDockerImgsArgs.insert(getDockerImgsArgs.end(), comboboxArguments.begin(), comboboxArguments.end());
    auto getDockerImgsHandle = getDockerImgs.launch(comboboxCommand, getDockerImgsArgs);
    int getDockerImgsCode = getDockerImgsHandle.wait();
    if (getDockerImgsCode)
      MITK_INFO << "[ExternalDockerModules]: " << "Error: " << getDockerImgsCode << " " << strerror(getDockerImgsCode);
    std::ifstream ifs("docker_imgs.txt");
    if (!ifs)
      MITK_INFO << "[ExternalDockerModules]: " << "Error: could not find docker_imgs.txt";
  } 

  m_Controls.comboContainers->addItem("m2aia/docker:base");
  m_Controls.comboContainers->addItem("m2aia/docker:pym2aia");
  m_Controls.comboContainers->addItem("m2aia/docker:pym2aia-example");
  m_Controls.comboContainers->addItem("m2aia/docker:python");
  m_Controls.comboContainers->addItem("m2aia/docker:r");
}

std::vector<std::string> split(const std::string& str, const char delimiter)
{
  std::stringstream sstream(str);
  std::string temp;
  std::vector<std::string> vec;
  while(getline(sstream, temp, delimiter))
  {
    if (temp != "")
      vec.push_back(temp);
  }
  return vec;
}

std::string escape(std::string input)
{
  size_t pos = 0;
  while (pos < input.size())
  {
    pos = input.find_first_of(' ', pos);
    if (pos >= input.size()) return input;
    input.replace(input.begin() + pos, input.begin() + pos + 1, "\\ ");
    pos += 2; // skip the two backslashes and the space
    // should only one backslash be skipped?
  }
  return input;
}

void ExternalDockerModules::ExecuteModule()
{
  //TODO: über alle nodes im data manager iterieren und pro node einen mountpoint adden
  auto node = m_Controls.inputImage->GetSelectedNode();
  auto image = static_cast<m2::ImzMLSpectrumImage*>(node->GetData());
  std::string inputPath_1;
  image->GetPropertyList()->GetStringProperty("MITK.IO.reader.inputlocation", inputPath_1);
  auto splitPath = split(inputPath_1, '/');
  inputPath_1 = "/";
  for (size_t i = 0; i < splitPath.size() - 1; ++i)
    inputPath_1 += splitPath[i] + "/";

  std::string outputPath_1 = mitk::IOUtil::CreateTemporaryDirectory();
  auto outputPathVec = split(outputPath_1, '/');
  outputPath_1 = "/";
  for (size_t i = 0; i < outputPathVec.size(); ++i)
    if (outputPathVec[i] != "")
      outputPath_1 += outputPathVec[i] + "/";

  std::string imzmlName = splitPath[splitPath.size() - 1];
  std::string containerName = m_Controls.comboContainers->currentText().toStdString();
  std::string interpreter = "";
  std::string fileEnding = "";
  auto foundpy = containerName.find("py");
  auto foundPy = containerName.find("Py");

  if (foundpy != std::string::npos || foundPy != std::string::npos)
  {
    interpreter = "python3";
    fileEnding = ".py";
  }
  auto moduleParamsVec = split(m_Controls.moduleParams->text().toStdString(), ' ');
  std::string moduleParams = "";
  std::string outputFileName;
  for (size_t i = 0; i < moduleParamsVec.size(); ++i)
  {
    if (moduleParamsVec[i] == "--m2aia-out") 
      continue;
    if (i > 0 && moduleParamsVec[i - 1] == "--m2aia-out")
    {
      outputFileName = moduleParamsVec[i];
      continue;
    }
    moduleParams += moduleParamsVec[i] + " ";
  }

  // forge command
  std::string command = "docker";
  std::string arguments = "run -t --rm --name=m2aia-container";
  arguments += " -v " + inputPath_1 + ":/data1/ "; 
  arguments += " -v " + outputPath_1 + ":/output1/ " + containerName;
  arguments += " " + interpreter + " m2aia-module" + fileEnding + " " + escape(imzmlName) + " " + outputFileName  + " " + moduleParams;

  auto stv = split(arguments, ' ');
  // get the args  
  Poco::Process::Args processArgs;
  processArgs.insert(processArgs.end(), stv.begin(), stv.end());
  // launch the process
  Poco::Process p;
  MITK_INFO << "launching process with command '" << command << " " << arguments << "'.";
  auto handle = p.launch(command, processArgs);
  int code = handle.wait();
  MITK_INFO << "finished processing with code " << code;
  if (code)
    MITK_INFO << strerror(code);
  else 
  {
    // load output file into m2aia
    auto dataPointerVec = mitk::IOUtil::Load(outputPath_1 + outputFileName);
    auto node = mitk::DataNode::New();
    node->SetData(dataPointerVec[0]);
    node->SetName(outputFileName);
    GetDataStorage()->Add(node);
  }
}
