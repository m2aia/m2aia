/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/


// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// mitk image
#include <mitkImage.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>

// m2aia
#include <m2ImzMLSpectrumImage.h>
#include <m2SpectrumImageBase.h>
#include <m2DockerHelper.h>

// Qt
#include <QMessageBox>

#include "BiomarkerVolcanoPlot.h"

const std::string BiomarkerVolcanoPlot::VIEW_ID = "org.mitk.views.biomarkervolcanoplot";

void BiomarkerVolcanoPlot::SetFocus()
{
  m_Controls.Images->setFocus();
}

void BiomarkerVolcanoPlot::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  m_Controls.Images->SetDataStorage(GetDataStorage());
  m_Controls.Images->SetNodePredicate(
  mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::SpectrumImageBase>::New(),
                          mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.Images->SetSelectionIsOptional(false);
  m_Controls.Images->SetInvalidInfo("Choose image");
  m_Controls.Images->SetPopUpTitel("Select image");
  m_Controls.Images->SetPopUpHint("Select the image you want to work with. This can be any opened image (*.imzML).");
  connect(m_Controls.GenerateBtn, &QPushButton::clicked, this, &BiomarkerVolcanoPlot::GenerateVolcanoPlot);
}

void BiomarkerVolcanoPlot::GenerateVolcanoPlot()
{
  auto nodes = m_Controls.Images->GetSelectedNodesStdVector();
  std::vector<std::string> args = m2::DockerHelper::split(m_Controls.Args->text().toStdString(), ' ');
  m2::DockerHelper::ExecuteModule("m2aia/docker:pym2aia-volcano-plot", nodes, args);
}
