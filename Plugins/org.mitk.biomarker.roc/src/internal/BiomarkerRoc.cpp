/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include "BiomarkerRoc.h"

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include <QmitkAbstractNodeSelectionWidget.h>
//#include "chart.h"

// Qt
#include <QMessageBox>
#include <QDialog>
#include <QFileDialog>

// mitk
#include <mitkImage.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateProperty.h>
#include <m2SpectrumImageBase.h>

const std::string BiomarkerRoc::VIEW_ID = "org.mitk.views.biomarkerrocanalysis";
BiomarkerRoc::BiomarkerRoc()
{
}

void BiomarkerRoc::SetFocus()
{
  m_Controls.label->setFocus();
}

void BiomarkerRoc::CreateQtPartControl(QWidget *parent)
{
  m_Controls.setupUi(parent);
  m_Controls.selectImage->SetDataStorage(GetDataStorage());
  m_Controls.selectImage->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::SpectrumImageBase>::New(),
      mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object")))
  );
  m_Controls.selectImage->SetSelectionIsOptional(true);
  m_Controls.selectImage->SetEmptyInfo("Choose the image");
  m_Controls.selectImage->SetPopUpTitel("Select image");
  m_Controls.selectImage->SetPopUpHint("Select the image you want to work with. This can be any opened image (*.imzML).");
  connect(m_Controls.calcButton, &QPushButton::clicked, this, &BiomarkerRoc::OnCalcButtonPressed);
}

void BiomarkerRoc::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/,
                                                const QList<mitk::DataNode::Pointer> &nodes)
{
  // iterate all selected objects, adjust warning visibility
  foreach (mitk::DataNode::Pointer node, nodes)
  {
    if (node.IsNotNull() && dynamic_cast<mitk::Image *>(node->GetData()))
    {
      m_Controls.label->setVisible(true);
      m_Controls.selectImage->setEnabled(true);
      m_Controls.calcButton->setEnabled(false);
      m_Controls.mdiArea->setVisible(false);
      return;
    }
  }
}

void BiomarkerRoc::OpenFileChooseDialog()
{
  QString filename = QFileDialog::getOpenFileName(nullptr, tr("Open a file"), nullptr, tr("*.roc Files (*.roc)"));
  m_Controls.label->setText("Selected File: \n" + filename);
}

void BiomarkerRoc::OnCalcButtonPressed()
{
  if (m_Controls.selectImage->GetSelectedNode())
  {
    std::string nodeName = m_Controls.selectImage->GetSelectedNode()->GetName();
    m_Controls.label->setText("Opened Image " + QString(nodeName.c_str()));
    m_Controls.label->update();
  }
}
