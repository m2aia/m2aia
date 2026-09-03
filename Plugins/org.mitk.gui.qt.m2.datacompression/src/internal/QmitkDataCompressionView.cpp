/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkDataCompressionView.h"

#include <QMessageBox>
#include <QtConcurrentRun>

// m2
#include <m2CoreCommon.h>
#include <m2DataNodePredicates.h>
#include <m2EmbeddingResult.h>
#include <m2IcaImageFilter.h>
#include <m2IntervalVector.h>
#include <m2NmfImageFilter.h>
#include <m2PlsDaImageFilter.h>
#include <m2KMeansImageFilter.h>
#include <m2MultiSliceFilter.h>
#include <m2PcaImageFilter.h>
#include <m2SpectralFeatureMatrix.h>
#include <m2SpectrumImage.h>
#include <m2TSNEImageFilter.h>
#include <signal/m2SignalCommon.h>

// mitk
#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkLabelSetImage.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkProgressBar.h>

// std
#include <algorithm>
#include <cmath>

// itksys
#include <itksys/SystemTools.hxx>

// itk
#include <itkIdentityTransform.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkResampleImageFilter.h>
#include <itkVectorImage.h>
#include <itkVectorImageToImageAdaptor.h>

// Don't forget to initialize the VIEW_ID.
const std::string QmitkDataCompressionView::VIEW_ID = "org.mitk.views.m2.datacompression";
using DisplayImageType = itk::Image<m2::DisplayImagePixelType, 3>;
using VectorImageAdaptorType = itk::VectorImageToImageAdaptor<m2::DisplayImagePixelType, 3>;
using VectorImageType = itk::VectorImage<m2::DisplayImagePixelType, 3>;

QmitkDataCompressionView::~QmitkDataCompressionView()
{
  // the worker holds pointers into this view, so it must not outlive it
  if (m_MethodWatcher.isRunning())
  {
    if (m_RunningFilter.IsNotNull())
      m_RunningFilter->RequestCancel();
    m_MethodWatcher.waitForFinished();
  }
}

void QmitkDataCompressionView::CreateQtPartControl(QWidget *parent)
{
  m_Parent = parent;
  // Setting up the UI is a true pleasure when using .ui files, isn't it?
  m_Controls.setupUi(parent);

  m_Controls.imageSelection->SetDataStorage(GetDataStorage());
  m_Controls.imageSelection->SetNodePredicate(mitk::NodePredicateAnd::New(m2::DataNodePredicates::IsSpectrumImage,
                                                                         m2::DataNodePredicates::NoActiveHelper));
  m_Controls.imageSelection->SetSelectionIsOptional(true);
  m_Controls.imageSelection->SetEmptyInfo(QString("Image selection"));
  m_Controls.imageSelection->SetPopUpTitel(QString("Image"));

  m_Controls.peakListSelection->SetDataStorage(GetDataStorage());
  m_Controls.peakListSelection->SetNodePredicate(mitk::NodePredicateAnd::New(
    m2::DataNodePredicates::IsCentroidSpectrum, m2::DataNodePredicates::NoActiveHelper));
  m_Controls.peakListSelection->SetSelectionIsOptional(true);
  m_Controls.peakListSelection->SetEmptyInfo(QString("PeakList selection"));
  m_Controls.peakListSelection->SetPopUpTitel(QString("PeakList"));

  m_Controls.maskSelection->SetDataStorage(GetDataStorage());
  m_Controls.maskSelection->SetNodePredicate(mitk::NodePredicateAnd::New(
    mitk::TNodePredicateDataType<mitk::MultiLabelSegmentation>::New(), m2::DataNodePredicates::NoActiveHelper));
  m_Controls.maskSelection->SetSelectionIsOptional(true);
  m_Controls.maskSelection->SetEmptyInfo(QString("Mask selection"));
  m_Controls.maskSelection->SetPopUpTitel(QString("Mask"));

  m_Controls.boxKMeansDistanceMetric->addItem("Euclidean", QVariant(to_underlying(m2::DistanceMetric::EUCLIDEAN)));
  m_Controls.boxKMeansDistanceMetric->addItem("Cosine", QVariant(to_underlying(m2::DistanceMetric::COSINE)));
  m_Controls.boxKMeansDistanceMetric->addItem("Correlation", QVariant(to_underlying(m2::DistanceMetric::CORRELATION)));
  m_Controls.boxKMeansDistanceMetric->setCurrentIndex(0);

  m_Controls.boxKMeansVariant->addItem("Standard", QVariant(to_underlying(m2::KMeansVariant::STANDARD)));
  m_Controls.boxKMeansVariant->addItem("Spatial", QVariant(to_underlying(m2::KMeansVariant::SPATIAL)));
  m_Controls.boxKMeansVariant->setCurrentIndex(0);

  connect(m_Controls.maskSelection,
          &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged,
          this,
          &QmitkDataCompressionView::OnMaskSelectionChanged);
  OnMaskSelectionChanged();

  connect(m_Controls.btnRunPCA, SIGNAL(clicked()), this, SLOT(OnStartPCA()));
  connect(m_Controls.btnRunKMeans, SIGNAL(clicked()), this, SLOT(OnStartKMeans()));
  connect(m_Controls.btnRunTSNE, SIGNAL(clicked()), this, SLOT(OnStartTSNE()));
  connect(m_Controls.btnRunNMF, SIGNAL(clicked()), this, SLOT(OnStartNMF()));
  connect(m_Controls.btnRunICA, SIGNAL(clicked()), this, SLOT(OnStartICA()));
  connect(m_Controls.btnRunPLSDA, SIGNAL(clicked()), this, SLOT(OnStartPlsDa()));
  connect(m_Controls.btnSaveDataCompressionResults, SIGNAL(clicked()), this, SLOT(OnSaveDataCompressionResults()));
  connect(m_Controls.btnCancelMethod, SIGNAL(clicked()), this, SLOT(OnCancelMethod()));

  connect(&m_MethodWatcher, &QFutureWatcher<MethodRun>::finished, this, &QmitkDataCompressionView::OnMethodFinished);
  // the methods run on a worker thread and report their progress from there
  connect(this,
          &QmitkDataCompressionView::MethodProgress,
          this,
          &QmitkDataCompressionView::OnMethodProgress,
          Qt::QueuedConnection);
}

void QmitkDataCompressionView::SetFocus() {}

void QmitkDataCompressionView::OnSaveDataCompressionResults()
{
  for (auto node : m_Controls.imageSelection->GetSelectedNodesStdVector())
  {
    std::string inputLocation;
    auto m2aiaDataPathProp = node->GetData()->GetProperty("m2aia.IO.path");
    auto dataPathProp = node->GetData()->GetProperty("path");
    if (m2aiaDataPathProp)
      inputLocation = m2aiaDataPathProp->GetValueAsString();
    else if (dataPathProp)
      inputLocation = dataPathProp->GetValueAsString();

    // every result of this view carries the method property, so no method has to be listed here
    auto isResultNode = mitk::NodePredicateFunction::New(
      [](const mitk::DataNode *candidate) -> bool
      { return candidate->GetProperty(M2AIA_DIMENSION_REDUCTION_METHOD_PROPERTY) != nullptr; });

    for (auto child : *this->GetDataStorage()->GetDerivations(node, isResultNode, true))
    {
      auto image = dynamic_cast<mitk::Image *>(child->GetData());
      if (!image)
        continue;

      mitk::IOUtil::Save(image,
                         itksys::SystemTools::GetFilenamePath(inputLocation) + "/" +
                           itksys::SystemTools::GetFilenameWithoutLastExtension(inputLocation) + "." +
                           child->GetName() + ".nrrd");
    }
  }
}

void QmitkDataCompressionView::OnMaskSelectionChanged()
{
  using LabelValueType = mitk::MultiLabelSegmentation::LabelValueType;

  // keep the current choice if that label value is still available
  const auto previousData = m_Controls.boxMaskLabelValue->currentData();

  // count for each label value in how many of the selected masks it occurs
  std::map<LabelValueType, unsigned int> labelValueCounts;
  for (auto maskNode : m_Controls.maskSelection->GetSelectedNodesStdVector())
  {
    auto mask = dynamic_cast<mitk::MultiLabelSegmentation *>(maskNode->GetData());
    if (!mask)
      continue;

    // a label value is unique within a segmentation, but may be used by several of its groups
    const auto labelValues = mask->GetAllLabelValues();
    for (auto labelValue : std::set<LabelValueType>(labelValues.begin(), labelValues.end()))
      ++labelValueCounts[labelValue];
  }

  m_Controls.boxMaskLabelValue->clear();
  for (const auto &kv : labelValueCounts)
    m_Controls.boxMaskLabelValue->addItem(QString("%1 (%2)").arg(kv.first).arg(kv.second),
                                          QVariant::fromValue(kv.first));

  m_Controls.boxMaskLabelValue->setEnabled(!labelValueCounts.empty());
  if (labelValueCounts.empty())
    return;

  const auto previousIndex = m_Controls.boxMaskLabelValue->findData(previousData);
  m_Controls.boxMaskLabelValue->setCurrentIndex(previousIndex < 0 ? 0 : previousIndex);
}

namespace
{
  /** A mask can only be applied to an image if both cover the same pixel grid. */
  bool HasMatchingGrid(const mitk::Image *image, const mitk::MultiLabelSegmentation *mask)
  {
    const auto &maskDimensions = mask->GetDimensions();
    if (image->GetDimension() != maskDimensions.size())
      return false;

    for (unsigned int i = 0; i < image->GetDimension(); ++i)
      if (image->GetDimensions()[i] != maskDimensions[i])
        return false;

    return true;
  }

  /** Puts an image onto the given geometry, provided it covers the same grid. Results are built
      as plain itk images and pass through conversions that do not carry a geometry along, so the
      one they end up with has to be set explicitly.

      The whole time geometry is taken over, not just the spatial one: mitk::Image::Initialize
      does not set the time bounds of a single time step correctly, and
      mitk::MultiLabelSegmentation::InsertGroup compares the time geometries and rejects anything
      that differs. This follows what MultiLabelSegmentation::GenerateNewGroupImage does for the
      group images it creates itself. */
  void AdoptGeometry(const mitk::Image::Pointer &image,
                     const mitk::TimeGeometry *geometry,
                     const unsigned int *expectedDimensions,
                     const std::string &name)
  {
    const auto *dimensions = image->GetDimensions();
    if (!std::equal(dimensions, dimensions + 3, expectedDimensions))
    {
      MITK_WARN << "The result [" << name
                << "] does not cover the grid it belongs to; it keeps the geometry it was computed on.";
      return;
    }

    image->SetClonedTimeGeometry(geometry);
    image->GetTimeGeometry()->UpdateBoundingBox();
  }

  /** Distinguishable colors in a fixed order, so that a cluster keeps its color between runs. */
  mitk::Color ClusterColor(unsigned int cluster)
  {
    // the golden angle spreads consecutive clusters as far apart on the hue circle as possible
    const double hue = std::fmod(cluster * 0.618033988749895, 1.0) * 6.0;
    const int sector = static_cast<int>(hue);
    const double fraction = hue - sector;
    const double p = 0.25, q = 1.0 - 0.75 * fraction, t = 0.25 + 0.75 * fraction;

    switch (sector)
    {
      case 0:
        return mitk::MakeColor(1.0, t, p);
      case 1:
        return mitk::MakeColor(q, 1.0, p);
      case 2:
        return mitk::MakeColor(p, 1.0, t);
      case 3:
        return mitk::MakeColor(p, q, 1.0);
      case 4:
        return mitk::MakeColor(t, p, 1.0);
      default:
        return mitk::MakeColor(1.0, p, q);
    }
  }
} // namespace

mitk::MultiLabelSegmentation::LabelValueType QmitkDataCompressionView::GetSelectedMaskLabelValue() const
{
  const auto data = m_Controls.boxMaskLabelValue->currentData();
  return data.isValid() ? data.value<mitk::MultiLabelSegmentation::LabelValueType>()
                        : mitk::MultiLabelSegmentation::UNLABELED_VALUE;
}

mitk::DataNode::ConstPointer QmitkDataCompressionView::GetMaskNode(const mitk::DataNode *imageNode)
{
  auto image = dynamic_cast<mitk::Image *>(imageNode->GetData());
  if (!image)
    return nullptr;

  const auto &derivations = this->GetDataStorage()->GetDerivations(imageNode)->CastToSTLConstContainer();

  mitk::DataNode::ConstPointer candidate;
  for (auto maskNode : m_Controls.maskSelection->GetSelectedNodesStdVector())
  {
    auto mask = dynamic_cast<mitk::MultiLabelSegmentation *>(maskNode->GetData());
    if (!mask || !HasMatchingGrid(image, mask))
      continue;

    // a mask below the image in the data hierarchy belongs to it unambiguously
    const auto isDerivation =
      std::any_of(derivations.begin(),
                  derivations.end(),
                  [&maskNode](const auto &node) { return node.GetPointer() == maskNode.GetPointer(); });
    if (isDerivation)
      return maskNode;

    if (candidate.IsNull())
      candidate = maskNode;
  }

  return candidate;
}

mitk::Image::Pointer QmitkDataCompressionView::GetMaskImage(const mitk::DataNode *imageNode)
{
  auto image = dynamic_cast<m2::SpectrumImage *>(imageNode->GetData());
  if (!image)
    return nullptr;

  auto maskNode = GetMaskNode(imageNode);
  if (maskNode.IsNull())
  {
    if (!m_Controls.maskSelection->GetSelectedNodesStdVector().empty())
      MITK_WARN << "None of the selected masks covers the grid of image [" << imageNode->GetName()
                << "]; its own segmentation is used instead.";
    return image->GetMultilabelSegmentation()->GetGroupImage(0);
  }

  auto mask = dynamic_cast<mitk::MultiLabelSegmentation *>(maskNode->GetData());
  const auto labelValue = GetSelectedMaskLabelValue();
  if (!mask->ExistLabel(labelValue))
  {
    MITK_WARN << "The mask [" << maskNode->GetName() << "] of image [" << imageNode->GetName()
              << "] does not contain the label value " << labelValue << "; the image is skipped.";
    return nullptr;
  }

  // only the pixels of the chosen label value are kept, all others are masked out
  mitk::Image::Pointer groupImage = mask->GetGroupImage(mask->GetGroupIndexOfLabel(labelValue));
  auto labelValueMask = groupImage->Clone();
  {
    mitk::ImagePixelWriteAccessor<mitk::MultiLabelSegmentation::LabelValueType, 3> acc(labelValueMask);
    auto data = acc.GetData();
    const auto dimensions = labelValueMask->GetDimensions();
    const auto n = dimensions[0] * dimensions[1] * dimensions[2];
    std::transform(data,
                   data + n,
                   data,
                   [labelValue](const auto value) -> mitk::MultiLabelSegmentation::LabelValueType
                   { return value == labelValue ? 1 : 0; });
  }

  return labelValueMask;
}

std::vector<QmitkDataCompressionView::Input> QmitkDataCompressionView::GetUsableInputs()
{
  std::vector<Input> inputs;

  for (auto imageNode : m_Controls.imageSelection->GetSelectedNodesStdVector())
  {
    auto image = dynamic_cast<m2::SpectrumImage *>(imageNode->GetData());
    if (!image || !image->GetImageAccessInitialized())
    {
      MITK_WARN << "The image [" << imageNode->GetName() << "] is not ready to be read; it is skipped.";
      continue;
    }

    auto mask = GetMaskImage(imageNode);
    if (mask.IsNull())
      continue;

    inputs.emplace_back(imageNode, mask);
  }

  return inputs;
}

std::vector<QmitkDataCompressionView::Input> QmitkDataCompressionView::GetSupervisedInputs()
{
  std::vector<Input> inputs;

  for (auto imageNode : m_Controls.imageSelection->GetSelectedNodesStdVector())
  {
    auto image = dynamic_cast<m2::SpectrumImage *>(imageNode->GetData());
    if (!image || !image->GetImageAccessInitialized())
    {
      MITK_WARN << "The image [" << imageNode->GetName() << "] is not ready to be read; it is skipped.";
      continue;
    }

    auto maskNode = GetMaskNode(imageNode);
    if (maskNode.IsNull())
    {
      MITK_WARN << "No selected mask covers the grid of image [" << imageNode->GetName()
                << "]; a supervised method cannot fall back to the segmentation of the image, which marks "
                   "the valid spectra rather than any classes.";
      continue;
    }

    auto mask = dynamic_cast<mitk::MultiLabelSegmentation *>(maskNode->GetData());
    const auto labelValue = GetSelectedMaskLabelValue();
    if (!mask->ExistLabel(labelValue))
    {
      MITK_WARN << "The mask [" << maskNode->GetName() << "] of image [" << imageNode->GetName()
                << "] does not contain the label value " << labelValue << "; the image is skipped.";
      continue;
    }

    // the chosen label value picks the group; every label of that group is then a class
    const auto groupIndex = mask->GetGroupIndexOfLabel(labelValue);
    auto classValues = mask->GetLabelValuesByGroup(groupIndex);
    classValues.erase(std::remove(classValues.begin(), classValues.end(), mitk::MultiLabelSegmentation::UNLABELED_VALUE),
                      classValues.end());

    if (classValues.size() < 2)
    {
      MITK_WARN << "The mask [" << maskNode->GetName() << "] of image [" << imageNode->GetName() << "] marks only "
                << classValues.size() << " class; a supervised method needs at least two.";
      continue;
    }

    // the group image is passed as it is, not binarized: its label values restrict the pixels that
    // take part and name the class of each of them at the same time
    inputs.emplace_back(imageNode, mask->GetGroupImage(groupIndex));
  }

  return inputs;
}

std::vector<m2::Interval> QmitkDataCompressionView::GetSelectedIntervals() const
{
  for (auto vectorNode : m_Controls.peakListSelection->GetSelectedNodesStdVector())
    if (auto vector = dynamic_cast<m2::IntervalVector *>(vectorNode->GetData()))
      return vector->GetIntervals();

  return {};
}

mitk::DataNode::ConstPointer QmitkDataCompressionView::GetSelectedPeakListNode() const
{
  for (auto vectorNode : m_Controls.peakListSelection->GetSelectedNodesStdVector())
    if (dynamic_cast<m2::IntervalVector *>(vectorNode->GetData()))
      return vectorNode;

  return nullptr;
}

std::string QmitkDataCompressionView::GetSelectedPeakListName() const
{
  for (auto vectorNode : m_Controls.peakListSelection->GetSelectedNodesStdVector())
    return vectorNode->GetName();

  return {};
}

void QmitkDataCompressionView::StartNextPendingRun()
{
  // an image whose run does not start, because it has no usable data, must not stall the queue
  while (!m_PendingInputs.empty() && m_StartRunForInput)
  {
    const auto input = m_PendingInputs.front();
    m_PendingInputs.erase(m_PendingInputs.begin());

    m_StartRunForInput(input);
    if (m_MethodWatcher.isRunning())
      return;

    MITK_WARN << "The image [" << input.first->GetName() << "] was skipped.";
  }
}

void QmitkDataCompressionView::SetMethodRunning(bool running)
{
  m_Controls.TabWidget->setEnabled(!running);
  m_Controls.imageSelection->setEnabled(!running);
  m_Controls.peakListSelection->setEnabled(!running);
  m_Controls.maskSelection->setEnabled(!running);
  m_Controls.boxMaskLabelValue->setEnabled(!running && m_Controls.boxMaskLabelValue->count() > 0);
  m_Controls.btnSaveDataCompressionResults->setEnabled(!running);
  m_Controls.btnCancelMethod->setEnabled(running);

  if (!running)
    m_Controls.labelMethodStatus->clear();
}

void QmitkDataCompressionView::OnCancelMethod()
{
  if (m_RunningFilter.IsNotNull())
  {
    // the images that have not been started yet are dropped along with the running one
    m_PendingInputs.clear();
    m_RunningFilter->RequestCancel();
    m_Controls.labelMethodStatus->setText("Cancelling ...");
    m_Controls.btnCancelMethod->setEnabled(false);
  }
}

void QmitkDataCompressionView::OnMethodProgress(unsigned int done, unsigned int total)
{
  if (!m_MethodWatcher.isRunning())
    return;

  m_Controls.labelMethodStatus->setText(
    QString("%1 ... %2/%3").arg(QString::fromStdString(m_RunningOptions.resultName)).arg(done).arg(total));
}

bool QmitkDataCompressionView::StartMethod(m2::EmbeddingFilterBase::Pointer filter,
                                           m2::SpectralFeatureMatrix features,
                                           std::vector<mitk::DataNode::ConstPointer> sourceNodes,
                                           const MethodOptions &options)
{
  if (m_MethodWatcher.isRunning())
  {
    QMessageBox::information(nullptr, "Busy", "Another method is still running.");
    return false;
  }

  if (features.IsEmpty())
  {
    // the caller walks a queue of images, so this one is only reported and left out
    MITK_WARN << options.resultName << " has no pixel to work on; the image is left out.";
    return false;
  }

  filter->SetFeatures(std::move(features));
  filter->SetProgressCallback([this](unsigned int done, unsigned int total) { emit MethodProgress(done, total); });

  m_RunningFilter = filter;
  m_RunningOptions = options;
  SetMethodRunning(true);
  m_Controls.labelMethodStatus->setText(QString("%1 ...").arg(QString::fromStdString(options.resultName)));

  // only the computation runs here; the spectrum images are read on the GUI thread because their
  // data access is shared with the renderer
  m_MethodWatcher.setFuture(QtConcurrent::run(
    [this, filter, sourceNodes, options]() -> MethodRun
    {
      filter->Update();
      return CollectResults(filter, sourceNodes, options);
    }));

  return true;
}

QmitkDataCompressionView::MethodRun QmitkDataCompressionView::CollectResults(
  m2::EmbeddingFilterBase *filter,
  const std::vector<mitk::DataNode::ConstPointer> &sourceNodes,
  const MethodOptions &options) const
{
  MethodRun run;
  run.methodName = filter->GetMethodName();
  run.producesLabels = filter->ProducesLabels();
  run.numberOfClusters = options.numberOfClusters;

  if (filter->IsCancelRequested())
    return run;

  const auto &features = filter->GetFeatures();
  auto embedding = filter->GetEmbedding();

  if (!run.producesLabels && embedding.cols() == 0)
  {
    MITK_ERROR << run.methodName << " did not produce an embedding.";
    return run;
  }

  if (options.convertToRGB)
    m2::EmbeddingResult::RescaleToByteRange(embedding, 0, static_cast<unsigned int>(embedding.rows()));

  for (unsigned int imageIndex = 0; imageIndex < features.GetNumberOfImages(); ++imageIndex)
  {
    if (imageIndex >= sourceNodes.size())
      break;

    MethodResult result;
    result.sourceNode = sourceNodes[imageIndex];
    result.name = options.prefixWithSourceName ? result.sourceNode->GetName() + "." + options.resultName
                                               : options.resultName;

    result.image = run.producesLabels ? m2::EmbeddingResult::ToLabelImage(filter->GetLabels(), features, imageIndex)
                                      : m2::EmbeddingResult::ToVectorImage(embedding, features, imageIndex);

    if (result.image.IsNull())
      continue;

    if (options.resampleToSource)
    {
      auto sourceImage = dynamic_cast<mitk::Image *>(result.sourceNode->GetData());
      result.image = const_cast<QmitkDataCompressionView *>(this)->ResampleVectorImage(result.image, sourceImage);
    }

    if (options.convertToRGB)
      result.image = m2::MultiSliceFilter::ConvertMitkVectorImageToRGB(result.image);

    run.results.push_back(result);

    // the loadings belong to the run as a whole, not to a pixel grid, so they are collected next
    // to the images and only for the methods that express their components over the peaks
    if (options.publishLoadings && filter->GetLoadings().size() > 0)
    {
      MethodLoadings loadings;
      loadings.sourceNode = result.sourceNode;
      loadings.peakListNode = options.peakListNode;
      loadings.values = filter->GetLoadings();
      run.loadings.push_back(loadings);
    }
  }

  return run;
}

void QmitkDataCompressionView::OnMethodFinished()
{
  const auto run = m_MethodWatcher.result();
  const bool cancelled = m_RunningFilter.IsNotNull() && m_RunningFilter->IsCancelRequested();

  m_RunningFilter = nullptr;
  SetMethodRunning(false);

  if (cancelled)
  {
    MITK_INFO << run.methodName << " was cancelled.";
    m_PendingInputs.clear();
    return;
  }

  for (const auto &result : run.results)
  {
    if (run.producesLabels)
      PublishSegmentation(result, run.methodName, run.numberOfClusters);
    else
      PublishImage(result, run.methodName);
  }

  for (const auto &loadings : run.loadings)
    AttachLoadings(loadings, run.methodName);

  this->RequestRenderWindowUpdate();

  // methods that treat every image on its own continue with the next one, once this slot has
  // returned; setting a new future from inside the finished signal of the watcher is not safe
  QMetaObject::invokeMethod(this, "StartNextPendingRun", Qt::QueuedConnection);
}

void QmitkDataCompressionView::PublishImage(const MethodResult &result, const std::string &methodName)
{
  auto sourceNode = const_cast<mitk::DataNode *>(result.sourceNode.GetPointer());

  // the conversions on the way here pass through itk, where a geometry is easily lost, so the
  // result is put back onto the geometry of the image it has to overlay
  if (auto sourceImage = dynamic_cast<mitk::Image *>(sourceNode->GetData()))
    AdoptGeometry(result.image, sourceImage->GetTimeGeometry(), sourceImage->GetDimensions(), result.name);

  // a second run of the same method replaces its earlier result instead of piling up nodes
  if (auto child = this->GetDataStorage()->GetNamedDerivedNode(result.name.c_str(), sourceNode))
  {
    child->SetData(result.image);
    child->SetStringProperty(M2AIA_DIMENSION_REDUCTION_METHOD_PROPERTY, methodName.c_str());
    return;
  }

  auto outputNode = mitk::DataNode::New();
  outputNode->SetData(result.image);
  outputNode->SetName(result.name);
  outputNode->SetStringProperty(M2AIA_DIMENSION_REDUCTION_METHOD_PROPERTY, methodName.c_str());
  this->GetDataStorage()->Add(outputNode, sourceNode);
}

void QmitkDataCompressionView::PublishSegmentation(const MethodResult &result,
                                                   const std::string &methodName,
                                                   unsigned int numberOfClusters)
{
  auto sourceNode = const_cast<mitk::DataNode *>(result.sourceNode.GetPointer());
  auto sourceImage = dynamic_cast<m2::SpectrumImage *>(sourceNode->GetData());
  if (!sourceImage)
    return;

  // The segmentation is built from the label image rather than by inserting the label image into
  // a clone of the segmentation of the source image: InsertGroup compares the whole time geometry
  // of the two and rejects anything that differs, which no amount of copying the geometry over
  // reliably satisfies. InitializeByLabeledImage takes the geometry from the label image, so the
  // result is consistent by construction. It also means the result does not inherit any further
  // groups the segmentation of the source image may have.
  auto segmentation = mitk::MultiLabelSegmentation::New();
  try
  {
    segmentation->InitializeByLabeledImage(result.image);
  }
  catch (const mitk::Exception &e)
  {
    MITK_ERROR << "The " << methodName << " result of [" << sourceNode->GetName()
               << "] could not be turned into a segmentation: " << e.GetDescription();
    return;
  }

  segmentation->SetGroupName(0, result.name);

  // InitializeByLabeledImage creates one label per value it finds, so a cluster that stayed empty
  // has none; the label value zero is the unlabeled value and never a cluster
  for (unsigned int cluster = 1; cluster <= numberOfClusters; ++cluster)
  {
    auto label = segmentation->GetLabel(cluster);
    if (nullptr == label)
      continue;

    label->SetName("Cluster " + std::to_string(cluster));
    label->SetColor(ClusterColor(cluster - 1));
    // transparency is a matter of the node, never of a single label
    label->SetOpacity(1.0f);
    segmentation->UpdateLookupTable(cluster);
  }

  if (segmentation->GetNumberOfLabels(0) > 0)
    segmentation->SetActiveLabel(segmentation->GetLabelValuesByGroup(0).front());

  auto outputNode = mitk::DataNode::New();
  outputNode->SetData(segmentation);
  outputNode->SetName(result.name);
  outputNode->SetStringProperty(M2AIA_DIMENSION_REDUCTION_METHOD_PROPERTY, methodName.c_str());

  // one node level control blends the whole segmentation over the image
  float opacity;
  if (!outputNode->GetOpacity(opacity, nullptr))
    outputNode->SetOpacity(0.75f);

  this->GetDataStorage()->Add(outputNode, sourceNode);
}

void QmitkDataCompressionView::AttachLoadings(const MethodLoadings &loadings, const std::string &methodName)
{
  if (loadings.peakListNode.IsNull())
    return;

  auto peakList = dynamic_cast<m2::IntervalVector *>(loadings.peakListNode->GetData());
  if (!peakList)
    return;

  const auto numberOfPeaks = static_cast<Eigen::Index>(peakList->GetIntervals().size());
  if (loadings.values.cols() != numberOfPeaks)
  {
    MITK_WARN << "The loadings of " << methodName << " describe " << loadings.values.cols()
              << " peaks but the peak list [" << loadings.peakListNode->GetName() << "] now holds " << numberOfPeaks
              << "; they are not attached.";
    return;
  }

  // one feature per component, named after the image and the method, so that the loadings of
  // several runs can be read next to each other for the same centroid
  for (Eigen::Index component = 0; component < loadings.values.rows(); ++component)
  {
    const auto name = loadings.sourceNode->GetName() + "." + methodName + ".c" + std::to_string(component + 1);

    std::vector<double> values(static_cast<size_t>(numberOfPeaks));
    for (Eigen::Index peak = 0; peak < numberOfPeaks; ++peak)
      values[peak] = loadings.values(component, peak);

    peakList->SetFeature(name, values);
  }

  MITK_INFO << methodName << " attached " << loadings.values.rows() << " loading(s) to the peak list ["
            << loadings.peakListNode->GetName() << "]";
}

void QmitkDataCompressionView::StartPerImageMethod(const std::string &resultName,
                                                   unsigned int numberOfComponents,
                                                   std::vector<Input> inputs,
                                                   std::function<m2::EmbeddingFilterBase::Pointer()> createFilter)
{
  const auto intervals = GetSelectedIntervals();
  if (intervals.size() <= 2)
  {
    QMessageBox::warning(nullptr, "Select a peak list first!", "Select at least three peaks!");
    return;
  }

  if (inputs.empty())
  {
    QMessageBox::warning(nullptr, "Select an image first!", "Select at least one image with a valid mask!");
    return;
  }

  const bool publishLoadings = m_Controls.checkPublishLoadings->isChecked();
  const auto peakListNode = GetSelectedPeakListNode();

  // every image is treated on its own, so each of them gets its own set of components
  m_PendingInputs = inputs;
  m_StartRunForInput = [this, intervals, numberOfComponents, resultName, createFilter, publishLoadings, peakListNode](
                         const Input &input)
  {
    m2::SpectralFeatureMatrixBuilder builder;
    builder.AddImage(dynamic_cast<m2::SpectrumImage *>(input.first->GetData()), input.second);

    auto progressBar = mitk::ProgressBar::GetInstance();
    progressBar->AddStepsToDo(intervals.size());
    builder.SetProgressCallback([progressBar](unsigned int, unsigned int) { progressBar->Progress(); });

    auto filter = createFilter();
    filter->SetNumberOfComponents(numberOfComponents);

    MethodOptions options;
    options.resultName = resultName;
    options.publishLoadings = publishLoadings;
    options.peakListNode = peakListNode;

    StartMethod(filter, builder.Build(intervals), {input.first}, options);
  };

  StartNextPendingRun();
}

void QmitkDataCompressionView::OnStartNMF()
{
  const auto iterations = static_cast<unsigned int>(m_Controls.nmf_iterations->value());
  const auto tolerance = m_Controls.nmf_tolerance->value();

  StartPerImageMethod("NMF",
                      static_cast<unsigned int>(m_Controls.nmf_components->value()),
                      GetUsableInputs(),
                      [iterations, tolerance]() -> m2::EmbeddingFilterBase::Pointer
                      {
                        auto filter = m2::NmfImageFilter::New();
                        filter->SetIterations(iterations);
                        filter->SetTolerance(tolerance);
                        return filter.GetPointer();
                      });
}

void QmitkDataCompressionView::OnStartICA()
{
  const auto iterations = static_cast<unsigned int>(m_Controls.ica_iterations->value());
  const auto tolerance = m_Controls.ica_tolerance->value();

  StartPerImageMethod("ICA",
                      static_cast<unsigned int>(m_Controls.ica_components->value()),
                      GetUsableInputs(),
                      [iterations, tolerance]() -> m2::EmbeddingFilterBase::Pointer
                      {
                        auto filter = m2::IcaImageFilter::New();
                        filter->SetIterations(iterations);
                        filter->SetTolerance(tolerance);
                        return filter.GetPointer();
                      });
}

void QmitkDataCompressionView::OnStartPlsDa()
{
  // a supervised method cannot be started at all without classes to separate, so this is checked
  // before anything is computed rather than reported once the run has failed
  auto inputs = GetSupervisedInputs();
  if (inputs.empty())
  {
    QMessageBox::warning(
      nullptr,
      "Select a mask with at least two classes!",
      "PLS-DA is supervised: it needs to be told which regions to tell apart.\n\n"
      "Select a mask whose labels mark those regions. Every label value of the group the chosen "
      "label belongs to is one class, and at least two of them are needed. The label value 0 is "
      "the background and is not a class.");
    return;
  }

  const auto autoScale = m_Controls.plsda_autoscale->isChecked();

  StartPerImageMethod("PLSDA",
                      static_cast<unsigned int>(m_Controls.plsda_components->value()),
                      inputs,
                      [autoScale]() -> m2::EmbeddingFilterBase::Pointer
                      {
                        auto filter = m2::PlsDaImageFilter::New();
                        filter->SetAutoScale(autoScale);
                        return filter.GetPointer();
                      });
}

void QmitkDataCompressionView::OnStartPCA()
{
  StartPerImageMethod("PCA",
                      static_cast<unsigned int>(m_Controls.pca_dims->value()),
                      GetUsableInputs(),
                      []() -> m2::EmbeddingFilterBase::Pointer
                      { return m2::PcaImageFilter::New().GetPointer(); });
}

void QmitkDataCompressionView::OnStartTSNE()
{
  auto inputs = GetUsableInputs();

  // t-SNE embeds the components of the PCA, so an image without one cannot take part
  inputs.erase(std::remove_if(inputs.begin(),
                              inputs.end(),
                              [this](const Input &input)
                              {
                                const auto pcaName = input.first->GetName() + ".PCA";
                                if (this->GetDataStorage()->GetNamedDerivedNode(pcaName.c_str(), input.first))
                                  return false;
                                MITK_WARN << "The image [" << input.first->GetName()
                                          << "] has no PCA result yet; it is skipped.";
                                return true;
                              }),
               inputs.end());

  if (inputs.empty())
  {
    QMessageBox::warning(nullptr, "Run the PCA first!", "t-SNE embeds the components of the PCA of an image.");
    return;
  }

  const auto perplexity = static_cast<unsigned int>(m_Controls.tsne_perplexity->value());
  const auto iterations = static_cast<unsigned int>(m_Controls.tnse_iters->value());
  const auto theta = m_Controls.tsne_theta->value();
  const auto shrinkFactor = static_cast<unsigned int>(m_Controls.tsne_shrink->value());

  m_PendingInputs = inputs;
  m_StartRunForInput = [this, perplexity, iterations, theta, shrinkFactor](const Input &input)
  {
    const auto pcaName = input.first->GetName() + ".PCA";
    auto pcaChild = this->GetDataStorage()->GetNamedDerivedNode(pcaName.c_str(), input.first);

    m2::SpectralFeatureMatrixBuilder builder;
    builder.AddImage(dynamic_cast<m2::SpectrumImage *>(input.first->GetData()), input.second);
    builder.SetShrinkFactor(shrinkFactor);
    // t-SNE compares distances, so every component has to carry the same weight
    builder.SetScaling(m2::FeatureScaling::Standardized);

    auto filter = m2::TSNEImageFilter::New();
    filter->SetPerplexity(perplexity);
    filter->SetIterations(iterations);
    filter->SetTheta(theta);
    filter->SetNumberOfComponents(3);

    MethodOptions options;
    options.resultName = "tSNE";
    options.convertToRGB = true;
    options.resampleToSource = shrinkFactor > 1;

    StartMethod(filter.GetPointer(),
                builder.BuildFromComponentImage(dynamic_cast<mitk::Image *>(pcaChild->GetData())),
                {input.first},
                options);
  };

  StartNextPendingRun();
}

void QmitkDataCompressionView::OnStartKMeans()
{
  const auto intervals = GetSelectedIntervals();
  const auto peakListName = GetSelectedPeakListName();
  if (intervals.empty())
  {
    QMessageBox::warning(nullptr, "Select a peak list first!", "Select a list of centroids!");
    return;
  }

  const auto inputs = GetUsableInputs();
  if (inputs.empty())
  {
    QMessageBox::warning(nullptr, "Select an image first!", "Select at least one image with a valid mask!");
    return;
  }

  const auto metricType =
    m_Controls.boxKMeansDistanceMetric->itemData(m_Controls.boxKMeansDistanceMetric->currentIndex())
      .value<m2::DistanceMetric>();
  const auto variantType = m_Controls.boxKMeansVariant->itemData(m_Controls.boxKMeansVariant->currentIndex())
                             .value<m2::KMeansVariant>();

  // all images are clustered together so that a cluster means the same thing in all of them
  m2::SpectralFeatureMatrixBuilder builder;
  std::vector<mitk::DataNode::ConstPointer> sourceNodes;
  for (const auto &input : inputs)
  {
    builder.AddImage(dynamic_cast<m2::SpectrumImage *>(input.first->GetData()), input.second);
    sourceNodes.push_back(input.first);
  }
  // only the pixels that carry a spectrum take part, and every peak is weighted equally
  builder.SetIndexSource(m2::FeatureIndexSource::Spectra);
  builder.SetScaling(m2::FeatureScaling::MinMax);

  auto progressBar = mitk::ProgressBar::GetInstance();
  progressBar->AddStepsToDo(inputs.size() * intervals.size());
  builder.SetProgressCallback([progressBar](unsigned int, unsigned int) { progressBar->Progress(); });

  // k-means runs once over all images, so nothing stays queued afterwards
  m_PendingInputs.clear();
  m_StartRunForInput = nullptr;

  auto filter = m2::KMeansImageFilter::New();
  filter->SetNumberOfClusters(m_Controls.kmeans_clusters->value());
  filter->SetDistanceMetric(metricType);
  filter->SetKMeansVariant(variantType);
  filter->SetSpatialWeight(m_Controls.spatialWeight->value());

  MethodOptions options;
  options.resultName =
    "KMeans_" + std::to_string(m_Controls.kmeans_clusters->value()) + (peakListName.empty() ? "" : "_" + peakListName);
  options.prefixWithSourceName = false;
  options.numberOfClusters = m_Controls.kmeans_clusters->value();

  StartMethod(filter.GetPointer(), builder.Build(intervals), sourceNodes, options);
}

mitk::Image::Pointer QmitkDataCompressionView::ResampleVectorImage(mitk::Image::Pointer vectorImage,
                                                                   mitk::Image::Pointer referenceImage)
{
  const unsigned int components = vectorImage->GetPixelType().GetNumberOfComponents();

  VectorImageType::Pointer vectorImageItk;
  mitk::CastToItkImage(vectorImage, vectorImageItk);

  DisplayImageType::Pointer referenceImageItk;
  mitk::CastToItkImage(referenceImage, referenceImageItk);

  auto resampledVectorImageItk = VectorImageType::New();
  resampledVectorImageItk->SetOrigin(referenceImageItk->GetOrigin());
  resampledVectorImageItk->SetDirection(referenceImageItk->GetDirection());
  resampledVectorImageItk->SetSpacing(referenceImageItk->GetSpacing());
  resampledVectorImageItk->SetRegions(referenceImageItk->GetLargestPossibleRegion());
  resampledVectorImageItk->SetNumberOfComponentsPerPixel(components);
  resampledVectorImageItk->Allocate();
  itk::VariableLengthVector<m2::DisplayImagePixelType> v(components);
  v.Fill(0);
  resampledVectorImageItk->FillBuffer(v);

  auto inAdaptor = VectorImageAdaptorType::New();
  auto outAdaptor = VectorImageAdaptorType::New();
  using LinearInterpolatorType = itk::LinearInterpolateImageFunction<VectorImageAdaptorType>;
  using TransformType = itk::IdentityTransform<double, 3>;

  for (unsigned int i = 0; i < components; ++i)
  {
    inAdaptor->SetExtractComponentIndex(i);
    inAdaptor->SetImage(vectorImageItk);
    inAdaptor->SetOrigin(vectorImageItk->GetOrigin());
    inAdaptor->SetDirection(vectorImageItk->GetDirection());
    inAdaptor->SetSpacing(vectorImageItk->GetSpacing());

    outAdaptor->SetExtractComponentIndex(i);
    outAdaptor->SetImage(resampledVectorImageItk);

    auto resampler = itk::ResampleImageFilter<VectorImageAdaptorType, DisplayImageType>::New();
    resampler->SetInput(inAdaptor);
    resampler->SetOutputParametersFromImage(referenceImageItk);
    resampler->SetInterpolator(LinearInterpolatorType::New());
    resampler->SetTransform(TransformType::New());
    resampler->Update();

    itk::ImageAlgorithm::Copy<DisplayImageType, VectorImageAdaptorType>(
      resampler->GetOutput(),
      outAdaptor,
      resampler->GetOutput()->GetLargestPossibleRegion(),
      outAdaptor->GetLargestPossibleRegion());
  }

  mitk::Image::Pointer outImage;
  mitk::CastToMitkImage(resampledVectorImageItk, outImage);

  return outImage;
}
