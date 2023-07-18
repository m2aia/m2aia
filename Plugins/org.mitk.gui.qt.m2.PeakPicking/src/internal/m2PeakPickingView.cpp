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
#include <QRandomGenerator>

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
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkProgressBar.h>
#include <mitkProperties.h>

const std::string m2PeakPickingView::VIEW_ID = "org.mitk.views.m2.PeakPicking";

void m2PeakPickingView::SetFocus()
{
  OnUpdateUI();
  OnStartPeakPickingOverview();
}

mitk::DataNode *m2PeakPickingView::GetParent(mitk::DataNode *child) const
{
  auto sources = this->GetDataStorage()->GetSources(child);
  if (sources && !sources->empty())
  {
    return sources->front().GetPointer();
  }
  return nullptr;
}

void m2PeakPickingView::CreateQtPartControl(QWidget *parent)
{
  using namespace std;

  auto NodePredicateIsCentroidSpectrum = mitk::NodePredicateFunction::New(
    [this](const mitk::DataNode *node) -> bool
    {
      if (auto intervals = dynamic_cast<m2::IntervalVector *>(node->GetData()))
        return ((unsigned int)(intervals->GetType())) & ((unsigned int)(m2::SpectrumFormat::Centroid));
      return false;
    });

  auto NodePredicateIsProfileSpectrum = mitk::NodePredicateFunction::New(
    [this](const mitk::DataNode *node) -> bool
    {
      if (auto intervals = dynamic_cast<m2::IntervalVector *>(node->GetData()))
        return ((unsigned int)(intervals->GetType())) & ((unsigned int)(m2::SpectrumFormat::Profile));
      return false;
    });

  auto NodePredicateIsOverviewSpectrum = mitk::NodePredicateFunction::New(
    [this](const mitk::DataNode *node)
    {
      if (auto intervals = dynamic_cast<m2::IntervalVector *>(node->GetData()))
        return intervals->GetInfo().find("overview") != std::string::npos;
      return false;
    });

  auto NodePredicateNoActiveHelper = mitk::NodePredicateNot::New(m_NodePredicateIsActiveHelperNode);

  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  // m_Controls.peakPickingControls->setAutoFillBackground(false);
  m_Controls.peakPickingControls->setWindowOpacity(1);

  m_Controls.sliderCOR->setMinimum(0.0);
  m_Controls.sliderCOR->setMaximum(1.0);
  m_Controls.sliderCOR->setValue(0.95);
  m_Controls.sliderCOR->setSingleStep(0.01);

  // m_Controls.targetCenroidSelector->SetDataStorage(GetDataStorage());
  // m_Controls.targetCenroidSelector->SetSelectionIsOptional(true);
  // m_Controls.targetCenroidSelector->SetEmptyInfo(QString("Target Spectrum"));
  // m_Controls.targetCenroidSelector->SetPopUpTitel(QString("Target Spectrum"));
  // m_Controls.targetCenroidSelector->SetNodePredicate(
  //   mitk::NodePredicateAnd::New(NodePredicateIsCentroidSpectrum,
  //                               NodePredicateNoActiveHelper,
  //                               mitk::NodePredicateNot::New(NodePredicateIsOverviewSpectrum)));

  m_Controls.sourceProfileSelector->SetDataStorage(GetDataStorage());
  m_Controls.sourceProfileSelector->SetSelectionIsOptional(true);
  m_Controls.sourceProfileSelector->SetAutoSelectNewNodes(true);
  m_Controls.sourceProfileSelector->SetEmptyInfo(QString("Select Overview Spectrum"));
  m_Controls.sourceProfileSelector->SetPopUpTitel(QString("Select Overview Spectrum"));
  m_Controls.sourceProfileSelector->SetNodePredicate(mitk::NodePredicateAnd::New(
    NodePredicateNoActiveHelper, NodePredicateIsOverviewSpectrum, NodePredicateIsProfileSpectrum));

  m_Controls.sourceMultipleCenroidsSelector->SetDataStorage(GetDataStorage());
  m_Controls.sourceMultipleCenroidsSelector->SetSelectionIsOptional(true);
  // m_Controls.sourceCenroidSelector->SetAutoSelectNewNodes(true);
  m_Controls.sourceMultipleCenroidsSelector->SetEmptyInfo(QString("Select Source Centroid Spectra"));
  m_Controls.sourceMultipleCenroidsSelector->SetPopUpTitel(QString("Select Source Centroid Spectra"));
  m_Controls.sourceMultipleCenroidsSelector->SetNodePredicate(
    mitk::NodePredicateAnd::New(NodePredicateIsCentroidSpectrum, NodePredicateNoActiveHelper));

  m_Controls.sourceCentroidsSelector->SetDataStorage(GetDataStorage());
  m_Controls.sourceCentroidsSelector->SetSelectionIsOptional(true);
  m_Controls.sourceCentroidsSelector->SetAutoSelectNewNodes(true);
  m_Controls.sourceCentroidsSelector->SetEmptyInfo(QString("Select Source Centroid Spectrum"));
  m_Controls.sourceCentroidsSelector->SetPopUpTitel(QString("Select Source Centroid Spectrum"));
  m_Controls.sourceCentroidsSelector->SetNodePredicate(
    mitk::NodePredicateAnd::New(NodePredicateIsCentroidSpectrum, NodePredicateNoActiveHelper));

  m_Controls.sourceProfileImageSelector->SetDataStorage(GetDataStorage());
  m_Controls.sourceProfileImageSelector->SetSelectionIsOptional(true);
  m_Controls.sourceProfileImageSelector->SetAutoSelectNewNodes(true);
  m_Controls.sourceProfileImageSelector->SetEmptyInfo(QString("Select Source Profile Spectrum Image"));
  m_Controls.sourceProfileImageSelector->SetPopUpTitel(QString("Select Source Profile Spectrum Image"));
  m_Controls.sourceProfileImageSelector->SetNodePredicate(m_NodePredicateIsProfileSpectrumImage);

  connect(m_Controls.sbTolerance, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPickingOverview()));
  connect(m_Controls.sbDistance, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPickingOverview()));
  connect(m_Controls.sliderSNR, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPickingOverview()));
  connect(m_Controls.sliderHWS, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPickingOverview()));
  connect(m_Controls.sliderCOR, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPickingOverview()));
  connect(
    m_Controls.btnPickPeaksPixelWise, &QAbstractButton::clicked, this, &m2PeakPickingView::OnStartPeakPickingImage);

  parent->layout()->removeWidget(m_Controls.peakPickingControls);
  parent->layout()->removeWidget(m_Controls.binningControls);

  connect(m_Controls.tabWidget, &QTabWidget::currentChanged, this, &m2PeakPickingView::OnCurrentTabChanged);
  m_Controls.tabWidget->setCurrentIndex(0);

  connect(m_Controls.sourceProfileSelector,
          &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged,
          this,
          [this]() { OnUpdateUI(); });

  connect(m_Controls.sourceProfileSelector,
          &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged,
          this,
          [this]() { OnUpdateUI(); });

  // connect(m_Controls.targetCenroidSelector,
  //         &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged,
  //         this,
  //         [this]() { OnUpdateUI(); });

  connect(m_Controls.ckbMonoisotopic,
          &QCheckBox::toggled,
          this,
          [this](bool b) { m_Controls.groupMonoisotopic->setVisible(b); });

  emit m_Controls.ckbMonoisotopic->toggled(false);

  connect(
    m_Controls.btnPickPeaksOverview, &QAbstractButton::clicked, this, &m2PeakPickingView::OnStartPeakPickingOverview);

  connect(m_Controls.btnCombineList, &QAbstractButton::clicked, this, &m2PeakPickingView::OnCombineLists);
  connect(m_Controls.btnBinPeaks, &QAbstractButton::clicked, this, &m2PeakPickingView::OnStartPeakBinning);

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
  OnUpdateUI();
}

void m2PeakPickingView::OnCurrentTabChanged(unsigned int idx)
{
  const auto tabText = m_Controls.tabWidget->tabText(idx);

  m_Controls.tabWidget->widget(0)->layout()->removeWidget(m_Controls.peakPickingControls);
  m_Controls.tabWidget->widget(1)->layout()->removeWidget(m_Controls.peakPickingControls);
  m_Controls.tabWidget->widget(1)->layout()->removeWidget(m_Controls.binningControls);
  m_Controls.tabWidget->widget(3)->layout()->removeWidget(m_Controls.binningControls);

  disconnect(m_Controls.sbTolerance, SIGNAL(valueChanged(double)), this, 0);
  disconnect(m_Controls.sbDistance, SIGNAL(valueChanged(double)), this, 0);
  disconnect(m_Controls.sliderSNR, SIGNAL(valueChanged(double)), this, 0);
  disconnect(m_Controls.sliderHWS, SIGNAL(valueChanged(double)), this, 0);
  disconnect(m_Controls.sliderCOR, SIGNAL(valueChanged(double)), this, 0);

  if (tabText == "Overview")
  { // 0
    dynamic_cast<QVBoxLayout *>(m_Controls.tabWidget->currentWidget()->layout())
      ->insertWidget(1, m_Controls.peakPickingControls);
    connect(m_Controls.sbTolerance, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPickingOverview()));
    connect(m_Controls.sbDistance, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPickingOverview()));
    connect(m_Controls.sliderSNR, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPickingOverview()));
    connect(m_Controls.sliderHWS, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPickingOverview()));
    connect(m_Controls.sliderCOR, SIGNAL(valueChanged(double)), this, SLOT(OnStartPeakPickingOverview()));
  }
  else if (tabText == "Pixel-wise")
  { // 1
    dynamic_cast<QVBoxLayout *>(m_Controls.tabWidget->currentWidget()->layout())
      ->insertWidget(1, m_Controls.peakPickingControls);
    dynamic_cast<QVBoxLayout *>(m_Controls.tabWidget->currentWidget()->layout())
      ->insertWidget(2, m_Controls.binningControls);
  }
  else if (tabText == "Combine Centroids")
  { // 2
    // dynamic_cast<QVBoxLayout *>(m_Controls.tabWidget->currentWidget()->layout())
    //   ->insertWidget(1, m_Controls.binningControls);
  }
  else if (tabText == "Binning")
  { // 2
    dynamic_cast<QVBoxLayout *>(m_Controls.tabWidget->currentWidget()->layout())
      ->insertWidget(1, m_Controls.binningControls);
  }
}

void m2PeakPickingView::OnUpdateUI()
{
  m_Controls.btnPickPeaksPixelWise->setEnabled(m_Controls.sourceProfileImageSelector->GetSelectedNode());
  // m_Controls.peakPickingControls->setEnabled(false);

  // const auto profileSelection = m_Controls.sourceProfileSelector->GetSelectedNodesStdVector();
  // if (profileSelection.empty())
  // {
  //   auto subset = GetDataStorage()->GetSubset(m_NodePredicateProcessableProfileImage);
  //   if(subset && !subset->empty()){
  //     m_Controls.peakPickingControls->setEnabled(true);
  //   }
  // }
  // else
  // {
  //   m_Controls.peakPickingControls->setEnabled(true);
  // }

  // const auto centroidSelection = m_Controls.sourceCenroidSelector->GetSelectedNodesStdVector();
  // if (!centroidSelection.empty())
  // {
  //   m_Controls.centroidControls->setEnabled(true);
  // }
}

void m2PeakPickingView::OnUpdateUIOverviewTab() {}

void m2PeakPickingView::OnUpdateUIPixelWiseTab() {}

void m2PeakPickingView::OnUpdateUICentroidTab() {}

mitk::DataNode::Pointer m2PeakPickingView::CreatePeakList(const mitk::DataNode *parent, std::string name)
{
  auto node = mitk::DataNode::New();

  if (parent)
    m2::CopyNodeProperties(parent, node);
  else
    m2::DefaultNodeProperties(node);

  node->SetIntProperty("spectrum.marker.size", 4);
  auto intervals = m2::IntervalVector::New();
  intervals->SetType(m2::SpectrumFormat::Centroid);
  intervals->SetInfo("centroids");
  node->SetName(name);
  node->SetData(intervals);
  return node;
}

std::vector<m2::Interval> m2PeakPickingView::PeakPicking(const std::vector<double> &xs, const std::vector<double> &ys)
{
  using namespace m2::Signal;
  const auto snr = m_Controls.sliderSNR->value();
  const auto hws = m_Controls.sliderHWS->value();
  const auto minCor = m_Controls.sliderCOR->value();
  const auto tol = m_Controls.sbTolerance->value();
  const auto dist = m_Controls.sbDistance->value();
  const std::vector<unsigned int> sizes = {3, 4, 5, 6, 7, 8, 9, 10};

  std::vector<m2::Interval> peaks;
  peaks.clear();
  const auto noise = m2::Signal::mad(ys);
  localMaxima(begin(ys), end(ys), begin(xs), back_inserter(peaks), hws, noise * snr);
  if (m_Controls.ckbMonoisotopic->isChecked())
    try
    {
      peaks = monoisotopic(peaks, sizes, minCor, tol, dist);
    }
    catch (std::exception &e)
    {
      MITK_WARN << "Monoisotopic peak identification failed. " << e.what();
    }
  return peaks;
}

void m2PeakPickingView::OnStartPeakBinning()
{
  if (auto sourceNode = m_Controls.sourceCentroidsSelector->GetSelectedNode())
  {
    auto sourceData = dynamic_cast<m2::IntervalVector *>(sourceNode->GetData());
    std::vector<double> xsAll = sourceData->GetXMean();
    std::vector<double> ysAll = sourceData->GetYMax();
    std::vector<unsigned int> ssAll(xsAll.size(), 0);
    std::iota(std::begin(ssAll), std::end(ssAll), 0);

    using dIt = decltype(cbegin(xsAll));
    using iIt = decltype(cbegin(ssAll));
    auto Grouper = m2::Signal::grouperStrict<dIt, dIt, dIt, iIt>;

    // auto Grouper = m2::Signal::grouperRelaxed<dIt, dIt, dIt, iIt>;
    auto R = m2::Signal::groupBinning(xsAll, ysAll, ssAll, Grouper, m_Controls.sbBinningTolerance->value());
    // no filtering: How to filter peaks if multiple images sources?

    auto I = std::get<0>(R);
    std::string targetNodeName = "Binned Centroids (" + sourceNode->GetName() + ")";
    auto targetNode = GetDerivations(sourceNode, targetNodeName);
    bool targetNodeAlreadyInDataStorage = targetNode;
    if (!targetNode)
      targetNode = CreatePeakList(sourceNode, targetNodeName);

    auto targetData = dynamic_cast<m2::IntervalVector *>(targetNode->GetData());
    targetData->GetIntervals() = I;

    if (auto prop = sourceData->GetProperty("spectrum.pixel.count"))
    {
      auto numberOfValidPixels = dynamic_cast<mitk::IntProperty *>(prop.GetPointer())->GetValue();
      targetData->SetProperty("spectrum.pixel.count", mitk::IntProperty::New(numberOfValidPixels));
    }
    else
    {
      MITK_WARN << "Number Source pixel is not given!";
    }

    targetData->SetProperty("spectrum.xaxis.count", mitk::IntProperty::New(targetData->GetIntervals().size()));
    targetNode->Modified();

    if (!targetNodeAlreadyInDataStorage)
      GetDataStorage()->Add(targetNode, sourceNode);
  }
}

mitk::DataNode::Pointer m2PeakPickingView::GetParent(const mitk::DataNode *node)
{
  auto parents = GetDataStorage()->GetSources(node);
  if (parents && !parents->empty())
    return const_cast<mitk::DataNode *>(parents->front().GetPointer());
  else
    return nullptr;
}

mitk::DataNode::Pointer m2PeakPickingView::GetNode(std::string substring)
{
  auto nodes = GetDataStorage()->GetAll();
  if (!nodes->empty())
  {
    for (const mitk::DataNode *node : *nodes)
      if (node->GetName().find(substring) != std::string::npos)
        return const_cast<mitk::DataNode *>(node);
  }
  return nullptr;
}

mitk::DataNode::Pointer m2PeakPickingView::GetDerivations(const mitk::DataNode *parentNode, std::string substring)
{
  auto derivations = GetDataStorage()->GetDerivations(parentNode);
  if (!derivations->empty())
  {
    for (const mitk::DataNode *node : *derivations)
      if (node->GetName().find(substring) != std::string::npos)
        return const_cast<mitk::DataNode *>(node);
  }
  return nullptr;
}

void m2PeakPickingView::OnStartPeakPickingOverview()
{
  auto sourceProfileNodes = m_Controls.sourceProfileSelector->GetSelectedNodesStdVector();
  using namespace std;
  using namespace m2::Signal;
  for (const mitk::DataNode *sourceNode : sourceProfileNodes)
  {
    auto parentNode = GetParent(sourceNode);
    const auto sourceName = sourceNode->GetName();
    const auto parentName = parentNode->GetName();

    // auto p1 = sourceName.find(parentNode->GetName());
    std::string targetNodeName = "Overview Centroids";
    auto targetNode = GetDerivations(parentNode, targetNodeName);
    // find target if in selection
    bool targetNodeAlreadyInDataStorage = targetNode;
    if (!targetNode)
      targetNode = CreatePeakList(parentNode, targetNodeName);

    auto targetData = dynamic_cast<m2::IntervalVector *>(targetNode->GetData());
    auto sourceData = dynamic_cast<m2::IntervalVector *>(sourceNode->GetData());

    vector<double> ys, xs;
    ys = sourceData->GetYMean();
    xs = sourceData->GetXMean();

    targetData->GetIntervals() = PeakPicking(xs, ys);
    auto image = dynamic_cast<m2::SpectrumImageBase *>(parentNode->GetData());

    targetData->SetProperty("spectrum.pixel.count", mitk::IntProperty::New(image->GetNumberOfValidPixels()));
    targetData->SetProperty("spectrum.xaxis.count", mitk::IntProperty::New(targetData->GetIntervals().size()));
    targetNode->Modified();

    if (!targetNodeAlreadyInDataStorage)
      GetDataStorage()->Add(targetNode, parentNode);
  }

  OnUpdateUILabel();
  // emit m2::UIUtils::Instance()->BroadcastProcessingNodes("", node);
}

void m2PeakPickingView::OnStartPeakPickingImage()
{
  auto imageNode = m_Controls.sourceProfileImageSelector->GetSelectedNode();

  if (auto image = dynamic_cast<m2::ImzMLSpectrumImage *>(imageNode->GetData()))
  {
    using namespace std;
    auto N = image->GetXAxis().size() * image->GetNumberOfValidPixels();
    vector<double> xs, ys, xsAll, ysAll;
    vector<int> ssAll;
    xsAll.reserve(N);
    ysAll.reserve(N);
    ssAll.reserve(N);
    // idxAll.reserve(N);

    auto xsAllIt = back_inserter(xsAll);
    auto ysAllIt = back_inserter(ysAll);
    auto ssAllIt = back_inserter(ssAll);
    // auto idxAllIt = back_inserter(idxAll);

    mitk::ProgressBar::GetInstance()->AddStepsToDo(image->GetNumberOfValidPixels());
    for (unsigned int i = 0; i < image->GetNumberOfValidPixels(); ++i)
    {
      image->GetSpectrum(i, xs, ys);
      if (image->GetSpectrumType().Format == m2::SpectrumFormat::ContinuousProfile)
      {
        // Pick peaks
        auto peaks = PeakPicking(xs, ys);
        for_each(begin(peaks),
                 end(peaks),
                 [&](m2::Interval &i)
                 {
                   xsAllIt = i.x.mean();
                   ysAllIt = i.y.max();
                 });
        fill_n(ssAllIt, peaks.size(), i);
      }
      else if (image->GetSpectrumType().Format == m2::SpectrumFormat::ProcessedCentroid)
      {
        copy(begin(xs), end(xs), xsAllIt);
        copy(begin(ys), end(ys), ysAllIt);
        fill_n(ssAllIt, xs.size(), i);
      }
      else
      {
        MITK_INFO << "Invalid SpectrumFormat for pixel wise peak picking.";
        return;
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

    std::string targetNodeName = "Image Centeoids (" + imageNode->GetName() + ")";
    auto targetNode = GetDerivations(imageNode, targetNodeName);
    bool targetNodeAlreadyInDataStorage = targetNode;
    if (!targetNode)
      targetNode = CreatePeakList(imageNode, targetNodeName);

    auto targetData = dynamic_cast<m2::IntervalVector *>(targetNode->GetData());
    targetData->GetIntervals() = newI;

    targetData->SetProperty("spectrum.pixel.count", mitk::IntProperty::New(image->GetNumberOfValidPixels()));
    targetData->SetProperty("spectrum.xaxis.count", mitk::IntProperty::New(targetData->GetIntervals().size()));
    targetNode->Modified();

    if (!targetNodeAlreadyInDataStorage)
      GetDataStorage()->Add(targetNode, imageNode);
  }
}

void m2PeakPickingView::OnCombineLists()
{
  auto sourceNodes = m_Controls.sourceMultipleCenroidsSelector->GetSelectedNodesStdVector();
  if (sourceNodes.empty())
    return;

  int numberOfValidPixels = 0;
  std::string targetNodeName = "Combined Centeoids";
  auto targetNode = GetNode(targetNodeName);
  bool targetNodeAlreadyInDataStorage = targetNode;
  if (!targetNode)
    targetNode = CreatePeakList(nullptr, targetNodeName);

  int i = 0;
  auto targetData = dynamic_cast<m2::IntervalVector *>(targetNode->GetData());
  for (auto sourceNode : sourceNodes)
  {
    auto sourceData = dynamic_cast<const m2::IntervalVector *>(sourceNode->GetData());
    std::transform(sourceData->GetIntervals().begin(),
                   sourceData->GetIntervals().end(),
                   std::back_inserter(targetData->GetIntervals()),
                   [i](m2::Interval v)
                   {
                     v.sourceId = i;
                     return v;
                   });
    ++i;

    if (auto prop = sourceData->GetProperty("spectrum.pixel.count"))
    {
      numberOfValidPixels += dynamic_cast<mitk::IntProperty *>(prop.GetPointer())->GetValue();
    }
    else
    {
      MITK_WARN << "Number Source pixel is not given!";
    }
  }

  std::sort(targetData->GetIntervals().begin(), targetData->GetIntervals().end());

  if (!targetNodeAlreadyInDataStorage)
    GetDataStorage()->Add(targetNode);
  targetData->SetProperty("spectrum.pixel.count", mitk::IntProperty::New(numberOfValidPixels));
  targetData->SetProperty("spectrum.xaxis.count", mitk::IntProperty::New(targetData->GetIntervals().size()));
  targetNode->Modified();
}

void m2PeakPickingView::OnUpdateUILabel()
{
  // auto vectorNode = m_Controls.targetCenroidSelector->GetSelectedNode();
  // if (!vectorNode)
  // {
  //   m_Controls.labelPeakList->setText(QString("Peaks (%1/%2)").arg(0).arg(0));
  //   return;
  // }

  // auto vector = dynamic_cast<m2::IntervalVector *>(vectorNode->GetData());
  // if (!vector)
  //   return;

  // m_Controls.labelPeakList->setText(QString("Peaks
  // (%1/%2)").arg(selected).arg((int)vector->GetIntervals().size()));
}
