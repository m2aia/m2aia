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

//std
#include <memory>
#include <algorithm>

// for logging purposes
#define ROC_SIG "[BiomarkerRoc] "

const std::string BiomarkerRoc::VIEW_ID = "org.mitk.views.biomarkerrocanalysis";
const int BiomarkerRoc::renderGranularity = 20;
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
  m_Controls.tableWidget->setVisible(false);
  m_Controls.tableWidget->setColumnCount(2);
  m_Controls.tableWidget->setRowCount(0);
  m_Controls.tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem("m/z"));
  m_Controls.tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem("AUC"));
  m_Controls.chartView->setVisible(false);
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
  m_Controls.image->SetInvalidInfo("Choose image");
  m_Controls.image->SetAutoSelectNewNodes(true);
  m_Controls.image->SetPopUpTitel("Select image");
  m_Controls.image->SetPopUpHint("Select the image you want to work with. This can be any opened image (*.imzML).");
  m_Controls.selection->SetSelectionIsOptional(false);
  m_Controls.selection->SetInvalidInfo("Choose selection");
  m_Controls.selection->SetAutoSelectNewNodes(true);
  m_Controls.selection->SetPopUpTitel("Select selection");
  m_Controls.selection->SetPopUpHint("Choose the selection you want to work with. This can be any currently opened selection.");
  connect(m_Controls.buttonCalc, &QPushButton::clicked, this, &BiomarkerRoc::OnButtonCalcPressed);
  connect(m_Controls.buttonChart, &QPushButton::clicked, this, &BiomarkerRoc::RenderChart);
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

void BiomarkerRoc::AddToTable(const double& mz, const double& auc)
{
  int size = m_Controls.tableWidget->rowCount();
  m_Controls.tableWidget->setRowCount(size + 1);
  char mztext[16] = {0};
  snprintf(mztext, 16, "%lf", mz);
  auto mzlabel = new QLabel();
  mzlabel->setText(mztext);
  m_Controls.tableWidget->setCellWidget(size, 0, mzlabel);
  char auctext[16] = {0};
  snprintf(auctext, 16, "%lf", auc);
  auto auclabel = new QLabel();
  auclabel->setText(auctext);
  m_Controls.tableWidget->setCellWidget(size, 1, auclabel);
}

void BiomarkerRoc::RenderChart()
{
  std::vector<double> TPR;
  TPR.reserve(this->renderGranularity / 2);
  std::vector<double> FPR;
  FPR.reserve(this->renderGranularity / 2);
  RocAnalysis(m_Controls.mzValue->value(), TPR, FPR);

  auto A = CalculateAUC(TPR, FPR);
  char auc[16] = {0};
  snprintf(auc, 15, "%s%lf", "AUC: ", A);
  m_Controls.labelAuc->setText(auc);

  auto serie = new QtCharts::QLineSeries();
  int T = 0, F = 0;
  for (size_t idx = 0; idx < FPR.size(); ++idx)
  {
    serie->append(FPR[idx], TPR[idx]);
    if (TPR[idx] > FPR[idx]) ++T; else ++F;
    //MITK_INFO << ROC_SIG << "[" << idx << "] FPR: " << FPR[idx] << ", TPR: " << TPR[idx];
  }
  auto chart = new QtCharts::QChart();
  chart->addSeries(serie);
  auto axisX = new QValueAxis(); 
  axisX->setMin(0);
  axisX->setMax(1);
  auto axisY = new QValueAxis(); 
  axisY->setMin(0);
  axisY->setMax(1);
  chart->addAxis(axisX, Qt::AlignBottom);
  chart->addAxis(axisY, Qt::AlignLeft);
  chart->setTheme(QtCharts::QChart::ChartTheme::ChartThemeDark);
  m_Controls.chartView->setChart(chart);
  m_Controls.chartView->update();
  m_Controls.chartView->setVisible(true);
}

struct Timer 
{
  Timer()
  {
    time = std::chrono::high_resolution_clock::now();
  }
  ~Timer()
  {
    auto stop_time = std::chrono::high_resolution_clock::now();
    auto start = std::chrono::time_point_cast<std::chrono::microseconds>(time).time_since_epoch().count();
    auto end = std::chrono::time_point_cast<std::chrono::microseconds>(stop_time).time_since_epoch().count();
    auto duration = end - start;
    MITK_INFO << ROC_SIG << "execution took " << duration << " microseconds";
  }
private:
  std::chrono::_V2::system_clock::time_point time;
};

double BiomarkerRoc::CalculateAUC(const std::vector<double>& TPR, const std::vector<double>& FPR)
{
  // duration: ca 1 microsecond
  // trapezoid approximation
  double A = 0;
  for (std::size_t i = 0; i < FPR.size() - 1; ++i)
  {
    //      C
    //    / |
    //  D   |a 
    // c| h |   A = (a + c)/2 * h
    //  A - B
    double a = TPR[i + 1], c = TPR[i], h = FPR[i] - FPR[i + 1];
    A += (a + c) / 2 * h;
  }
  return A;
}

  
void BiomarkerRoc::RocAnalysis(const double& mz, std::vector<double>& TPR, std::vector<double>& FPR)
{
  auto imageNode = m_Controls.image->GetSelectedNode();
  auto selectionNode = m_Controls.selection->GetSelectedNode();
  if (!imageNode || !selectionNode) return;

  auto originalImage = dynamic_cast<m2::ImzMLSpectrumImage*>(imageNode->GetData());
  auto selection = dynamic_cast<mitk::Image*>(selectionNode->GetData());
  auto maskedImage = mitk::Image::New(); // image to which the selection will be applied to
  auto channelDescriptor = selection->GetChannelDescriptor();
  {
    maskedImage->Initialize(originalImage);
    originalImage->GetImage(mz, 0.45, selection, maskedImage); //write in maskedImage
    if(false)
    {
      auto dataNode = mitk::DataNode::New();
      dataNode->SetData(maskedImage);
      dataNode->SetName("Biomarker Roc data");
      GetDataStorage()->Add(dataNode, m_Controls.image->GetSelectedNode());
    }
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
  std::vector<double> thresholds;
  thresholds.reserve(this->renderGranularity);
  thresholds.push_back( std::min(
    *std::min_element(valuesLabelOne.begin(), valuesLabelOne.end()),
    *std::min_element(valuesLabelTwo.begin(), valuesLabelTwo.end()))
  );
  //last threshold
  double tn = std::max(
    *std::max_element(valuesLabelOne.begin(), valuesLabelOne.end()),
    *std::max_element(valuesLabelTwo.begin(), valuesLabelTwo.end())
  );
  for (int i = 1; i < this->renderGranularity - 1; ++i)
  {
    thresholds.push_back( thresholds[0] + (tn - thresholds[0]) * i / this->renderGranularity );
  }
  thresholds.push_back(tn);
  //true positive and false negative rate are going to be the axis to plot the graph
  //calculate TPR and FPR for every threshold
  for (const auto& threshold : thresholds)
  {
    double TP = 0, TN = 0, FN = 0, FP = 0;
    for (const auto& valueLabelOne : valuesLabelOne)
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
    for (const auto& valueLabelTwo : valuesLabelTwo)
    {
      if (valueLabelTwo < threshold)
      {
        TN += 1;
      } 
      else
      {
        FP += 1;
      }
    }
    TPR.push_back( TP / (TP + FN) );
    FPR.push_back( FP / (FP + TN) );
  }
}

void BiomarkerRoc::OnButtonCalcPressed()
{
  auto imageNode = m_Controls.image->GetSelectedNode();
  auto originalImage = dynamic_cast<m2::ImzMLSpectrumImage*>(imageNode->GetData());
  //filters the selection for one mz value, displays the resulting image afterwards
  auto peaks = originalImage->GetPeaks();
  for (auto& peak : peaks) 
  {
    double mz = peak.GetX();
    std::vector<double> TPR;
    TPR.reserve(this->renderGranularity);
    std::vector<double> FPR;
    FPR.reserve(this->renderGranularity);
    RocAnalysis(mz, TPR, FPR);
    double auc = CalculateAUC(TPR, FPR);
    AddToTable(mz, auc);
  }
  m_Controls.tableWidget->setVisible(true);
}
