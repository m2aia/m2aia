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
#include <berryPlatform.h>
#include <berryPlatformUI.h>

// Qmitk
#include <QmitkAbstractNodeSelectionWidget.h>
//#include "chart.h"

// Qt
#include <QMessageBox>
#include <QDialog>
#include <QFileDialog>
#include <QtCharts/QChartView>
#include <QLineSeries>
#include <QValueAxis>

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
  connect(m_Controls.buttonCalc, &QPushButton::clicked, this, &BiomarkerRoc::OnButtonCalcPressed);
  connect(m_Controls.buttonOpenPeakPickingView, &QCommandLinkButton::clicked, this, 
    []()
    {
      try
      {
        if (auto platform = berry::PlatformUI::GetWorkbench())
          if (auto workbench = platform->GetActiveWorkbenchWindow())
            if (auto page = workbench->GetActivePage())
              page->ShowView("org.mitk.views.m2.PeakPicking");
      }
      catch (berry::PartInitException& e)
      {
        BERRY_ERROR << "Error: " << e.what() << std::endl;
      }
    }
  );
}

void BiomarkerRoc::RenderChart(std::vector<double> TPR, std::vector<double> FNR)
{
  auto serie = new QLineSeries();
  int T = 0, F = 0;
  for (size_t idx = 0; idx < FNR.size(); ++idx)
  {
    serie->append(FNR[idx], TPR[idx]);
    if (TPR[idx] > FNR[idx]) ++T; else ++F;
    MITK_INFO << ROC_SIG << "FNR[" << idx << "]: " << FNR[idx] << " F: " << F;
    MITK_INFO << ROC_SIG << "TPR[" << idx << "]: " << TPR[idx] << " T: " << T;
  }
  MITK_INFO << ROC_SIG << "sum T = " << T;
  MITK_INFO << ROC_SIG << "sum F = " << F;
  auto chart = new QtCharts::QChart();
  chart->addSeries(serie);
  auto axisX = new QValueAxis();
  axisX->setMin(0);
  axisX->setMax(1);
  auto axisY = new QValueAxis();
  axisY->setMin(0);
  axisY->setMax(1);
  
  chart->setAxisX(axisX, serie);
  chart->setAxisY(axisY, serie);
  m_Controls.chartView->setChart(chart);
  m_Controls.chartView->update();
  m_Controls.chartView->setVisible(true);
}

void BiomarkerRoc::OnButtonCalcPressed()
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
    // get read access to the images
    mitk::ImageReadAccessor maskedReadAccessor(maskedImage);
    mitk::ImageReadAccessor selectionReadAccessor(selection);
    const auto maskedData = static_cast<const double*>(maskedReadAccessor.GetData());
    const auto selectionData = static_cast<const mitk::Label::PixelType*>(selectionReadAccessor.GetData());
    auto dims = maskedImage->GetDimensions();
    auto maskedDataSize = dims[0] * dims[1] * dims[2]; 
    // get iterators
    auto maskedDataIter = maskedData;
    auto selectionDataIter = selectionData;
    // all mz values corresponding to label one will be stored in valuesLabelOne
    std::vector<double> valuesLabelOne;
    std::vector<double> valuesLabelTwo;
    valuesLabelOne.reserve(maskedDataSize/2);
    valuesLabelTwo.reserve(maskedDataSize/2);
    for (; maskedDataIter != maskedData + maskedDataSize; ++maskedDataIter, ++selectionDataIter)
    {
      // add to valuesLabelOne
      if (*selectionDataIter == 1) //TODO: robustness: replace 1 with the label's corresponding value
      {
        valuesLabelOne.push_back(*maskedDataIter);
      }
      // add to valuesLabelTwo
      else if (*selectionDataIter == 2) //TODO: robustness: replace 2 with the label's corresponding value
      {
        valuesLabelTwo.push_back(*maskedDataIter);
      }
    }
    valuesLabelOne.shrink_to_fit();
    valuesLabelTwo.shrink_to_fit();
    //calculate the thresholds
    constexpr int granularity = 20;
    std::vector<double> thresholds;
    thresholds.reserve(granularity);
    thresholds.push_back( std::min(
      *std::min_element(valuesLabelOne.begin(), valuesLabelOne.end()),
      *std::min_element(valuesLabelTwo.begin(), valuesLabelTwo.end()))
    );
    //last threshold
    double tn = std::max(
      *std::max_element(valuesLabelOne.begin(), valuesLabelOne.end()),
      *std::max_element(valuesLabelTwo.begin(), valuesLabelTwo.end())
    );
    for (int i = 1; i < granularity - 1; ++i)
    {
      thresholds.push_back( thresholds[0] + (tn - thresholds[0]) * i / granularity );
    }
    thresholds.push_back(tn);

    //true positive and false negative rate are going to be the axis to plot the graph
    std::vector<double> TPR;
    TPR.reserve(granularity);
    std::vector<double> FNR;
    FNR.reserve(granularity);
    //calculate TPR and FNR for every threshold
    for (auto& threshold : thresholds)
    {
      double TP = 0, TN = 0, FN = 0;
      for (auto& valueLabelOne : valuesLabelOne)
      {
        if (valueLabelOne < threshold)
        {
          FN += 1;
        } 
        else
        {
          TP += 1;
        }
      }
      for (auto& valueLabelTwo : valuesLabelTwo)
      {
        if (valueLabelTwo < threshold)
        {
          TN += 1;
        } 
      }
      TPR.push_back( 1 - TP / (TP + TN) );
      FNR.push_back( FN / (FN + TP) );
    }
    RenderChart(TPR, FNR);
    //TODO: put data into a diagram
    MITK_INFO << ROC_SIG << "original dt: " << originalImage->GetPixelType().GetTypeAsString();
    MITK_INFO << ROC_SIG << "masked dt: " << maskedImage->GetPixelType().GetTypeAsString();
    MITK_INFO << ROC_SIG << "data size: " << maskedDataSize;
    MITK_INFO << ROC_SIG << "data[0] : " << maskedData[0];
    auto peaks = originalImage->GetPeaks();
  }
}
