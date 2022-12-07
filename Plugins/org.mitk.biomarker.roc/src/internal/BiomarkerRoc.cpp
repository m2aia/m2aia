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

// Qt
#include <QDialog>
#include <QLineSeries>
#include <QMessageBox>
#include <QValueAxis>
#include <QtCharts/QChartView>

// mitk
#include <mitkImage.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkLabelSetImage.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>

// m2aia
#include <m2ImzMLSpectrumImage.h>
#include <m2BiomarkerIdentificationAlgorithms.h>
#include <m2SpectrumImageBase.h>
#include <m2Peak.h>
#include <m2UIUtils.h>

// std
#include <array>
#include <tuple>

const std::string BiomarkerRoc::VIEW_ID = "org.mitk.views.biomarkerrocanalysis";

BiomarkerRoc::BiomarkerRoc() : m_Image(nullptr), m_MaskData(nullptr), m_ImageData(nullptr), m_ImageDataSize(0) {}

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
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.selection->SetDataStorage(GetDataStorage());
  m_Controls.selection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::LabelSetImage>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.image->SetSelectionIsOptional(false);
  m_Controls.image->SetInvalidInfo("Choose image");
  m_Controls.image->SetAutoSelectNewNodes(true);
  m_Controls.image->SetPopUpTitel("Select image");
  m_Controls.image->SetPopUpHint("Select the image you want to work with. This can be any opened image (*.imzML).");
  m_Controls.selection->SetSelectionIsOptional(false);
  m_Controls.selection->SetInvalidInfo("Choose selection");
  m_Controls.selection->SetAutoSelectNewNodes(true);
  m_Controls.selection->SetPopUpTitel("Select selection");
  m_Controls.selection->SetPopUpHint(
    "Choose the selection you want to work with. This can be any currently opened selection.");
  connect(m_Controls.buttonCalc, &QPushButton::clicked, this, &BiomarkerRoc::OnButtonCalcPressed);
  connect(m_Controls.buttonChart, &QPushButton::clicked, this, &BiomarkerRoc::OnButtonRenderChartPressed);
  const auto tableHandler = [this] (const QTableWidgetItem* item) {
    auto node = m_Controls.image->GetSelectedNode();
    if (auto spImage = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
    {
      auto mz = std::stod(item->text().toStdString());
      emit m2::UIUtils::Instance()->UpdateImage(mz, spImage->ApplyTolerance(mz));
    }
  };
  connect(m_Controls.tableWidget, &QTableWidget::itemDoubleClicked, this, tableHandler);
  connect(m_Controls.buttonOpenPeakPickingView, &QCommandLinkButton::clicked, this, [] {
    try
    {
      if (auto platform = berry::PlatformUI::GetWorkbench())
        if (auto workbench = platform->GetActiveWorkbenchWindow())
          if (auto page = workbench->GetActivePage())
            page->ShowView("org.mitk.views.m2.PeakPicking");
    }
    catch (berry::PartInitException &e)
    {
      BERRY_ERROR << "Error: " << e.what() << std::endl;
    }
  });
}

void BiomarkerRoc::OnButtonCalcPressed()
{
  //DoRocAnalysisFast();
  DoRocAnalysisWithThresholds();
  m_Controls.tableWidget->sortItems(1, Qt::DescendingOrder);
}

void BiomarkerRoc::DoRocAnalysisWithThresholds()
{
  // initialize
  auto imageNode = m_Controls.image->GetSelectedNode();
  auto maskNode = m_Controls.selection->GetSelectedNode();
  if (!imageNode || !maskNode)
    return;
  auto originalImage = dynamic_cast<m2::ImzMLSpectrumImage *>(imageNode->GetData());
  auto mask = dynamic_cast<mitk::Image *>(maskNode->GetData());
  m_Image = mitk::Image::New(); // image to which the mask will be applied to
  m_Image->Initialize(originalImage);
  mitk::ImageReadAccessor readAccessorMask(mask);
  m_MaskData = static_cast<const mitk::Label::PixelType *>(readAccessorMask.GetData());
  auto peaks = originalImage->GetPeaks();

  //m_timer.storage = 0;
  for (auto& peak : peaks)
  {
    //m_timer.start();
    //prepare state for PrepareTumorVectors
    double mz = peak.GetX();
    originalImage->GetImage(mz, m_Tolerance, mask, m_Image);
    mitk::ImageReadAccessor imagereader(m_Image);
    auto dims = m_Image->GetDimensions();
    m_ImageDataSize = dims[0] * dims[1] * dims[2];
    m_ImageData = static_cast<const double*>(imagereader.GetData());

    auto [A, B, P, N] = PrepareTumorVectors();
    
    std::array<double, m_numThresholds> thresholds;
    thresholds[0] = std::min(
      *std::min_element(A.begin(), A.end()),
      *std::min_element(B.begin(), B.end())
    );
    thresholds[thresholds.size() - 1] = std::max(
      *std::max_element(A.begin(), A.end()),
      *std::max_element(B.begin(), B.end())
    );
    for (size_t i = 1; i < m_numThresholds - 1; ++i)
    {
      thresholds[i] = thresholds[0] + (thresholds[thresholds.size() - 1] - thresholds[0]) * i / m_numThresholds;
    }
    std::array<double, m_numThresholds> TPR, FPR;
    for (size_t i = 0; i < m_numThresholds; ++i)
    {
      double TP = 0, TN = 0, FN = 0, FP = 0;
      for (const auto& positive : A)
      {
        if (positive < thresholds[i])
          FN += 1;
        else
          TP += 1;
      }
      for (const auto& negative : B)
      {
        if (negative < thresholds[i])
          TN += 1;
        else
          FP += 1;
      }
      TPR[i] = (TP / (TP + FN));
      FPR[i] = (FP / (FP + TN));
    }
    double auc = m2::BiomarkerIdentificationAlgorithms::AucTrapezoid(TPR.begin(), TPR.end(), FPR.begin());
    AddToTable(mz, auc);
    //m_timer.storage += m_timer.getDuration();
  }
  //MITK_INFO << ROC_SIG << "Dauer: " << m_timer.storage << " µs (" << m_timer.storage / 1000.0 << ") ms. " << m_timer.num_measurements << " Messungen durchgeführt.";
  m_Controls.tableWidget->setVisible(true);
}

void BiomarkerRoc::DoRocAnalysisMannWhitneyU()
{
  // initialize
  auto imageNode = m_Controls.image->GetSelectedNode();
  auto maskNode = m_Controls.selection->GetSelectedNode();
  if (!imageNode || !maskNode)
    return;
  auto originalImage = dynamic_cast<m2::ImzMLSpectrumImage *>(imageNode->GetData());
  auto mask = dynamic_cast<mitk::Image *>(maskNode->GetData());
  m_Image = mitk::Image::New(); // image to which the mask will be applied to
  m_Image->Initialize(originalImage);
  mitk::ImageReadAccessor readAccessorMask(mask);
  m_MaskData = static_cast<const mitk::Label::PixelType *>(readAccessorMask.GetData());
  auto peaks = originalImage->GetPeaks();
  for (auto& peak : peaks)
  {
    //m_timer.start();
    //prepare state for GetLabeledMz
    double mz = peak.GetX();
    originalImage->GetImage(mz, m_Tolerance, mask, m_Image);
    mitk::ImageReadAccessor imagereader(m_Image);
    auto dims = m_Image->GetDimensions();
    m_ImageDataSize = dims[0] * dims[1] * dims[2];
    m_ImageData = static_cast<const double*>(imagereader.GetData());

    auto [D, P, N] = GetLabeledMz();
    
    //ROC has finished already here, this is merely calculating the AUC
    double auc = m2::BiomarkerIdentificationAlgorithms::MannWhitneyU(D.begin(), D.end(), P, N);
    AddToTable(mz, auc);
    //m_timer.storage += m_timer.getDuration();
  }
  m_Controls.tableWidget->setVisible(true);
  //MITK_INFO << ROC_SIG << "Dauer: " << m_timer.storage << " µs (" << m_timer.storage / 1000.0 << ") ms. " << m_timer.num_measurements << " Messungen durchgeführt.";
}

void BiomarkerRoc::OnButtonRenderChartPressed()
{
  double mz = m_Controls.mzValue->value();
  //initialize image with new mz val
  {
    auto imageNode = m_Controls.image->GetSelectedNode();
    auto maskNode = m_Controls.selection->GetSelectedNode();
    auto originalImage = dynamic_cast<m2::ImzMLSpectrumImage *>(imageNode->GetData());
    auto mask = dynamic_cast<mitk::Image *>(maskNode->GetData());
    if (!m_Image)
      m_Image = mitk::Image::New();
    m_Image->Initialize(originalImage);
    originalImage->GetImage(mz, m_Tolerance, mask, m_Image); // write in m_Image
    // get read access to the images
    mitk::ImageReadAccessor readAccessorImage(m_Image);
    mitk::ImageReadAccessor readAccessorMask(mask);
    m_ImageData = static_cast<const double *>(readAccessorImage.GetData());
    m_MaskData = static_cast<const mitk::Label::PixelType*>(readAccessorMask.GetData());
    auto dims = m_Image->GetDimensions();
    m_ImageDataSize = dims[0] * dims[1] * dims[2];
  }
  // get P and N
  auto [D, P, N] = GetLabeledMz();
  auto [TrueRates, AUC] = m2::BiomarkerIdentificationAlgorithms::AucTrapezoidExtraData(D.begin(), D.end(), P, N);

  char auc[16] = {0};
  snprintf(auc, 15, "%s%lf", "AUC: ", AUC);
  m_Controls.labelAuc->setText(auc);

  auto serie = new QtCharts::QLineSeries();
  for (size_t idx = 0; idx < TrueRates.size(); ++idx)
  {
    auto [fpr, tpr] = TrueRates[idx];
    serie->append(fpr, tpr);
    // MITK_INFO << ROC_SIG << "[" << idx << "] FPR: " << fpr << ", TPR: " << tpr;
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

void BiomarkerRoc::AddToTable(double mz, double auc)
{
  int size = m_Controls.tableWidget->rowCount();
  m_Controls.tableWidget->setRowCount(size + 1);
  char mztext[16] = {0};
  snprintf(mztext, 16, "%lf", mz);
  auto widget = new QTableWidgetItem(mztext);
  widget->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  m_Controls.tableWidget->setItem(size, 0, widget);
  char auctext[16] = {0};
  snprintf(auctext, 16, "%lf", auc);
  auto aucwidget = new QTableWidgetItem(auctext);
  aucwidget->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  m_Controls.tableWidget->setItem(size, 1, aucwidget);
}

std::tuple<std::vector<double>, std::vector<double>, size_t, size_t> BiomarkerRoc::PrepareTumorVectors()
{
  // prepare data for ROC
  size_t P = 0; //NUM TUMOR
  size_t N = 0; //NUM NONTUMOR
  std::vector<double> A, B;
  A.reserve(m_ImageDataSize);
  B.reserve(m_ImageDataSize);
  auto maskIter = m_MaskData;
  for (auto imageIter = m_ImageData; imageIter != m_ImageData + m_ImageDataSize; ++maskIter, ++imageIter)
  {
    if (*maskIter == 1)
    {
      A.push_back(*imageIter); // positives
      ++P;
    }
    else if (*maskIter == 2)
    {
      B.push_back(*imageIter); // negatives
      ++N;
    }
  }
  A.shrink_to_fit();
  B.shrink_to_fit();
  return std::make_tuple(A, B, P, N);
}

std::tuple<std::vector<std::tuple<double, bool>>, size_t, size_t> BiomarkerRoc::GetLabeledMz()
{
  auto [A, B, P, N] = PrepareTumorVectors();
  constexpr const bool TUMOR = true;
  constexpr const bool NONTUMOR = false;
  std::vector<std::tuple<double, bool>> D;
  D.reserve(P + N);
  for (size_t i = 0; i < A.size(); ++i)
    D.push_back(std::make_tuple(A[i], TUMOR));
  for (size_t i = 0; i < B.size(); ++i)
    D.push_back(std::make_tuple(B[i], NONTUMOR));
  std::sort(D.begin(), D.end(), [](const std::tuple<double, bool>& a, const std::tuple<double, bool>& b)
  {
    auto [da, ba] = a;
    auto [db, bb] = b;
    return da < db;
  });
  return std::make_tuple(D, P, N);
}
/*
  if (false) // as of now this is considered a debug feature
  {
    // add new image to DataManager
    auto dataNode = mitk::DataNode::New();
    dataNode->SetData(m_Image);
    dataNode->SetName("Biomarker Roc data");
    GetDataStorage()->Add(dataNode, m_Controls.image->GetSelectedNode());
  }
*/