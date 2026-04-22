/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "QmitkConnectedComponentsView.h"
#include "QmitkGroupWidget.h"

// Qt
#include <QMessageBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QColor>
#include <QFileDialog>
#include <QString>

// MITK
#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkLabelSetImage.h>
#include <mitkLabel.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateOr.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateFunction.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkProgressBar.h>
#include <mitkRenderingManager.h>
#include <mitkStatusBar.h>

// MITK Qt Widgets
#include <QmitkMultiNodeSelectionWidget.h>

// ITK morphology
#include <itkBinaryBallStructuringElement.h>
#include <itkBinaryErodeImageFilter.h>
#include <itkBinaryDilateImageFilter.h>
#include <itkBinaryMorphologicalClosingImageFilter.h>
#include <itkBinaryFillholeImageFilter.h>

// ITK connected components
#include <itkConnectedComponentImageFilter.h>
#include <itkCastImageFilter.h>

// Standard
#include <algorithm>
#include <numeric>
#include <cmath>
#include <map>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>

// M2aia
#include <m2SpectrumImage.h>
#include <m2SpectrumImageStack.h>
#include <m2CoreCommon.h>

const std::string QmitkConnectedComponentsView::VIEW_ID = "org.mitk.views.m2.connectedcomponents";

// ---------------------------------------------------------------------------
// Type aliases used throughout
// All working pixel types are unsigned short 3D.
// ---------------------------------------------------------------------------
using PixelType          = unsigned short;
using LabelImageType     = itk::Image<PixelType,      3>;
using CCLabelImageType   = itk::Image<unsigned long,  3>;
using StructElemType     = itk::BinaryBallStructuringElement<PixelType, 3>;

// ---------------------------------------------------------------------------
// CreateQtPartControl
// ---------------------------------------------------------------------------

void QmitkConnectedComponentsView::CreateQtPartControl(QWidget *parent)
{
  m_Parent = parent;
  m_Controls.setupUi(parent);

  // ---- Image selection predicates ----
  auto isHelperObject = mitk::NodePredicateFunction::New(
    [](const mitk::DataNode *n) { return n->IsOn("helper object", nullptr, false); });
  auto noHelper = mitk::NodePredicateNot::New(isHelperObject);

  // Accept any image (binary or multilabel)
  auto isImage = mitk::NodePredicateAnd::New(
    mitk::NodePredicateOr::New(
        mitk::TNodePredicateDataType<mitk::Image>::New(),
        mitk::TNodePredicateDataType<mitk::MultiLabelSegmentation>::New()), 
    noHelper);

  m_Controls.segmentationSelection->SetDataStorage(GetDataStorage());
  m_Controls.segmentationSelection->SetNodePredicate(isImage);
  m_Controls.segmentationSelection->SetSelectionIsOptional(true);
  m_Controls.segmentationSelection->SetEmptyInfo("Select segmentation / binary image");
  m_Controls.segmentationSelection->SetPopUpTitel("Segmentation Image");

  // ---- Default morphology radius ----
  m_Controls.spinRadius->setValue(1);

  // ---- Morphology buttons ----
  connect(m_Controls.btnErode,     &QPushButton::clicked, this, &QmitkConnectedComponentsView::OnErode);
  connect(m_Controls.btnDilate,    &QPushButton::clicked, this, &QmitkConnectedComponentsView::OnDilate);
  connect(m_Controls.btnClose,     &QPushButton::clicked, this, &QmitkConnectedComponentsView::OnClose);
  connect(m_Controls.btnFillHoles, &QPushButton::clicked, this, &QmitkConnectedComponentsView::OnFillHoles);

  // ---- CC button ----
  connect(m_Controls.btnCreateCC, &QPushButton::clicked,
          this, &QmitkConnectedComponentsView::OnCreateConnectedComponents);

  // ---- Group buttons ----
  connect(m_Controls.btnSuggestGroups, &QPushButton::clicked,
          this, &QmitkConnectedComponentsView::OnSuggestGroups);
  connect(m_Controls.btnAddGroup, &QPushButton::clicked,
          this, &QmitkConnectedComponentsView::OnAddGroup);
  connect(m_Controls.btnApplyGroups, &QPushButton::clicked,
          this, &QmitkConnectedComponentsView::OnApplyGroups);

  // ---- CSV Export ----
  {
    // Accept any image-like node (plain Image, MultiLabelSegmentation, SpectrumImage)
    auto isHelperObj = mitk::NodePredicateFunction::New(
      [](const mitk::DataNode *n) { return n->IsOn("helper object", nullptr, false); });
    auto noHelperExport = mitk::NodePredicateNot::New(isHelperObj);
    auto isAnyImage = mitk::NodePredicateAnd::New(
      mitk::NodePredicateOr::New(
        mitk::TNodePredicateDataType<mitk::Image>::New(),
        mitk::TNodePredicateDataType<mitk::MultiLabelSegmentation>::New()),
      noHelperExport);

    m_Controls.featureImageSelection->SetDataStorage(GetDataStorage());
    m_Controls.featureImageSelection->SetNodePredicate(isAnyImage);
    m_Controls.featureImageSelection->SetSelectionIsOptional(true);
    m_Controls.featureImageSelection->SetEmptyInfo("Select feature images (optional)");
    m_Controls.featureImageSelection->SetPopUpTitel("Feature Images");

    connect(m_Controls.btnExportCSV, &QPushButton::clicked,
            this, &QmitkConnectedComponentsView::OnExportCSV);
  }

  // Ensure scroll area layout exists
  if (!m_Controls.scrollAreaWidgetContents->layout())
  {
    auto *lay = new QVBoxLayout(m_Controls.scrollAreaWidgetContents);
    lay->setAlignment(Qt::AlignTop);
    lay->setSpacing(4);
    m_Controls.scrollAreaWidgetContents->setLayout(lay);
  }
  else
  {
    auto *lay = m_Controls.scrollAreaWidgetContents->layout();
    lay->setAlignment(Qt::AlignTop);
  }
}

void QmitkConnectedComponentsView::SetFocus()
{
  m_Controls.segmentationSelection->setFocus();
}

// ---------------------------------------------------------------------------
// CommitMorphologyResult
// ---------------------------------------------------------------------------

void QmitkConnectedComponentsView::CommitMorphologyResult(
  mitk::DataNode *node, mitk::Image::Pointer result)
{
  if (!node || !result)
    return;

  if (auto *seg = dynamic_cast<mitk::MultiLabelSegmentation *>(node->GetData()))
  {
    // Threshold result in-place (>0 → 1) directly via write accessor on the result image,
    // then hand it straight to AddGroup(Image*) — no temporary image, no UpdateGroupImage.
    const auto dims  = result->GetDimensions();
    const std::size_t total = std::accumulate(dims, dims + result->GetDimension(),
                                              std::size_t{1}, std::multiplies<std::size_t>{});
    {
      mitk::ImagePixelWriteAccessor<PixelType, 3> writer(result);
      PixelType *data = writer.GetData();
      std::transform(data, data + total, data,
                     [](PixelType v) -> PixelType { return v > 0 ? 1 : 0; });
    }
    // AddGroup(Image*) copies the image buffer using SetVolume internally —
    // geometry must match the segmentation (already cloned from group 0).
    seg->AddGroup(result.GetPointer());
  }
  else
  {
    node->SetData(result);
  }
}

// ---------------------------------------------------------------------------
// Morphology helpers (template + lambda approach)
// ---------------------------------------------------------------------------

mitk::Image* QmitkConnectedComponentsView::ResolveInputImage(mitk::DataNode* node) const
{
  if (!node)
    return nullptr;
  // Prefer MultiLabelSegmentation: use group 0 as the working image
  if (auto *seg = dynamic_cast<mitk::MultiLabelSegmentation *>(node->GetData()))
    return seg->GetGroupImage(0);
  return dynamic_cast<mitk::Image *>(node->GetData());
}

// ---------------------------------------------------------------------------
// Morphology helpers (ITK fixed-type)

mitk::Image::Pointer QmitkConnectedComponentsView::ApplyMorphologyToImage(
  mitk::Image::Pointer inputImage, const std::string &operation, int radius)
{
  if (!inputImage)
    return nullptr;

  // Work entirely in unsigned short (PixelType) / 3D.
  using StructType = itk::BinaryBallStructuringElement<PixelType, 3>;

  const unsigned int r = static_cast<unsigned int>(std::max(1, radius));
  StructType structElem;
  structElem.SetRadius(r);
  structElem.CreateStructuringElement();

  LabelImageType::Pointer itkImg;
  try
  {
    mitk::CastToItkImage(inputImage, itkImg);
  }
  catch (const std::exception &e)
  {
    MITK_WARN << "ApplyMorphologyToImage: cast failed: " << e.what();
    return nullptr;
  }

  // Determine the foreground value present in the image (first non-zero pixel).
  // For binary segmentations this is typically 1 (MultiLabelSegmentation) or 255.
  PixelType fgValue = 1;
  {
    const auto dims = inputImage->GetDimensions();
    const std::size_t total = std::accumulate(dims, dims + inputImage->GetDimension(),
                                              std::size_t{1}, std::multiplies<std::size_t>{});
    mitk::ImagePixelReadAccessor<PixelType, 3> reader(inputImage);
    const PixelType *data = reader.GetData();
    const PixelType *found = std::find_if(data, data + total,
                                          [](PixelType v){ return v > 0; });
    if (found != data + total)
      fgValue = *found;
  }

  LabelImageType::Pointer itkResult;
  try
  {
    if (operation == "erode")
    {
      using FilterType = itk::BinaryErodeImageFilter<LabelImageType, LabelImageType, StructType>;
      auto filter = FilterType::New();
      filter->SetInput(itkImg);
      filter->SetKernel(structElem);
      filter->SetForegroundValue(fgValue);
      filter->Update();
      itkResult = filter->GetOutput();
    }
    else if (operation == "dilate")
    {
      using FilterType = itk::BinaryDilateImageFilter<LabelImageType, LabelImageType, StructType>;
      auto filter = FilterType::New();
      filter->SetInput(itkImg);
      filter->SetKernel(structElem);
      filter->SetForegroundValue(fgValue);
      filter->Update();
      itkResult = filter->GetOutput();
    }
    else if (operation == "close")
    {
      using FilterType = itk::BinaryMorphologicalClosingImageFilter<LabelImageType, LabelImageType, StructType>;
      auto filter = FilterType::New();
      filter->SetInput(itkImg);
      filter->SetKernel(structElem);
      filter->SetForegroundValue(fgValue);
      filter->Update();
      itkResult = filter->GetOutput();
    }
    else if (operation == "fillholes")
    {
      using FillType = itk::BinaryFillholeImageFilter<LabelImageType>;
      auto filter = FillType::New();
      filter->SetInput(itkImg);
      filter->SetForegroundValue(fgValue);
      filter->Update();
      itkResult = filter->GetOutput();
    }
    else
    {
      MITK_WARN << "ApplyMorphologyToImage: unknown operation '" << operation << "'";
      return nullptr;
    }
  }
  catch (const std::exception &e)
  {
    MITK_WARN << "Morphology operation '" << operation << "' failed: " << e.what();
    return nullptr;
  }

  // Convert ITK result back to MITK image and copy pixel data via accessors.
  mitk::Image::Pointer outputImage = mitk::Image::New();
  mitk::CastToMitkImage(itkResult, outputImage);
  outputImage->SetGeometry(inputImage->GetGeometry()->Clone());
  return outputImage;
}

// ---------------------------------------------------------------------------
// Morphology slots
// ---------------------------------------------------------------------------

void QmitkConnectedComponentsView::OnErode()
{
  auto node = m_Controls.segmentationSelection->GetSelectedNode();
  if (!node) { QMessageBox::warning(m_Parent, "No selection", "Please select an image first."); return; }

  auto img = ResolveInputImage(node);
  if (!img) return;

  auto result = ApplyMorphologyToImage(img, "erode", m_Controls.spinRadius->value());
  if (!result) return;

  CommitMorphologyResult(node, result);
  m_WorkingImage = result;
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  mitk::StatusBar::GetInstance()->DisplayText("Erosion applied.");
}

void QmitkConnectedComponentsView::OnDilate()
{
  auto node = m_Controls.segmentationSelection->GetSelectedNode();
  if (!node) { QMessageBox::warning(m_Parent, "No selection", "Please select an image first."); return; }

  auto img = ResolveInputImage(node);
  if (!img) return;

  auto result = ApplyMorphologyToImage(img, "dilate", m_Controls.spinRadius->value());
  if (!result) return;

  CommitMorphologyResult(node, result);
  m_WorkingImage = result;
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  mitk::StatusBar::GetInstance()->DisplayText("Dilation applied.");
}

void QmitkConnectedComponentsView::OnClose()
{
  auto node = m_Controls.segmentationSelection->GetSelectedNode();
  if (!node) { QMessageBox::warning(m_Parent, "No selection", "Please select an image first."); return; }

  auto img = ResolveInputImage(node);
  if (!img) return;

  auto result = ApplyMorphologyToImage(img, "close", m_Controls.spinRadius->value());
  if (!result) return;

  CommitMorphologyResult(node, result);
  m_WorkingImage = result;
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  mitk::StatusBar::GetInstance()->DisplayText("Morphological closing applied.");
}

void QmitkConnectedComponentsView::OnFillHoles()
{
  auto node = m_Controls.segmentationSelection->GetSelectedNode();
  if (!node) { QMessageBox::warning(m_Parent, "No selection", "Please select an image first."); return; }

  auto img = ResolveInputImage(node);
  if (!img) return;

  auto result = ApplyMorphologyToImage(img, "fillholes", m_Controls.spinRadius->value());
  if (!result) return;

  CommitMorphologyResult(node, result);
  m_WorkingImage = result;
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  mitk::StatusBar::GetInstance()->DisplayText("Fill holes applied.");
}

// ---------------------------------------------------------------------------
// Create Connected Components
// ---------------------------------------------------------------------------

void QmitkConnectedComponentsView::OnCreateConnectedComponents()
{
  auto node = m_Controls.segmentationSelection->GetSelectedNode();
  if (!node)
  {
    QMessageBox::warning(m_Parent, "No selection", "Please select an image first.");
    return;
  }

  auto mitkImg = ResolveInputImage(node);
  if (!mitkImg)
  {
    QMessageBox::warning(m_Parent, "Invalid data", "Selected node does not contain a supported image.");
    return;
  }

  m_ComponentSizes.clear();
  m_CCNode = nullptr;

  mitk::ProgressBar::GetInstance()->AddStepsToDo(3);

  try
  {
    // ---- Cast input to unsigned short (PixelType) 3D ----
    LabelImageType::Pointer itkImg;
    mitk::CastToItkImage(mitkImg, itkImg);

    mitk::ProgressBar::GetInstance()->Progress();

    // ---- Connected component labeling ----
    using CCFilter = itk::ConnectedComponentImageFilter<LabelImageType, CCLabelImageType>;
    auto ccFilter = CCFilter::New();
    ccFilter->SetInput(itkImg);
    ccFilter->SetFullyConnected(m_Controls.checkFullyConnected->isChecked());
    ccFilter->Update();

    mitk::ProgressBar::GetInstance()->Progress();

    // ---- Count voxels per label via raw ITK buffer pointer ----
    // CCLabelImageType uses unsigned long; we read directly from the ITK buffer
    // (no need to convert to a MITK image for counting).
    CCLabelImageType::Pointer ccOut = ccFilter->GetOutput();
    const std::size_t total = ccOut->GetLargestPossibleRegion().GetNumberOfPixels();
    const unsigned long *rawCC = ccOut->GetBufferPointer();

    std::map<unsigned long, unsigned long> labelCount;
    std::for_each(rawCC, rawCC + total, [&labelCount](unsigned long lbl)
    {
      if (lbl > 0) ++labelCount[lbl];
    });

    // Build sorted list: (size, oldLabel) descending by size
    std::vector<std::pair<unsigned long, unsigned long>> sizeLabel;
    sizeLabel.reserve(labelCount.size());
    for (auto &kv : labelCount)
      sizeLabel.emplace_back(kv.second, kv.first);
    std::sort(sizeLabel.begin(), sizeLabel.end(),
              [](const auto &a, const auto &b){ return a.first > b.first; });

    // Build remap: oldLabel -> newLabel (1-based rank by size)
    std::unordered_map<unsigned long, PixelType> remap;
    remap.reserve(sizeLabel.size());
    m_ComponentSizes.clear();
    m_ComponentSizes.reserve(sizeLabel.size());
    for (PixelType rank = 1;
         rank <= static_cast<PixelType>(sizeLabel.size()); ++rank)
    {
      remap[sizeLabel[rank - 1].second] = rank;
      m_ComponentSizes.push_back(sizeLabel[rank - 1].first);
    }

    // Build relabeled unsigned-short MITK image via ImagePixelWriteAccessor.
    // Initialize with same geometry as the input.
    auto ccMitkImage = mitk::Image::New();
    ccMitkImage->Initialize(mitk::MakeScalarPixelType<PixelType>(),
                            *mitkImg->GetTimeGeometry());
    ccMitkImage->SetGeometry(mitkImg->GetGeometry()->Clone());
    {
      mitk::ImagePixelWriteAccessor<PixelType, 3> writer(ccMitkImage);
      PixelType *dst = writer.GetData();
      std::transform(rawCC, rawCC + total, dst, [&remap](unsigned long lbl) -> PixelType
      {
        if (lbl == 0) return 0;
        auto it = remap.find(lbl);
        return it != remap.end() ? it->second : static_cast<PixelType>(0);
      });
    }

    mitk::ProgressBar::GetInstance()->Progress();

    // ---- Create / update output data node ----
    std::string outName = node->GetName() + "_CC";
    mitk::DataNode::Pointer outNode = GetDataStorage()->GetNamedNode(outName);
    if (!outNode)
    {
      outNode = mitk::DataNode::New();
      outNode->SetName(outName);
      GetDataStorage()->Add(outNode, node);
    }
    outNode->SetData(ccMitkImage);
    outNode->SetProperty("binary", mitk::BoolProperty::New(false));
    m_CCNode = outNode;

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();

    // ---- Update stats label ----
    unsigned long minSz = m_ComponentSizes.empty() ? 0 : m_ComponentSizes.back();
    unsigned long maxSz = m_ComponentSizes.empty() ? 0 : m_ComponentSizes.front();
    m_Controls.labelStats->setText(
      QString("Components: %1   |   Min size: %2 vx   |   Max size: %3 vx")
        .arg(static_cast<int>(m_ComponentSizes.size()))
        .arg(minSz)
        .arg(maxSz));

    // Update group spinbox limits
    for (auto *gw : m_GroupWidgets)
      gw->SetAbsoluteMax(static_cast<int>(maxSz));

    UpdateGroupCountLabels();

    mitk::StatusBar::GetInstance()->DisplayText(
      QString("Connected components: %1").arg(m_ComponentSizes.size()).toLocal8Bit().constData());
  }
  catch (const std::exception &e)
  {
    QMessageBox::critical(m_Parent, "Error", QString("Connected components failed:\n%1").arg(e.what()));
    mitk::ProgressBar::GetInstance()->Progress(3);
  }
}

// ---------------------------------------------------------------------------
// Groups
// ---------------------------------------------------------------------------

void QmitkConnectedComponentsView::OnSuggestGroups()
{
  if (m_ComponentSizes.empty())
  {
    QMessageBox::information(m_Parent, "No data",
                             "Please create connected components first.");
    return;
  }

  ClearGroups();

  // m_ComponentSizes is sorted descending. Build ascending copy for quantile math.
  std::vector<unsigned long> sorted = m_ComponentSizes;
  std::sort(sorted.begin(), sorted.end());

  const int n = static_cast<int>(sorted.size());
  unsigned long q33 = sorted[static_cast<size_t>(std::floor(n * 0.33))];
  unsigned long q67 = sorted[static_cast<size_t>(std::floor(n * 0.67))];
  unsigned long qMax = sorted.back();

  int absMax = static_cast<int>(qMax);

  AddGroupWidget("Small",  0,                 static_cast<int>(q33), QColor(0x4e, 0xa3, 0xd4), absMax);
  AddGroupWidget("Medium", static_cast<int>(q33) + 1, static_cast<int>(q67), QColor(0x6a, 0xbf, 0x69), absMax);
  AddGroupWidget("Large",  static_cast<int>(q67) + 1, absMax,           QColor(0xe0, 0x7b, 0x39), absMax);

  UpdateGroupCountLabels();
}

void QmitkConnectedComponentsView::OnAddGroup()
{
  int absMax = m_ComponentSizes.empty() ? 999999 : static_cast<int>(m_ComponentSizes.front());
  QString name = QString("Group %1").arg(m_GroupWidgets.size() + 1);

  // Colors cycling
  static const QColor palette[] = {
    QColor(0xe0, 0x7b, 0x39),
    QColor(0x4e, 0xa3, 0xd4),
    QColor(0x6a, 0xbf, 0x69),
    QColor(0xa5, 0x6f, 0xbf),
    QColor(0xf0, 0xc0, 0x40),
    QColor(0xe0, 0x50, 0x50),
  };
  QColor color = palette[m_GroupWidgets.size() % 6];

  AddGroupWidget(name, 0, absMax, color, absMax);
  UpdateGroupCountLabels();
}

void QmitkConnectedComponentsView::AddGroupWidget(const QString &name, int minSize, int maxSize,
                                                  const QColor &color, int absoluteMax)
{
  auto *gw = new QmitkGroupWidget(name, minSize, maxSize, color,
                                  absoluteMax > 0 ? absoluteMax : 999999,
                                  m_Controls.scrollAreaWidgetContents);

  connect(gw, &QmitkGroupWidget::GroupChanged,    this, &QmitkConnectedComponentsView::OnGroupChanged);
  connect(gw, &QmitkGroupWidget::RemoveRequested, this, &QmitkConnectedComponentsView::OnRemoveGroup);

  m_GroupWidgets.push_back(gw);
  m_Controls.scrollAreaWidgetContents->layout()->addWidget(gw);
}

void QmitkConnectedComponentsView::OnGroupChanged()
{
  UpdateGroupCountLabels();
}

void QmitkConnectedComponentsView::OnRemoveGroup(QmitkGroupWidget *widget)
{
  auto it = std::find(m_GroupWidgets.begin(), m_GroupWidgets.end(), widget);
  if (it != m_GroupWidgets.end())
    m_GroupWidgets.erase(it);

  m_Controls.scrollAreaWidgetContents->layout()->removeWidget(widget);
  widget->deleteLater();
  UpdateGroupCountLabels();
}

void QmitkConnectedComponentsView::ClearGroups()
{
  for (auto *gw : m_GroupWidgets)
  {
    m_Controls.scrollAreaWidgetContents->layout()->removeWidget(gw);
    delete gw;
  }
  m_GroupWidgets.clear();
}

void QmitkConnectedComponentsView::UpdateGroupCountLabels()
{
  if (m_ComponentSizes.empty()) return;

  for (auto *gw : m_GroupWidgets)
  {
    int lo = gw->GetMinSize();
    int hi = gw->GetMaxSize();
    int count = 0;
    for (unsigned long s : m_ComponentSizes)
    {
      int si = static_cast<int>(s);
      if (si >= lo && si <= hi) ++count;
    }
    gw->SetMatchCount(count);
  }
}

// ---------------------------------------------------------------------------
// Apply groups → color-coded MultiLabelSegmentation
// ---------------------------------------------------------------------------

void QmitkConnectedComponentsView::OnApplyGroups()
{
  if (!m_CCNode || m_GroupWidgets.empty())
  {
    QMessageBox::information(m_Parent, "Nothing to apply",
                             "Please create connected components and define at least one group.");
    return;
  }

  auto ccImage = dynamic_cast<mitk::Image *>(m_CCNode->GetData());
  if (!ccImage)
  {
    QMessageBox::warning(m_Parent, "Error", "Connected component image is invalid.");
    return;
  }

  try
  {
    // Build a group-index image: each voxel gets the group index of its label
    // 0 = unassigned

    // First collect group assignments per component label (1-based)
    // m_ComponentSizes[i] => label i+1 (after relabeling)
    const int numComponents = static_cast<int>(m_ComponentSizes.size());
    std::vector<int> labelToGroup(numComponents + 1, 0); // 0 = unassigned

    for (int gi = 0; gi < static_cast<int>(m_GroupWidgets.size()); ++gi)
    {
      auto *gw = m_GroupWidgets[gi];
      int lo = gw->GetMinSize();
      int hi = gw->GetMaxSize();
      for (int li = 1; li <= numComponents; ++li)
      {
        int sz = static_cast<int>(m_ComponentSizes[static_cast<size_t>(li - 1)]);
        if (sz >= lo && sz <= hi)
          labelToGroup[li] = gi + 1; // 1-based group index
      }
    }

    // Build group image (unsigned short, same dims as CC image) via accessors.
    const auto dims       = ccImage->GetDimensions();
    const std::size_t total = std::accumulate(dims, dims + ccImage->GetDimension(),
                                              std::size_t{1}, std::multiplies<std::size_t>{});

    auto mitkGroupImg = mitk::Image::New();
    mitkGroupImg->Initialize(mitk::MakeScalarPixelType<PixelType>(), *ccImage->GetTimeGeometry());
    mitkGroupImg->SetGeometry(ccImage->GetGeometry()->Clone());
    {
      mitk::ImagePixelReadAccessor<PixelType, 3>  reader(ccImage);
      mitk::ImagePixelWriteAccessor<PixelType, 3> writer(mitkGroupImg);
      const PixelType *src = reader.GetData();
      PixelType       *dst = writer.GetData();
      std::transform(src, src + total, dst,
                     [&labelToGroup, numComponents](PixelType lbl) -> PixelType
      {
        if (lbl > 0 && static_cast<int>(lbl) <= numComponents)
          return static_cast<PixelType>(labelToGroup[lbl]);
        return 0;
      });
    }

    // Build proper MultiLabelSegmentation
    auto labelSetImg = mitk::MultiLabelSegmentation::New();
    labelSetImg->InitializeByLabeledImage(mitkGroupImg.GetPointer());

    // Assign group colors to labels
    for (int gi = 0; gi < static_cast<int>(m_GroupWidgets.size()); ++gi)
    {
      mitk::Label::PixelType labelValue = static_cast<mitk::Label::PixelType>(gi + 1);
      if (labelSetImg->ExistLabel(labelValue))
      {
        auto label = labelSetImg->GetLabel(labelValue);
        if (label.IsNotNull())
        {
          QColor qc = m_GroupWidgets[gi]->GetColor();
          mitk::Color mitkColor;
          mitkColor.Set(qc.redF(), qc.greenF(), qc.blueF());
          label->SetColor(mitkColor);
          label->SetName(m_GroupWidgets[gi]->GetGroupName().toStdString());
          labelSetImg->UpdateLookupTable(labelValue);
        }
      }
    }

    // Add/update output node
    std::string outName = m_CCNode->GetName() + "_Groups";
    mitk::DataNode::Pointer outNode = GetDataStorage()->GetNamedNode(outName);
    if (!outNode)
    {
      outNode = mitk::DataNode::New();
      outNode->SetName(outName);
      GetDataStorage()->Add(outNode, m_CCNode);
    }
    outNode->SetData(labelSetImg);

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
    mitk::StatusBar::GetInstance()->DisplayText(
      QString("Group segmentation applied: %1 groups.").arg(m_GroupWidgets.size()).toLocal8Bit().constData());
  }
  catch (const std::exception &e)
  {
    QMessageBox::critical(m_Parent, "Error",
                          QString("Apply groups failed:\n%1").arg(e.what()));
  }
}

// ---------------------------------------------------------------------------
// CSV Export
// ---------------------------------------------------------------------------

void QmitkConnectedComponentsView::OnExportCSV()
{
  ExportCSV(m_Controls.radioPixelWise->isChecked());
}

void QmitkConnectedComponentsView::ExportCSV(bool pixelWise)
{
  if (!m_CCNode)
  {
    QMessageBox::warning(m_Parent, "No CC result",
                         "Please create connected components first.");
    return;
  }

  auto *ccImage = dynamic_cast<mitk::Image *>(m_CCNode->GetData());
  if (!ccImage)
  {
    QMessageBox::warning(m_Parent, "Error", "Connected component image is invalid.");
    return;
  }

  // -------------------------------------------------------------------------
  // Ask for output file
  // -------------------------------------------------------------------------
  const QString defaultName = pixelWise ? "cc_export_pixelwise.csv"
                                        : "cc_export_objectwise.csv";
  const QString filePath = QFileDialog::getSaveFileName(
    m_Parent, "Export CSV", defaultName, "CSV files (*.csv);;All files (*)");
  if (filePath.isEmpty())
    return;

  // -------------------------------------------------------------------------
  // Rebuild cluster-label → group-name mapping
  // -------------------------------------------------------------------------
  const int numComponents = static_cast<int>(m_ComponentSizes.size());
  std::vector<std::string> labelToGroupName(static_cast<std::size_t>(numComponents + 1), "");
  for (int gi = 0; gi < static_cast<int>(m_GroupWidgets.size()); ++gi)
  {
    auto *gw = m_GroupWidgets[gi];
    const int lo = gw->GetMinSize(), hi = gw->GetMaxSize();
    const std::string name = gw->GetGroupName().toStdString();
    for (int li = 1; li <= numComponents; ++li)
    {
      const int sz = static_cast<int>(m_ComponentSizes[static_cast<std::size_t>(li - 1)]);
      if (sz >= lo && sz <= hi)
        labelToGroupName[static_cast<std::size_t>(li)] = name;
    }
  }

  // -------------------------------------------------------------------------
  // Build feature sources
  // -------------------------------------------------------------------------
  struct FeatureSource
  {
    enum class Type { PLAIN_IMAGE, MULTILABEL, SPECTRUM_IMAGE } type;
    std::string nodeName;
    std::vector<std::string> columnNames;
    mitk::BaseGeometry *geom = nullptr;

    // PLAIN_IMAGE / MULTILABEL: pre-converted float ITK image
    itk::Image<float, 3>::Pointer floatImg;
    // MULTILABEL: label value → label name
    std::map<unsigned int, std::string> labelNames;

    // SPECTRUM_IMAGE
    m2::SpectrumImage *specImage = nullptr;
    // Pre-created read accessor for the index image (avoids per-pixel locking)
    std::shared_ptr<mitk::ImagePixelReadAccessor<m2::IndexImagePixelType, 3>> indexAcc;
    std::array<unsigned int, 3> indexImgDims = {0, 0, 0};
  };

  std::vector<FeatureSource> sources;

  auto selectedNodes = m_Controls.featureImageSelection->GetSelectedNodes();
  for (const auto &nodePtr : selectedNodes)
  {
    mitk::DataNode *node = nodePtr.GetPointer();
    if (!node || !node->GetData()) continue;

    FeatureSource src;
    src.nodeName = node->GetName();
    src.geom     = node->GetData()->GetGeometry();

    // ---- m2::SpectrumImage (check before mitk::Image, it inherits from it) ----
    if (auto *si = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
    {
      // Skip SpectrumImageStack
      if (dynamic_cast<m2::SpectrumImageStack *>(node->GetData()))
        continue;

      src.type      = FeatureSource::Type::SPECTRUM_IMAGE;
      src.specImage = si;

      mitk::Image *idxImg = si->GetIndexImage();
      try
      {
        src.indexAcc = std::make_shared<mitk::ImagePixelReadAccessor<m2::IndexImagePixelType, 3>>(idxImg);
        src.indexImgDims = {idxImg->GetDimension(0),
                            idxImg->GetDimension(1),
                            idxImg->GetDimension(2)};
      }
      catch (const std::exception &e)
      {
        MITK_WARN << "CSV export: cannot access index image of '" << src.nodeName << "': " << e.what();
        continue;
      }

      const auto &xAxis = si->GetXAxis();
      src.columnNames.reserve(xAxis.size());
      for (double mz : xAxis)
      {
        std::ostringstream oss;
        oss << src.nodeName << "_mz_" << std::fixed << std::setprecision(4) << mz;
        src.columnNames.push_back(oss.str());
      }
    }
    // ---- mitk::MultiLabelSegmentation ----
    else if (auto *seg = dynamic_cast<mitk::MultiLabelSegmentation *>(node->GetData()))
    {
      src.type = FeatureSource::Type::MULTILABEL;
      try
      {
        mitk::CastToItkImage(seg->GetGroupImage(0), src.floatImg);
      }
      catch (const std::exception &e)
      {
        MITK_WARN << "CSV export: cannot cast MultiLabelSegmentation '" << src.nodeName << "': " << e.what();
        continue;
      }
      src.columnNames.push_back(src.nodeName + "_label");
      for (auto lv : seg->GetAllLabelValues())
      {
        auto lbl = seg->GetLabel(lv);
        if (lbl.IsNotNull())
          src.labelNames[static_cast<unsigned int>(lv)] = lbl->GetName();
      }
    }
    // ---- plain mitk::Image ----
    else if (auto *img = dynamic_cast<mitk::Image *>(node->GetData()))
    {
      src.type = FeatureSource::Type::PLAIN_IMAGE;
      try
      {
        mitk::CastToItkImage(img, src.floatImg);
      }
      catch (const std::exception &e)
      {
        MITK_WARN << "CSV export: cannot cast image '" << src.nodeName << "': " << e.what();
        continue;
      }
      src.columnNames.push_back(src.nodeName + "_value");
    }
    else
      continue;

    sources.push_back(std::move(src));
  }

  // Count total feature columns
  std::size_t totalCols = 0;
  for (const auto &src : sources)
    totalCols += src.columnNames.size();

  // -------------------------------------------------------------------------
  // Helper: sample one FeatureSource at a world point → append to out
  // -------------------------------------------------------------------------
  auto sampleSource = [&](FeatureSource &src,
                          const mitk::Point3D &worldPt,
                          std::vector<double> &out)
  {
    if (!src.geom || !src.geom->IsInside(worldPt))
    {
      out.insert(out.end(), src.columnNames.size(),
                 std::numeric_limits<double>::quiet_NaN());
      return;
    }

    mitk::Point3D idx3d;
    src.geom->WorldToIndex(worldPt, idx3d);
    itk::Index<3> idx = {{static_cast<itk::IndexValueType>(std::lround(idx3d[0])),
                          static_cast<itk::IndexValueType>(std::lround(idx3d[1])),
                          static_cast<itk::IndexValueType>(std::lround(idx3d[2]))}};

    if (src.type == FeatureSource::Type::SPECTRUM_IMAGE)
    {
      // Bounds check against index image dimensions
      if (idx[0] < 0 || idx[1] < 0 || idx[2] < 0 ||
          static_cast<unsigned int>(idx[0]) >= src.indexImgDims[0] ||
          static_cast<unsigned int>(idx[1]) >= src.indexImgDims[1] ||
          static_cast<unsigned int>(idx[2]) >= src.indexImgDims[2])
      {
        out.insert(out.end(), src.columnNames.size(),
                   std::numeric_limits<double>::quiet_NaN());
        return;
      }
      const auto spectrumId = src.indexAcc->GetPixelByIndex(idx);
      if (spectrumId == 0)
      {
        out.insert(out.end(), src.columnNames.size(), 0.0);
        return;
      }

      // Initialize all output columns to zero; for processed centroid imzML
      // each spectrum carries its own sparse, potentially shifted m/z axis,
      // so we must match every peak to the nearest global-axis column.
      std::vector<double> colVals(src.columnNames.size(), 0.0);

      std::vector<double> specXs, specYs;
      src.specImage->GetSpectrum(spectrumId, specXs, specYs);

      const std::vector<double> &globalXAxis = src.specImage->GetXAxis();

      for (std::size_t pi = 0; pi < specXs.size(); ++pi)
      {
        const double mz  = specXs[pi];
        const double tol = src.specImage->ApplyTolerance(mz); // absolute half-window

        // Binary search for insertion point
        auto it = std::lower_bound(globalXAxis.begin(), globalXAxis.end(), mz);

        // Find nearest neighbour among iterator and its predecessor
        std::size_t bestCol  = globalXAxis.size(); // sentinel = no match
        double      bestDist = std::numeric_limits<double>::max();

        if (it != globalXAxis.end())
        {
          const std::size_t c = static_cast<std::size_t>(std::distance(globalXAxis.begin(), it));
          const double d = std::abs(globalXAxis[c] - mz);
          if (d < bestDist) { bestDist = d; bestCol = c; }
        }
        if (it != globalXAxis.begin())
        {
          const std::size_t c = static_cast<std::size_t>(std::distance(globalXAxis.begin(), std::prev(it)));
          const double d = std::abs(globalXAxis[c] - mz);
          if (d < bestDist) { bestDist = d; bestCol = c; }
        }

        if (bestCol < colVals.size() && bestDist <= tol)
          colVals[bestCol] = specYs[pi];
      }

      out.insert(out.end(), colVals.begin(), colVals.end());
    }
    else // PLAIN_IMAGE / MULTILABEL
    {
      if (!src.floatImg->GetLargestPossibleRegion().IsInside(idx))
      {
        out.insert(out.end(), src.columnNames.size(),
                   std::numeric_limits<double>::quiet_NaN());
        return;
      }
      out.push_back(static_cast<double>(src.floatImg->GetPixel(idx)));
    }
  };

  // -------------------------------------------------------------------------
  // Open output file and write header
  // -------------------------------------------------------------------------
  std::ofstream ofs(filePath.toLocal8Bit().constData());
  if (!ofs.is_open())
  {
    QMessageBox::critical(m_Parent, "Error",
                          QString("Could not open file for writing:\n%1").arg(filePath));
    return;
  }
  ofs << std::setprecision(6);

  if (pixelWise)
    ofs << "x_world,y_world,z_world,clusterID,groupLabel";
  else
    ofs << "x_center,y_center,z_center,clusterID,size,groupLabel";

  for (const auto &src : sources)
    for (const auto &col : src.columnNames)
      ofs << "," << col;
  ofs << "\n";

  // -------------------------------------------------------------------------
  // Iterate CC image pixels
  // -------------------------------------------------------------------------
  auto *ccGeom              = ccImage->GetGeometry();
  const unsigned int *dims  = ccImage->GetDimensions();
  mitk::ImagePixelReadAccessor<PixelType, 3> ccAcc(ccImage);

  if (pixelWise)
  {
    // -----------------------------------------------------------------------
    // Pixel-wise: one row per non-background voxel
    // -----------------------------------------------------------------------
    for (unsigned int z = 0; z < dims[2]; ++z)
    for (unsigned int y = 0; y < dims[1]; ++y)
    for (unsigned int x = 0; x < dims[0]; ++x)
    {
      itk::Index<3> idx = {{static_cast<itk::IndexValueType>(x),
                            static_cast<itk::IndexValueType>(y),
                            static_cast<itk::IndexValueType>(z)}};
      const PixelType clusterId = ccAcc.GetPixelByIndex(idx);
      if (clusterId == 0) continue;

      // Voxel-centre index → world
      mitk::Point3D mitkIdx, worldPt;
      mitkIdx[0] = static_cast<mitk::ScalarType>(x);
      mitkIdx[1] = static_cast<mitk::ScalarType>(y);
      mitkIdx[2] = static_cast<mitk::ScalarType>(z);
      ccGeom->IndexToWorld(mitkIdx, worldPt);

      const std::size_t ci = static_cast<std::size_t>(clusterId);
      const std::string &groupLabel =
        (ci < labelToGroupName.size()) ? labelToGroupName[ci] : "";

      ofs << worldPt[0] << "," << worldPt[1] << "," << worldPt[2]
          << "," << clusterId << "," << groupLabel;

      for (auto &src : sources)
      {
        std::vector<double> vals;
        sampleSource(src, worldPt, vals);
        for (double v : vals)
          ofs << "," << v;
      }
      ofs << "\n";
    }
  }
  else
  {
    // -----------------------------------------------------------------------
    // Object-wise: accumulate per cluster, write means
    // -----------------------------------------------------------------------
    struct ClusterAccum
    {
      double wx = 0.0, wy = 0.0, wz = 0.0;
      unsigned long count = 0;
      std::vector<double>       featureSum;
      std::vector<unsigned long> featureCount; // non-NaN pixel count per column
    };

    std::unordered_map<PixelType, ClusterAccum> accum;
    accum.reserve(static_cast<std::size_t>(numComponents));

    for (unsigned int z = 0; z < dims[2]; ++z)
    for (unsigned int y = 0; y < dims[1]; ++y)
    for (unsigned int x = 0; x < dims[0]; ++x)
    {
      itk::Index<3> idx = {{static_cast<itk::IndexValueType>(x),
                            static_cast<itk::IndexValueType>(y),
                            static_cast<itk::IndexValueType>(z)}};
      const PixelType clusterId = ccAcc.GetPixelByIndex(idx);
      if (clusterId == 0) continue;

      mitk::Point3D mitkIdx, worldPt;
      mitkIdx[0] = x; mitkIdx[1] = y; mitkIdx[2] = z;
      ccGeom->IndexToWorld(mitkIdx, worldPt);

      auto &a = accum[clusterId];
      if (a.count == 0)
      {
        a.featureSum.assign(totalCols, 0.0);
        a.featureCount.assign(totalCols, 0UL);
      }
      a.wx += worldPt[0]; a.wy += worldPt[1]; a.wz += worldPt[2];
      ++a.count;

      std::vector<double> vals;
      for (auto &src : sources)
        sampleSource(src, worldPt, vals);

      for (std::size_t col = 0; col < vals.size(); ++col)
      {
        if (!std::isnan(vals[col]))
        {
          a.featureSum[col]  += vals[col];
          a.featureCount[col]++;
        }
      }
    }

    // Write one row per cluster, sorted by cluster ID
    std::vector<PixelType> clusterIds;
    clusterIds.reserve(accum.size());
    for (auto &kv : accum)
      clusterIds.push_back(kv.first);
    std::sort(clusterIds.begin(), clusterIds.end());

    for (const PixelType clusterId : clusterIds)
    {
      const auto &a     = accum.at(clusterId);
      const double inv  = 1.0 / static_cast<double>(a.count);
      const std::size_t ci = static_cast<std::size_t>(clusterId);
      const std::string &groupLabel =
        (ci < labelToGroupName.size()) ? labelToGroupName[ci] : "";

      ofs << (a.wx * inv) << "," << (a.wy * inv) << "," << (a.wz * inv)
          << "," << clusterId << "," << a.count << "," << groupLabel;

      for (std::size_t col = 0; col < totalCols; ++col)
      {
        if (a.featureCount[col] > 0)
          ofs << "," << (a.featureSum[col] / static_cast<double>(a.featureCount[col]));
        else
          ofs << "," << std::numeric_limits<double>::quiet_NaN();
      }
      ofs << "\n";
    }
  }

  ofs.close();
  mitk::StatusBar::GetInstance()->DisplayText(
    QString("CSV exported: %1").arg(filePath).toLocal8Bit().constData());
  QMessageBox::information(m_Parent, "Export complete",
                           QString("CSV written to:\n%1").arg(filePath));
}
