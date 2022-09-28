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
#include <mitkLabelSetImage.h>
#include <mitkImagePixelReadAccessor.h>

// m2aia
#include <m2SpectrumImageBase.h>
#include <m2ImzMLSpectrumImage.h>

// for logging purposes
#define ROC_SIG "[BiomarkerRoc] "

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
  m_Controls.image->SetDataStorage(GetDataStorage());
  m_Controls.image->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::SpectrumImageBase>::New(),
      mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object")))
  );
  m_Controls.selection->SetDataStorage(GetDataStorage());
  m_Controls.selection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::LabelSetImage>::New(),
      mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object")))
  );
  m_Controls.image->SetSelectionIsOptional(false);
  m_Controls.image->SetEmptyInfo("Choose image");
  m_Controls.image->SetPopUpTitel("Select image");
  m_Controls.image->SetPopUpHint("Select the image you want to work with. This can be any opened image (*.imzML).");
  m_Controls.selection->SetSelectionIsOptional(false);
  m_Controls.selection->SetEmptyInfo("Choose selection");
  m_Controls.selection->SetPopUpTitel("Select selection");
  m_Controls.selection->SetPopUpHint("Choose the selection you want to work with. This can be any currently opened selection.");
  connect(m_Controls.calcButton, &QPushButton::clicked, this, &BiomarkerRoc::OnCalcButtonPressed);
}

void BiomarkerRoc::OnCalcButtonPressed()
{
  auto originalImage = dynamic_cast<m2::ImzMLSpectrumImage*>(m_Controls.image->GetSelectedNode()->GetData());
  auto selection = dynamic_cast<mitk::Image*>(m_Controls.selection->GetSelectedNode()->GetData());
  auto maskedImage = mitk::Image::New(); // image to which the selection has been applied to
  ///mitk::equals() für image info abgleich
  //in registration view -> open peak picking view abgucken
  if (originalImage && selection)
  {
    //filters the selection for one mz value, displays the resulting image afterwards
    {
      double mz = m_Controls.mzValue->value();
      maskedImage->Initialize(originalImage);
      originalImage->GetImage(mz, 0.45, selection, maskedImage); //write in maskedImage
      auto dataNode = mitk::DataNode::New();
      dataNode->SetData(maskedImage);
      dataNode->SetName("Biomarker Roc data");
      GetDataStorage()->Add(dataNode, m_Controls.image->GetSelectedNode());
    }
    // accessing the image data using ImageReadAccessor
    mitk::ImageReadAccessor readAccessor(maskedImage);
    auto data = static_cast<const double*>(readAccessor.GetData());
    auto dims = maskedImage->GetDimensions();
    size_t N = dims[0] * dims[1] * dims[2];
    //TODO: do the same for maskedImage and then after the screenshot
    MITK_INFO << ROC_SIG << "original dt: " << originalImage->GetPixelType().GetTypeAsString();
    MITK_INFO << ROC_SIG << "masked dt: " << maskedImage->GetPixelType().GetTypeAsString();
    auto peaks = originalImage->GetPeaks();
  }
}
