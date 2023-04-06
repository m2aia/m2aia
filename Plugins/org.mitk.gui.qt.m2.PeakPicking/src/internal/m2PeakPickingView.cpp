/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>
#include <m2ElxUtil.h>
// Qmitk
#include "m2PeakPickingView.h"

// Qt
#include <QFileDialog>
#include <QMessageBox>

// boost
#include <boost/algorithm/string.hpp>
#include <boost/any.hpp>

// m2
#include <m2CoreCommon.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2IntervalVector.h>
#include <m2UIUtils.h>
#include <signal/m2Binning.h>
#include <signal/m2MedianAbsoluteDeviation.h>
#include <signal/m2PeakDetection.h>

// itk
#include <itkExtractImageFilter.h>
#include <itkFixedArray.h>
#include <itkImageAlgorithm.h>

// mitk image
#include <QmitkSingleNodeSelectionWidget.h>
#include <mitkImage.h>
#include <mitkImageAccessByItk.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkLabelSetImage.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateFunction.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkProgressBar.h>
#include <mitkProperties.h>

const std::string m2PeakPickingView::VIEW_ID = "org.mitk.views.m2.PeakPicking";

void m2PeakPickingView::SetFocus()
{
  OnUpdateUI();
  OnStartPeakPicking();
}

void m2PeakPickingView::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  m_Controls.cbOverviewSpectra->addItems({"Skyline", "Mean"});
  m_Controls.cbOverviewSpectra->setCurrentIndex(1);

  m_Controls.sliderCOR->setMinimum(0.0);
  m_Controls.sliderCOR->setMaximum(1.0);
  m_Controls.sliderCOR->setValue(0.95);
  m_Controls.sliderCOR->setSingleStep(0.01);

  m_Controls.imageSelection->SetDataStorage(GetDataStorage());
  m_Controls.imageSelection->SetAutoSelectNewNodes(true);
  m_Controls.imageSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::SpectrumImageBase>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.imageSelection->SetSelectionIsOptional(true);
  m_Controls.imageSelection->SetEmptyInfo(QString("Image selection"));
  m_Controls.imageSelection->SetPopUpTitel(QString("Image"));

  auto isChildOfImageNode = [this](const mitk::DataNode *node) {
    if (auto imageNode = m_Controls.imageSelection->GetSelectedNode())
    {
      auto childNodes = GetDataStorage()->GetDerivations(imageNode);
      using namespace std;
      return find(begin(*childNodes), end(*childNodes), node) != end(*childNodes);
    }
    return false;
  };

  m_Controls.peakListSelection->SetDataStorage(GetDataStorage());
  m_Controls.peakListSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::IntervalVector>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object")),
                                mitk::NodePredicateFunction::New(isChildOfImageNode)));

  m_Controls.peakListSelection->SetAutoSelectNewNodes(true);
  m_Controls.peakListSelection->SetSelectionIsOptional(true);
  m_Controls.peakListSelection->SetEmptyInfo(QString("PeakList selection"));
  m_Controls.peakListSelection->SetPopUpTitel(QString("PeakList"));

  auto tableWidget = m_Controls.tableWidget;
  connect(m_Controls.btnSelectAll, &QAbstractButton::clicked, m_Controls.tableWidget, [tableWidget]() mutable {
    for (int i = 0; i < tableWidget->rowCount(); ++i)
      tableWidget->item(i, 0)->setCheckState(Qt::CheckState::Checked);
  });

  connect(m_Controls.btnDeselectAll, &QAbstractButton::clicked, m_Controls.tableWidget, [tableWidget]() mutable {
    for (int i = 0; i < tableWidget->rowCount(); ++i)
      tableWidget->item(i, 0)->setCheckState(Qt::CheckState::Unchecked);
  });

  const auto itemHandler = [this](QTableWidgetItem *item) {
    auto mz = std::stod(item->text().toStdString());
    emit m2::UIUtils::Instance()->UpdateImage(mz, -1);    
  };

  connect(m_Controls.tableWidget, &QTableWidget::itemActivated, this, itemHandler);
  connect(m_Controls.tableWidget, &QTableWidget::itemDoubleClicked, this, itemHandler);
  connect(m_Controls.sbTolerance, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPicking()));
  connect(m_Controls.sbDistance, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPicking()));
  connect(m_Controls.cbOverviewSpectra, SIGNAL(currentIndexChanged(int)), this, SLOT(OnStartPeakPicking()));
  connect(m_Controls.cbOverviewSpectra, SIGNAL(currentIndexChanged(int)), this, SLOT(OnUpdateUI()));
  connect(m_Controls.sliderSNR, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPicking()));
  connect(m_Controls.sliderHWS, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPicking()));
  connect(m_Controls.sliderCOR, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPicking()));
  connect(m_Controls.ckbMonoisotopic, SIGNAL(stateChanged(int)), this, SLOT(OnStartPeakPicking()));
  connect(m_Controls.btnPickAndBin, SIGNAL(clicked()), this, SLOT(OnStartPickPeaksAndBin()));
  connect(m_Controls.btnAddPeakList, &QAbstractButton::clicked, this, &m2PeakPickingView::OnAddPeakList);

  connect(m_Controls.imageSelection, &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged, this, [this]() {
    m_Controls.peakListSelection->SetCurrentSelectedNode(nullptr);
    m_Controls.peakListSelection->SetAutoSelectNewNodes(true);
    OnUpdateUI();
  });

  connect(m_Controls.peakListSelection, &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged, this, [this]() {
    OnUpdateUI();
    OnUpdatePeakListImage();
    OnUpdatePeakListUI();
    OnPeakListSelectionChanged();
  });

  // connect(m_Controls.btnRunSparsePCA, SIGNAL(clicked()), this, SLOT(OnStartSparsePCA()));
  // connect(m_Controls.btnStartPeakPicking, &QCommandLinkButton::clicked, this,
  // &m2PeakPickingView::OnStartPeakPicking);
  // connect(m_Controls.btnPCA, &QCommandLinkButton::clicked, this, &m2PeakPickingView::OnStartPCA);
  // connect(m_Controls.btnRunLasso, &QCommandLinkButton::clicked, this, &m2PeakPickingView::OnStartLasso);
  // connect(m_Controls.btnRunUmap, &QCommandLinkButton::clicked, this, &m2PeakPickingView::OnStartUMAP);
  // connect(m_Controls.btnTSNE, &QCommandLinkButton::clicked, this, &m2PeakPickingView::OnStartTSNE);
  // connect(m_Controls.btnRunMPM, &QCommandLinkButton::clicked, this, &m2PeakPickingView::OnStartMPM);

  // const auto dockerAvailable = mitk::DockerHelper::CheckDocker();
  // m_Controls.tabSparsePCA->setEnabled(dockerAvailable);
  // m_Controls.tabUmap->setEnabled(dockerAvailable);
  OnPeakListSelectionChanged();
}

void m2PeakPickingView::OnPeakListSelectionChanged()
{
  bool status = false, isProcessedCentroid = false, isContinuousProfile = false;
  if (auto imageNode = m_Controls.imageSelection->GetSelectedNode())
    if (auto image = dynamic_cast<m2::SpectrumImageBase *>(imageNode->GetData()))
      if (image->GetIsDataAccessInitialized())
      {
        auto format = image->GetSpectrumType().Format;
        isProcessedCentroid = (format == m2::SpectrumFormat::ProcessedCentroid);
        isProfile =
          (format == m2::SpectrumFormat::ContinuousProfile) || (format == m2::SpectrumFormat::ProcessedProfile);

        if (isProfile)
          m_Controls.btnPickAndBin->setText("Start peak picking and binning (individual spectra)");

        if (isProcessedCentroid)
          m_Controls.btnPickAndBin->setText("Start peak binning (individual spectra)");

        if (auto peakListNode = m_Controls.peakListSelection->GetSelectedNode())
          if (dynamic_cast<m2::IntervalVector *>(peakListNode->GetData()))
            status = true;
  
      }

  m_Controls.btnPickAndBin->setVisible(status & (isProfile | isProcessedCentroid));
  m_Controls.sbBinningTolerance->setVisible(status & (isProfile | isProcessedCentroid));
  m_Controls.sbFilterPeaks->setVisible(status & (isProfile | isProcessedCentroid));
  
  m_Controls.labelCOR->setVisible(status & isProfile);
  m_Controls.labelOverviewSpectra->setVisible(status & isProfile);
  m_Controls.labelHWS->setVisible(status & isProfile);
  m_Controls.labelSNR->setVisible(status & isProfile);
  m_Controls.labelTolerance->setVisible(status & isProfile);
  m_Controls.labelDistance->setVisible(status & isProfile);
  m_Controls.sliderSNR->setVisible(status & isProfile);
  m_Controls.sliderCOR->setVisible(status & isProfile);
  m_Controls.sliderHWS->setVisible(status & isProfile);
  m_Controls.sbTolerance->setVisible(status & isProfile);
  m_Controls.sbDistance->setVisible(status & isProfile);
  m_Controls.ckbMonoisotopic->setVisible(status & isProfile);
  m_Controls.cbOverviewSpectra->setVisible(status & isProfile);
  m_Controls.labelOverviewSpectra_2->setVisible(status & isProfile);
  
  m_Controls.labelPeakList->setVisible(status);
  m_Controls.btnSelectAll->setVisible(status);
  m_Controls.btnDeselectAll->setVisible(status);
  m_Controls.tableWidget->setVisible(status);
  
}

void m2PeakPickingView::OnUpdateUI()
{
  {
    auto imageNode = m_Controls.imageSelection->GetSelectedNode();
    if (!imageNode)
      return;

    auto image = dynamic_cast<m2::SpectrumImageBase *>(imageNode->GetData());
    if (!image)
      return;

    if (!image->GetIsDataAccessInitialized())
      return;

    MITK_INFO << "Update UI - valid image";

    if (image->GetSpectrumType().Format == m2::SpectrumFormat::ContinuousCentroid ||
        image->GetSpectrumType().Format == m2::SpectrumFormat::ProcessedCentroid)
    {
      m_Controls.labelCOR->setVisible(false);
      m_Controls.labelOverviewSpectra->setVisible(false);
      m_Controls.labelHWS->setVisible(false);
      m_Controls.labelSNR->setVisible(false);
      m_Controls.labelTolerance->setVisible(false);
      m_Controls.labelDistance->setVisible(false);

      m_Controls.sliderSNR->setVisible(false);
      m_Controls.sliderCOR->setVisible(false);
      m_Controls.sliderHWS->setVisible(false);
      m_Controls.sbTolerance->setVisible(false);
      m_Controls.sbDistance->setVisible(false);

      m_Controls.ckbMonoisotopic->setVisible(false);
      m_Controls.cbOverviewSpectra->setVisible(false);
    }
    
  }
}

// void m2PeakPickingView::OnStartLasso()
// {
//   if (auto node = m_Controls.imageSelection->GetSelectedNode())
//   {
//     if (auto image = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
//     {
//       if (!image->GetIsDataAccessInitialized())
//         return;
//       if (image->GetSpectrumType().Format == m2::SpectrumFormat::ContinuousCentroid)
//       {
//         if (mitk::DockerHelper::CheckDocker())
//         {
//           auto labelNode = m_Controls.lassoLabelImageSelection->GetSelectedNode();
//           if (labelNode)
//           {
//             mitk::DockerHelper helper("m2aia/extensions:lasso");
//             helper.AddAutoSaveData(image, "--imzml", "*.imzML");
//             helper.AddAutoSaveData(labelNode->GetData(), "--labels", "labels.nrrd");
//             helper.AddLoadLaterOutput("--importance", "test.csv");
//             helper.AddLoadLaterOutput("--metrics", "metrics.csv");

//             helper.AddAutoLoadOutput("--predictions", "predictions.nrrd");
//             helper.AddApplicationArgument("--limit_perf_loss",
//                                           std::to_string(m_Controls.spnBxLossPerformance->value()));
//             helper.AddApplicationArgument("--n_folds", std::to_string(m_Controls.spnBxFolds->value()));
//             const auto results = helper.GetResults();

//             {
//               auto newNode = mitk::DataNode::New();
//               newNode->SetData(results[0]);
//               newNode->SetName(node->GetName() + "_lasso");
//               GetDataStorage()->Add(newNode, node);
//             }
//           }
//         }
//       }
//     }
//   }
// }



void m2PeakPickingView::OnAddPeakList()
{
  auto imageNode = m_Controls.imageSelection->GetSelectedNode();
  if (!imageNode)
    return;

  auto image = dynamic_cast<m2::SpectrumImageBase *>(imageNode->GetData());
  if (!image)
    return;

  if (!image->GetIsDataAccessInitialized())
  {
    MITK_WARN << "Selected image is not initialized!";
    return;
  }

  auto vectorNode = mitk::DataNode::New();
  auto intervals = m2::IntervalVector::New();

  vectorNode->SetName(imageNode->GetName() + "_picked");
  vectorNode->SetData(intervals);
  GetDataStorage()->Add(vectorNode, imageNode);
  m_Controls.peakListSelection->SetCurrentSelectedNode(vectorNode);  
  OnStartPeakPicking();
}

void m2PeakPickingView::OnStartPickPeaksAndBin()
{
  auto imageNode = m_Controls.imageSelection->GetSelectedNode();
  if (!imageNode)
    return;

  auto image = dynamic_cast<m2::SpectrumImageBase *>(imageNode->GetData());
  if (!image)
    return;

  if (!image->GetIsDataAccessInitialized())
    return;

  // if (auto image = dynamic_cast<m2::ImzMLSpectrumImage *>(imageNode->GetData()))
  {
    using namespace std;
    using namespace m2::Signal;

    const auto snr = m_Controls.sliderSNR->value();
    const auto hws = m_Controls.sliderHWS->value();
    const auto minCor = m_Controls.sliderCOR->value();
    const auto tol = m_Controls.sbTolerance->value();
    const auto dist = m_Controls.sbDistance->value();
    const std::vector<unsigned int> sizes = {3, 4, 5, 6, 7, 8, 9, 10};
    // bool isMax;
    // if (m_Controls.cbOverviewSpectra->currentIndex() == 0) // skyline
    //   isMax = true;
    // if (m_Controls.cbOverviewSpectra->currentIndex() == 1) // mean
    //   isMax = false;

    auto N = image->GetXAxis().size() * image->GetNumberOfValidPixels();
    vector<float> xs, ys, xsAll, ysAll;
    vector<int> ssAll;
    //, idxAll;
    xsAll.reserve(N);
    ysAll.reserve(N);
    ssAll.reserve(N);
    // idxAll.reserve(N);

    auto xsAllIt = back_inserter(xsAll);
    auto ysAllIt = back_inserter(ysAll);
    auto ssAllIt = back_inserter(ssAll);
    // auto idxAllIt = back_inserter(idxAll);

    std::vector<m2::Interval> peaks;
    mitk::ProgressBar::GetInstance()->AddStepsToDo(image->GetNumberOfValidPixels());
    for (unsigned int i = 0; i < image->GetNumberOfValidPixels(); ++i)
    {
      image->GetSpectrum(i, xs, ys);
      if (image->GetSpectrumType().Format == m2::SpectrumFormat::ContinuousProfile)
      {
        // Pick peaks
        peaks.clear();
        const auto noise = m2::Signal::mad(ys);
        localMaxima(begin(ys), end(ys), begin(xs), back_inserter(peaks), hws, noise * snr);
        if (m_Controls.ckbMonoisotopic->isChecked())
          peaks = monoisotopic(peaks, sizes, minCor, tol, dist);

        for_each(begin(peaks), end(peaks), [&](m2::Interval &i) {
          xsAllIt = i.x.mean();
          ysAllIt = i.y.max();
        });
        fill_n(ssAllIt, peaks.size(), i);
      }
      else
      {
        copy(begin(xs), end(xs), xsAllIt);
        copy(begin(ys), end(ys), ysAllIt);
        fill_n(ssAllIt, xs.size(), i);
      }
      mitk::ProgressBar::GetInstance()->Progress();
    }

    auto indices = m2::argsort(xsAll);
    auto xsSorted = m2::argsortApply(xsAll, indices);
    auto ysSorted = m2::argsortApply(ysAll, indices);
    auto ssSorted = m2::argsortApply(ssAll, indices);
    // idxSorted = m2::argsortApply(idxAll, indices);

    using dIt = decltype(cbegin(xsAll));
    using iIt = decltype(cbegin(ssAll));
    auto Grouper = m2::Signal::grouperStrict<dIt, dIt, dIt, iIt>;
    // auto Grouper = m2::Signal::grouperRelaxed<dIt, dIt, dIt, iIt>;
    auto R = m2::Signal::groupBinning(xsSorted, ysSorted, ssSorted, Grouper, m_Controls.sbBinningTolerance->value());

    auto frequency = m_Controls.sbFilterPeaks->value() / double(100) * image->GetNumberOfValidPixels();
    auto I = get<0>(R);
    decltype(I) newI;

    copy_if(
      begin(I), end(I), back_inserter(newI), [frequency](const m2::Interval &in) { return in.x.count() > frequency; });
    MITK_INFO << "Freq " << frequency << " " << newI.size();

    auto vectorNode = m_Controls.peakListSelection->GetSelectedNode();
    if (!vectorNode)
    {
      auto vectorNode = mitk::DataNode::New();
      auto intervals = m2::IntervalVector::New();
      image->GetIntervals() = intervals->GetIntervals() = newI;
      vectorNode->SetName(imageNode->GetName() + "_binned");
      vectorNode->SetData(intervals);
      GetDataStorage()->Add(vectorNode, imageNode);
      return;
    }

    auto vector = dynamic_cast<m2::IntervalVector *>(vectorNode->GetData());
    if (!vector)
      return;

    vector->GetIntervals() = newI;
    OnUpdatePeakListImage();
    OnUpdatePeakListUI();
  }
}

void m2PeakPickingView::OnStartPeakPicking()
{
  // m_PeakList.clear();

  auto imageNode = m_Controls.imageSelection->GetSelectedNode();
  if (!imageNode)
    return;

  auto image = dynamic_cast<m2::SpectrumImageBase *>(imageNode->GetData());
  if (!image)
    return;

  if (!image->GetIsDataAccessInitialized())
    return;

  // check if the vectorNode is a child of the image node
  auto vectorNode = m_Controls.peakListSelection->GetSelectedNode();
  if (!vectorNode)
    return;

  auto vector = dynamic_cast<m2::IntervalVector *>(vectorNode->GetData());
  if (!vector)
    return;

  // Already peaks available, so start using those
  if (image->GetSpectrumType().Format == m2::SpectrumFormat::ContinuousCentroid ||
      image->GetSpectrumType().Format == m2::SpectrumFormat::ProcessedCentroid)
  {
    using namespace std;
    auto &I = vector->GetIntervals();
    const auto &xs = image->GetXAxis();
    const auto &ymax = image->SkylineSpectrum();
    // const auto &ymean = image->MeanSpectrum();
    const auto &ysum = image->SumSpectrum();
    I.resize(xs.size());
    for (unsigned int i = 0; i < xs.size(); ++i)
    {
      I[i].x(xs[i]);
      I[i].y.m_count = image->GetNumberOfValidPixels();
      I[i].y.m_max = ymax[i];
      I[i].y.m_min = 0;
      I[i].y.m_sum = ysum[i];
      // I[i].index(i);
    }
  }
  else if (image->GetSpectrumType().Format == m2::SpectrumFormat::ContinuousProfile ||
           image->GetSpectrumType().Format == m2::SpectrumFormat::ProcessedProfile)
  {
    // MITK_INFO << "ContinuousProfile";
    std::vector<m2::Interval> peaks;
    std::vector<double> ys, xs;
    if (m_Controls.cbOverviewSpectra->currentIndex() == 0) // skyline
      ys = image->SkylineSpectrum();
    if (m_Controls.cbOverviewSpectra->currentIndex() == 1) // mean
      ys = image->MeanSpectrum();
    xs = image->GetXAxis();
    using namespace std;
    using namespace m2::Signal;

    const auto noise = m2::Signal::mad(ys);
    const auto snr = m_Controls.sliderSNR->value();

    const auto hws = m_Controls.sliderHWS->value();
    const auto minCor = m_Controls.sliderCOR->value();
    const auto tol = m_Controls.sbTolerance->value();
    const auto dist = m_Controls.sbDistance->value();

    const std::vector<unsigned int> sizes = {3, 4, 5, 6, 7, 8, 9, 10};
    localMaxima(begin(ys), end(ys), begin(xs), back_inserter(peaks), hws, noise * snr);

    if (m_Controls.ckbMonoisotopic->isChecked())
      peaks = monoisotopic(peaks, sizes, minCor, tol, dist);

    vector->GetIntervals() = peaks;
  }

  // apply min intensity

  // auto &intervals = vector->GetIntervals();
  // using namespace std;
  // auto it = remove_if(begin(intervals), end(intervals), [this](const m2::Interval &I) {
  //   return I.y.mean() < m_Controls.sliderMinIntensity->value();
  // });
  // vector->GetIntervals().erase(it, vector->GetIntervals().end());

  OnUpdatePeakListImage();
  OnUpdatePeakListUI();
  // emit m2::UIUtils::Instance()->BroadcastProcessingNodes("", node);
}

void m2PeakPickingView::OnUpdatePeakListUILabel()
{
  auto vectorNode = m_Controls.peakListSelection->GetSelectedNode();
  if (!vectorNode)
  {
    m_Controls.labelPeakList->setText(QString("Peaks (%1/%2)").arg(0).arg(0));
    return;
  }

  auto vector = dynamic_cast<m2::IntervalVector *>(vectorNode->GetData());
  if (!vector)
    return;

  unsigned int selected = 0;
  for (int i = 0; i < m_Controls.tableWidget->rowCount(); ++i)
    if (m_Controls.tableWidget->item(i, 0)->checkState() == Qt::CheckState::Checked)
      selected += 1;
  m_Controls.labelPeakList->setText(QString("Peaks (%1/%2)").arg(selected).arg((int)vector->GetIntervals().size()));
}

void m2PeakPickingView::OnUpdatePeakListImage()
{
  auto imageNode = m_Controls.imageSelection->GetSelectedNode();
  if (!imageNode)
    return;

  auto image = dynamic_cast<m2::SpectrumImageBase *>(imageNode->GetData());
  if (!image)
    return;

  if (!image->GetIsDataAccessInitialized())
    return;

  auto vectorNode = m_Controls.peakListSelection->GetSelectedNode();
  if (!vectorNode)
  {
    image->GetIntervals().clear();
    image->PeakListModified();
    return;
  }

  auto vector = dynamic_cast<m2::IntervalVector *>(vectorNode->GetData());
  if (!vector)
    return;

  image->GetIntervals() = vector->GetIntervals();
  image->PeakListModified();
}

void m2PeakPickingView::OnUpdatePeakListUI()
{
  auto imageNode = m_Controls.imageSelection->GetSelectedNode();
  if (!imageNode)
  {
    m_Controls.tableWidget->clearContents();
    m_Controls.peakListSelection->SetCurrentSelectedNode(nullptr);
    OnUpdatePeakListUILabel();
    return;
  }

  auto vectorNode = m_Controls.peakListSelection->GetSelectedNode();
  if (!vectorNode)
  {
    m_Controls.tableWidget->clearContents();
    OnUpdatePeakListUILabel();
    return;
  }

  auto image = dynamic_cast<m2::SpectrumImageBase *>(imageNode->GetData());
  if (!image)
    return;

  if (!image->GetIsDataAccessInitialized())
    return;

  auto vector = dynamic_cast<m2::IntervalVector *>(vectorNode->GetData());
  if (!vector)
    return;

  const auto &I = vector->GetIntervals();
  m_Controls.tableWidget->clearContents();
  m_Controls.tableWidget->setRowCount(I.size());

  unsigned int row = 0;
  m_Controls.tableWidget->blockSignals(true);
  for (auto &p : I)
  {
    auto item = new QTableWidgetItem((std::to_string(p.x.mean()) + " [" + std::to_string(p.y.min()) + "; " +
                                      std::to_string(p.y.mean()) + "; " + std::to_string(p.y.max()) + "]")
                                       .c_str());
    item->setCheckState(Qt::CheckState::Checked);
    m_Controls.tableWidget->setItem(row++, 0, item);
  }
  m_Controls.tableWidget->blockSignals(false);
  OnUpdatePeakListUILabel();
}
