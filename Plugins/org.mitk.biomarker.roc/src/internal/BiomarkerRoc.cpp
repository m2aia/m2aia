/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/


// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include "BiomarkerRoc.h"
//#include "chart.h"

// Qt
#include <QMessageBox>
#include <QDialog>
#include <QFileDialog>

// mitk image
#include <mitkImage.h>

const std::string BiomarkerRoc::VIEW_ID = "org.mitk.views.biomarkerrocanalysis";
BiomarkerRoc::BiomarkerRoc()
{}

void BiomarkerRoc::SetFocus()
{
  if(true)
    m_Controls.FileChooseButton->setFocus();
  else
    m_Controls.MdiArea->setFocus();
}

void BiomarkerRoc::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  connect(m_Controls.FileChooseButton, &QPushButton::clicked, this, &BiomarkerRoc::OpenFileChooseDialog);
  connect(m_Controls.CalculateButton, &QPushButton::clicked, this, &BiomarkerRoc::CalculateRocCurve);
}

void BiomarkerRoc::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/,
                                                const QList<mitk::DataNode::Pointer> &nodes)
{
  // iterate all selected objects, adjust warning visibility
  foreach (mitk::DataNode::Pointer node, nodes)
  {
    if (node.IsNotNull() && dynamic_cast<mitk::Image *>(node->GetData()))
    {
      m_Controls.Label->setVisible(true);
      m_Controls.FileChooseButton->setEnabled(true);
      m_Controls.CalculateButton->setEnabled(false);
      m_Controls.MdiArea->setVisible(false);
      return;
    }
  }

  //m_Controls.labelWarning->setVisible(true);
  //m_Controls.buttonPerformImageProcessing->setEnabled(false);
}

void BiomarkerRoc::OpenFileChooseDialog()
{
    QString filename = QFileDialog::getOpenFileName(nullptr, tr("Open a file"), nullptr, tr("*.roc Files (*.roc)"));
    m_Controls.Label->setText("Selected File: \n" + filename);
}

void BiomarkerRoc::CalculateRocCurve()
{
    //chart = new Chart(m_Controls.MdiArea);
    //chart->show();
}
