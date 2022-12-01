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

// Qmitk
#include <QmitkMultiNodeSelectionWidget.h>

// std
#include <filesystem>

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
#include <m2DockerHelper.h>

#define EDM_SIG "[ExternalDockerModules] "

const std::string ExternalDockerModules::VIEW_ID = "org.mitk.views.externaldockermodules";

void ExternalDockerModules::SetFocus()
{
  m_Controls.inputImage->setFocus();
}

void ExternalDockerModules::CreateQtPartControl(QWidget *parent)
{
  m_Controls.setupUi(parent);
  m_Controls.inputImage->SetDataStorage(GetDataStorage());
  m_Controls.inputImage->SetNodePredicate(
  mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::SpectrumImageBase>::New(),
                          mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.inputImage->SetSelectionIsOptional(false);
  m_Controls.inputImage->SetInvalidInfo("Choose image");
  m_Controls.inputImage->SetPopUpTitel("Select image");
  m_Controls.inputImage->SetPopUpHint("Select the image you want to work with. This can be any opened image (*.imzML).");
  connect(m_Controls.btnExecute, &QPushButton::clicked, this, &ExternalDockerModules::Execute);

  m_Controls.comboContainers->addItem("m2aia/docker:base");
  m_Controls.comboContainers->addItem("m2aia/docker:pym2aia");
  m_Controls.comboContainers->addItem("m2aia/docker:pym2aia-example");
  m_Controls.comboContainers->addItem("m2aia/docker:python");
  m_Controls.comboContainers->addItem("m2aia/docker:r");
  m_Controls.comboContainers->addItem("m2aia/docker:r-example");
}

void ExternalDockerModules::Execute()
{
  std::string containerName = m_Controls.comboContainers->currentText().toStdString();
  auto nodes = m_Controls.inputImage->GetSelectedNodesStdVector();

  // TODO: once xml schema will be implemented, this is subject to change
  std::string moduleParams = m_Controls.moduleParams->text().toStdString();
  std::string outputPath = m2::DockerHelper::ExecuteModule(containerName, nodes, m2::DockerHelper::split(moduleParams, ' '));
  // container exited with error
  if (outputPath == "") return;

  // load output files into m2aia
  namespace fs = std::filesystem; 
  for (auto& file : fs::directory_iterator(outputPath))
  {
    auto dataPointerVec = mitk::IOUtil::Load(file.path());
    auto node = mitk::DataNode::New();
    node->SetData(dataPointerVec[0]);
    auto filenameVec = m2::DockerHelper::split(file.path(), '/');
    node->SetName(filenameVec[filenameVec.size()-1]);
    GetDataStorage()->Add(node);
  }
}
