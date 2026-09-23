/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "m2PlotView.h"
#include "m2PointCloudWidget.h"
#include "ui_m2PlotViewControls.h"

#include <QLocale>
#include <QVBoxLayout>

#include <m2DataNodePredicates.h>
#include <m2IntervalVector.h>

#include <mitkImage.h>
#include <mitkImageReadAccessor.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateFunction.h>

#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkVariant.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <utility>

const std::string m2PlotView::VIEW_ID = "org.mitk.views.m2.plot";

namespace
{
  /** What the colour box carries, so that a mode survives the box being repopulated. */
  const char *const COLOR_UNIFORM = "uniform";
  const char *const COLOR_MASK = "mask";
  const char *const COLOR_POSITION = "position";
  const char *const COLOR_RGB = "rgb";
  /** Which of the selected embeddings a point came from. */
  const char *const COLOR_INPUT = "input";
  /** The position of the pixel in the image, as a colour. */
  const char *const COLOR_PIXEL_RGB = "pixelrgb";
  /** Followed by the index of the dimension. */
  const char *const COLOR_DIMENSION = "dimension:";

  /** One component of one pixel, whatever the image stores its values as. Results published by
      the Data Compression view are float, ones read back from a container are whatever the file
      held, so the type is not assumed. */
  double ReadComponent(const void *data, mitk::PixelType::ItkIOComponentType type, std::size_t offset)
  {
    switch (type)
    {
      case itk::IOComponentEnum::FLOAT:
        return static_cast<const float *>(data)[offset];
      case itk::IOComponentEnum::DOUBLE:
        return static_cast<const double *>(data)[offset];
      case itk::IOComponentEnum::CHAR:
        return static_cast<const char *>(data)[offset];
      case itk::IOComponentEnum::UCHAR:
        return static_cast<const unsigned char *>(data)[offset];
      case itk::IOComponentEnum::SHORT:
        return static_cast<const short *>(data)[offset];
      case itk::IOComponentEnum::USHORT:
        return static_cast<const unsigned short *>(data)[offset];
      case itk::IOComponentEnum::INT:
        return static_cast<const int *>(data)[offset];
      case itk::IOComponentEnum::UINT:
        return static_cast<const unsigned int *>(data)[offset];
      case itk::IOComponentEnum::LONG:
        return static_cast<const long *>(data)[offset];
      case itk::IOComponentEnum::ULONG:
        return static_cast<const unsigned long *>(data)[offset];
      case itk::IOComponentEnum::LONGLONG:
        return static_cast<const long long *>(data)[offset];
      case itk::IOComponentEnum::ULONGLONG:
        return static_cast<const unsigned long long *>(data)[offset];
      default:
        return 0.0;
    }
  }

  bool IsReadableComponentType(mitk::PixelType::ItkIOComponentType type)
  {
    switch (type)
    {
      case itk::IOComponentEnum::FLOAT:
      case itk::IOComponentEnum::DOUBLE:
      case itk::IOComponentEnum::CHAR:
      case itk::IOComponentEnum::UCHAR:
      case itk::IOComponentEnum::SHORT:
      case itk::IOComponentEnum::USHORT:
      case itk::IOComponentEnum::INT:
      case itk::IOComponentEnum::UINT:
      case itk::IOComponentEnum::LONG:
      case itk::IOComponentEnum::ULONG:
      case itk::IOComponentEnum::LONGLONG:
      case itk::IOComponentEnum::ULONGLONG:
        return true;
      default:
        return false;
    }
  }

  /** The number of pixels an image holds, counting a two dimensional one as one slice. */
  std::size_t PixelCountOf(const mitk::Image *image)
  {
    const auto *dimensions = image->GetDimensions();
    const std::size_t sizeZ = image->GetDimension() > 2 ? dimensions[2] : 1;
    return static_cast<std::size_t>(dimensions[0]) * dimensions[1] * sizeZ;
  }

  /** Where a pixel of the grid lies in an image that covers the same grid. */
  std::size_t OffsetOf(const itk::Index<3> &index, const mitk::Image *image)
  {
    const auto *dimensions = image->GetDimensions();
    const std::size_t sizeX = dimensions[0];
    const std::size_t sizeY = dimensions[1];
    return static_cast<std::size_t>(index[0]) +
           sizeX * (static_cast<std::size_t>(index[1]) + sizeY * static_cast<std::size_t>(index[2]));
  }

  /** Walks the pixels of one embedding and reports those that took part in the reduction.

      A pixel that stayed zero in every component never took part; drawing it would pile the whole
      background onto the origin. The callback is handed the data of the image, the pixel and the
      label the mask gives it, so a caller that only counts and one that reads both use the same
      walk. */
  template <typename Callback>
  void ForEachCandidate(const mitk::Image *image,
                        const mitk::Image *maskImage,
                        bool restrictToMask,
                        Callback &&callback)
  {
    const auto componentType = image->GetPixelType().GetComponentType();
    const auto numberOfComponents = static_cast<unsigned int>(image->GetPixelType().GetNumberOfComponents());
    const auto pixelCount = PixelCountOf(image);
    if (pixelCount == 0 || numberOfComponents == 0)
      return;

    mitk::ImageReadAccessor imageAccessor(image);
    const void *imageData = imageAccessor.GetData();

    std::unique_ptr<mitk::ImageReadAccessor> maskAccessor;
    const mitk::MultiLabelSegmentation::LabelValueType *maskData = nullptr;
    if (maskImage)
    {
      maskAccessor = std::make_unique<mitk::ImageReadAccessor>(maskImage);
      maskData = static_cast<const mitk::MultiLabelSegmentation::LabelValueType *>(maskAccessor->GetData());
    }

    for (std::size_t pixel = 0; pixel < pixelCount; ++pixel)
    {
      const auto label =
        maskData ? maskData[pixel] : mitk::MultiLabelSegmentation::UNLABELED_VALUE;

      if (restrictToMask && maskData && label == mitk::MultiLabelSegmentation::UNLABELED_VALUE)
        continue;

      bool contributes = false;
      for (unsigned int component = 0; component < numberOfComponents && !contributes; ++component)
        contributes = ReadComponent(imageData, componentType, pixel * numberOfComponents + component) != 0.0;

      if (contributes)
        callback(imageData, pixel, label);
    }
  }

  /** Decides which of a number of candidates are drawn when there are more of them than the plot
      should hold.

      Consecutive candidates are scattered over the whole range instead of following each other,
      so the sample covers the whole image rather than its first rows, which taking every n-th
      pixel of a raster would not. The decision only depends on the position of a candidate, so
      the same images always yield the same picture. Several images are counted in one run, which
      gives each of them a share of the budget in proportion to what it offers. */
  class Sampler
  {
  public:
    Sampler(std::size_t available, std::size_t wanted) : m_KeepAll(available <= wanted || wanted == 0)
    {
      if (!m_KeepAll)
        m_Threshold = static_cast<std::uint32_t>(
          (static_cast<double>(wanted) / static_cast<double>(available)) * 4294967295.0);
    }

    bool operator()(std::size_t position) const
    {
      return m_KeepAll || static_cast<std::uint32_t>(position * 2654435761u) <= m_Threshold;
    }

  private:
    bool m_KeepAll = true;
    std::uint32_t m_Threshold = 0;
  };

  /** A colour for the n-th category of a legend that names things rather than values. The eight
      hues are the ones of the Okabe-Ito set, which stay apart for the common colour vision
      deficiencies; beyond that they repeat. */
  void CategoryColor(std::size_t category, double &red, double &green, double &blue)
  {
    static const double palette[8][3] = {{0.000, 0.447, 0.698},
                                         {0.835, 0.369, 0.000},
                                         {0.000, 0.620, 0.451},
                                         {0.800, 0.475, 0.655},
                                         {0.902, 0.624, 0.000},
                                         {0.337, 0.706, 0.914},
                                         {0.941, 0.894, 0.259},
                                         {0.400, 0.400, 0.400}};

    const auto &color = palette[category % 8];
    red = color[0];
    green = color[1];
    blue = color[2];
  }

  /** The range the dimming is stretched over: the given quantiles of the values rather than their
      extremes.

      A similarity map or an ion image is rarely spread evenly. A handful of outlying pixels at
      either end is enough to push everything else into a narrow band of the scale - the values of
      a cosine similarity sit between 0.8 and 1.0 while one pixel holds 0.1 - and the cloud then
      comes out evenly bright, which is what made the highlighting hard to see. Cutting the tails
      off gives the bulk of the points the whole ramp, and the points beyond a quantile are simply
      clamped, so the brightest few per cent are drawn at full strength. */
  std::pair<double, double> RobustRange(const std::vector<float> &values, double lower, double upper)
  {
    if (values.empty())
      return {0.0, 1.0};

    std::vector<float> sample(values);
    const auto last = sample.size() - 1;
    const auto lowIndex = static_cast<std::size_t>(lower * static_cast<double>(last));
    const auto highIndex = static_cast<std::size_t>(upper * static_cast<double>(last));

    std::nth_element(sample.begin(), sample.begin() + lowIndex, sample.end());
    const auto low = static_cast<double>(sample[lowIndex]);

    std::nth_element(sample.begin() + lowIndex, sample.begin() + highIndex, sample.end());
    const auto high = static_cast<double>(sample[highIndex]);

    if (high > low)
      return {low, high};

    // the quantiles fall together where most of the image holds one value; the extremes are then
    // the only thing left to stretch over
    const auto extremes = std::minmax_element(values.begin(), values.end());
    return {static_cast<double>(*extremes.first), static_cast<double>(*extremes.second)};
  }

  QString FormatCount(std::size_t count)
  {
    return QLocale().toString(static_cast<qulonglong>(count));
  }
} // namespace

m2PlotView::m2PlotView() = default;

m2PlotView::~m2PlotView()
{
  ReleaseIntensityObservers();
  delete m_Controls;
}

void m2PlotView::CreateQtPartControl(QWidget *parent)
{
  m_Controls = new Ui::m2PlotViewControls;
  m_Controls->setupUi(parent);

  m_PointCloud = new m2PointCloudWidget(m_Controls->plotContainer);
  auto *plotLayout = new QVBoxLayout(m_Controls->plotContainer);
  plotLayout->setContentsMargins(0, 0, 0, 0);
  plotLayout->addWidget(m_PointCloud);

  m_Controls->splitter->setStretchFactor(0, 0);
  m_Controls->splitter->setStretchFactor(1, 1);

  // an embedding is any image with more than one component: what the Data Compression view
  // publishes, and equally a result read back from one of the docker methods. A segmentation is
  // excluded even when it has several groups; it is a mask, not a set of coordinates.
  auto isEmbedding = mitk::NodePredicateFunction::New(
    [](const mitk::DataNode *node) -> bool
    {
      if (dynamic_cast<mitk::MultiLabelSegmentation *>(node->GetData()))
        return false;
      auto image = dynamic_cast<mitk::Image *>(node->GetData());
      return image && image->GetPixelType().GetNumberOfComponents() > 1 &&
             IsReadableComponentType(image->GetPixelType().GetComponentType());
    });

  auto embeddingPredicate = mitk::NodePredicateAnd::New(isEmbedding, m2::DataNodePredicates::NoActiveHelper);
  m_Controls->embeddingSelection->SetDataStorage(this->GetDataStorage());
  m_Controls->embeddingSelection->SetNodePredicate(embeddingPredicate);
  m_Controls->embeddingSelection->SetSelectionIsOptional(true);
  m_Controls->embeddingSelection->SetEmptyInfo(QStringLiteral("Select one or more embedding images"));
  m_Controls->embeddingSelection->SetPopUpTitel(QStringLiteral("Embedding images"));
  m_Controls->embeddingSelection->SetPopUpHint(
    QStringLiteral("Selecting the embeddings of a combined reduction draws them in one cloud."));

  auto maskPredicate =
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::MultiLabelSegmentation>::New(),
                                m2::DataNodePredicates::NoActiveHelper);
  m_Controls->maskSelection->SetDataStorage(this->GetDataStorage());
  m_Controls->maskSelection->SetNodePredicate(maskPredicate);
  m_Controls->maskSelection->SetSelectionIsOptional(true);
  m_Controls->maskSelection->SetEmptyInfo(QStringLiteral("Select a mask per embedding"));
  m_Controls->maskSelection->SetPopUpTitel(QStringLiteral("Masks"));
  m_Controls->maskSelection->SetPopUpHint(
    QStringLiteral("Every embedding is paired with the mask that covers its grid."));

  // the RGB rendering a three component reduction is published as, and equally the three
  // components themselves; either way it is read as a colour per pixel rather than as coordinates
  auto isRgbImage = mitk::NodePredicateFunction::New(
    [](const mitk::DataNode *node) -> bool
    {
      if (dynamic_cast<mitk::MultiLabelSegmentation *>(node->GetData()))
        return false;
      auto image = dynamic_cast<mitk::Image *>(node->GetData());
      return image && image->GetPixelType().GetNumberOfComponents() >= 3 &&
             IsReadableComponentType(image->GetPixelType().GetComponentType());
    });

  auto rgbPredicate = mitk::NodePredicateAnd::New(isRgbImage, m2::DataNodePredicates::NoActiveHelper);
  m_Controls->rgbSelection->SetDataStorage(this->GetDataStorage());
  m_Controls->rgbSelection->SetNodePredicate(rgbPredicate);
  m_Controls->rgbSelection->SetSelectionIsOptional(true);
  m_Controls->rgbSelection->SetEmptyInfo(QStringLiteral("Select an RGB image per embedding"));
  m_Controls->rgbSelection->SetPopUpTitel(QStringLiteral("RGB images"));

  // an ion image, a similarity map of the Embedding Search view: one value per pixel, whatever
  // produced it
  auto isIntensityImage = mitk::NodePredicateFunction::New(
    [](const mitk::DataNode *node) -> bool
    {
      if (dynamic_cast<mitk::MultiLabelSegmentation *>(node->GetData()))
        return false;
      auto image = dynamic_cast<mitk::Image *>(node->GetData());
      return image && image->GetPixelType().GetNumberOfComponents() == 1 &&
             IsReadableComponentType(image->GetPixelType().GetComponentType());
    });

  auto intensityPredicate = mitk::NodePredicateAnd::New(isIntensityImage, m2::DataNodePredicates::NoActiveHelper);
  m_Controls->intensitySelection->SetDataStorage(this->GetDataStorage());
  m_Controls->intensitySelection->SetNodePredicate(intensityPredicate);
  m_Controls->intensitySelection->SetSelectionIsOptional(true);
  m_Controls->intensitySelection->SetEmptyInfo(QStringLiteral("Select an intensity image per embedding"));
  m_Controls->intensitySelection->SetPopUpTitel(QStringLiteral("Intensity images"));

  // only a centroid list that an analysis has described is of use here; the loadings are what
  // places its points, and without them there is nothing to plot
  auto isCentroidListWithLoadings = mitk::NodePredicateFunction::New(
    [](const mitk::DataNode *node) -> bool
    {
      auto intervals = dynamic_cast<m2::IntervalVector *>(node->GetData());
      return intervals && !intervals->GetFeatureNames().empty() &&
             (static_cast<unsigned int>(intervals->GetType()) &
              static_cast<unsigned int>(m2::SpectrumFormat::Centroid));
    });

  auto centroidPredicate =
    mitk::NodePredicateAnd::New(isCentroidListWithLoadings, m2::DataNodePredicates::NoActiveHelper);
  m_Controls->centroidSelection->SetDataStorage(this->GetDataStorage());
  m_Controls->centroidSelection->SetNodePredicate(centroidPredicate);
  m_Controls->centroidSelection->SetSelectionIsOptional(true);
  m_Controls->centroidSelection->SetEmptyInfo(QStringLiteral("Select a centroid list carrying loadings"));
  m_Controls->centroidSelection->SetPopUpTitel(QStringLiteral("Centroid list with loadings"));

  m_Controls->boxSource->addItem(QStringLiteral("Embedding (pixels)"));
  m_Controls->boxSource->addItem(QStringLiteral("Loadings (centroids)"));

  connect(m_Controls->embeddingSelection,
          &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged,
          this,
          &m2PlotView::OnInputChanged);
  connect(m_Controls->maskSelection,
          &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged,
          this,
          &m2PlotView::OnInputChanged);
  connect(m_Controls->rgbSelection,
          &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged,
          this,
          &m2PlotView::OnInputChanged);
  connect(m_Controls->intensitySelection,
          &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged,
          this,
          &m2PlotView::OnIntensitySelectionChanged);
  connect(m_Controls->centroidSelection,
          &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged,
          this,
          &m2PlotView::OnInputChanged);

  connect(m_Controls->boxSource, qOverload<int>(&QComboBox::currentIndexChanged), this, &m2PlotView::OnSourceChanged);
  connect(
    m_Controls->boxAnalysis, qOverload<int>(&QComboBox::currentIndexChanged), this, &m2PlotView::OnAnalysisChanged);
  connect(m_Controls->boxAxisX, qOverload<int>(&QComboBox::currentIndexChanged), this, &m2PlotView::OnAxisChanged);
  connect(m_Controls->boxAxisY, qOverload<int>(&QComboBox::currentIndexChanged), this, &m2PlotView::OnAxisChanged);
  connect(m_Controls->boxAxisZ, qOverload<int>(&QComboBox::currentIndexChanged), this, &m2PlotView::OnAxisChanged);
  connect(m_Controls->boxColor, qOverload<int>(&QComboBox::currentIndexChanged), this, &m2PlotView::OnColorChanged);

  connect(m_Controls->checkRestrictToMask, &QCheckBox::toggled, this, &m2PlotView::OnInputChanged);
  connect(m_Controls->spinMaxPoints, &QSpinBox::editingFinished, this, &m2PlotView::OnInputChanged);
  connect(m_Controls->spinPointSize, qOverload<int>(&QSpinBox::valueChanged), this, &m2PlotView::OnPointSizeChanged);
  connect(m_Controls->checkIntensityBrightness, &QCheckBox::toggled, this, &m2PlotView::OnColorChanged);
  connect(m_Controls->spinBrightnessFloor,
          qOverload<double>(&QDoubleSpinBox::valueChanged),
          this,
          &m2PlotView::OnColorChanged);
  connect(
    m_Controls->spinEmphasis, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &m2PlotView::OnColorChanged);
  connect(m_Controls->btnResetCamera, &QPushButton::clicked, this, &m2PlotView::OnResetCamera);

  connect(m_PointCloud, &m2PointCloudWidget::PointPicked, this, &m2PlotView::OnPointPicked);

  m_PointCloud->SetPointSize(m_Controls->spinPointSize->value());

  RebuildCloud();
}

void m2PlotView::SetFocus()
{
  if (m_PointCloud)
    m_PointCloud->setFocus();
}

m2PlotView::Source m2PlotView::GetSource() const
{
  return m_Controls->boxSource->currentIndex() == 1 ? Source::Loadings : Source::Embedding;
}

void m2PlotView::OnInputChanged()
{
  if (m_Updating)
    return;
  RebuildCloud();
}

void m2PlotView::OnSourceChanged()
{
  if (m_Updating)
    return;
  RebuildCloud();
}

void m2PlotView::OnAnalysisChanged()
{
  if (m_Updating)
    return;
  RebuildCloud();
}

void m2PlotView::OnAxisChanged()
{
  if (m_Updating)
    return;
  UpdatePlot();
}

void m2PlotView::OnColorChanged()
{
  if (m_Updating)
    return;
  UpdateColors();
}

void m2PlotView::OnPointSizeChanged(int size)
{
  m_PointCloud->SetPointSize(size);
}

void m2PlotView::OnResetCamera()
{
  m_PointCloud->ResetCamera();
}

void m2PlotView::OnIntensitySelectionChanged()
{
  if (m_Updating)
    return;

  ObserveIntensityNodes(m_Controls->intensitySelection->GetSelectedNodes());
  RebuildCloud();
}

void m2PlotView::ReleaseIntensityObservers()
{
  for (auto &watch : m_IntensityWatches)
  {
    if (watch.node.IsNotNull() && watch.nodeTag)
      watch.node->RemoveObserver(watch.nodeTag);

    if (watch.data.IsNotNull() && watch.dataTag)
      watch.data->RemoveObserver(watch.dataTag);
  }

  m_IntensityWatches.clear();
}

void m2PlotView::ObserveIntensityNodes(const QList<mitk::DataNode::Pointer> &nodes)
{
  ReleaseIntensityObservers();

  for (const auto &node : nodes)
  {
    if (node.IsNull())
      continue;

    auto command = itk::SimpleMemberCommand<m2PlotView>::New();
    command->SetCallbackFunction(this, &m2PlotView::OnIntensityModified);

    // the node is watched because a view that publishes a new result puts a new image on it, and
    // the image because a view that recomputes in place only modifies the image it already has
    WatchedNode watch;
    watch.node = node;
    watch.nodeTag = node->AddObserver(itk::ModifiedEvent(), command);
    watch.data = node->GetData();
    if (watch.data.IsNotNull())
      watch.dataTag = watch.data->AddObserver(itk::ModifiedEvent(), command);

    m_IntensityWatches.push_back(watch);
  }
}

void m2PlotView::OnIntensityModified()
{
  // a new image on a node means the observer has to follow it there; a node fires for every
  // property it carries as well, so this is also what tells an actual new result from a colour
  // or a visibility that changed
  for (auto &watch : m_IntensityWatches)
  {
    if (watch.node.IsNull())
      continue;

    auto *data = watch.node->GetData();
    if (data == watch.data.GetPointer())
      continue;

    if (watch.data.IsNotNull() && watch.dataTag)
      watch.data->RemoveObserver(watch.dataTag);

    auto command = itk::SimpleMemberCommand<m2PlotView>::New();
    command->SetCallbackFunction(this, &m2PlotView::OnIntensityModified);

    watch.data = data;
    watch.dataTag = watch.data.IsNotNull() ? watch.data->AddObserver(itk::ModifiedEvent(), command) : 0;
  }

  // nothing is drawn that could be recoloured
  if (m_Values.empty() || m_PixelIndices.empty() || GetSource() != Source::Embedding)
    return;

  // The images are not read here. A modification is announced while the operation that made it is
  // still on the stack, and the write accessor that filled the image may still be open; a read
  // accessor would then wait for a lock the same thread holds. Reading is left to the event
  // loop, which also collapses a burst of modifications into one pass over the points.
  if (m_IntensityRefreshPending)
    return;

  m_IntensityRefreshPending = true;
  QMetaObject::invokeMethod(this, [this] { RefreshIntensityColors(); }, Qt::QueuedConnection);
}

void m2PlotView::RefreshIntensityColors()
{
  m_IntensityRefreshPending = false;

  if (m_Values.empty() || m_PixelIndices.empty() || GetSource() != Source::Embedding)
    return;

  m_PointIntensities.clear();
  m_IntensityName.clear();

  // the images are taken from the nodes again rather than from what was read before, because a
  // view that publishes a new result has put a new image on the node in the meantime
  const bool complete =
    !m_Inputs.empty() && std::all_of(m_Inputs.begin(),
                                     m_Inputs.end(),
                                     [](const InputImage &input)
                                     {
                                       auto *image = ImageOf(input.intensityNode);
                                       return image && SameGrid(input.image, image);
                                     });

  if (complete)
  {
    m_PointIntensities.assign(m_PixelIndices.size(), 0.0f);
    for (const auto &input : m_Inputs)
      AppendPointIntensities(input, ImageOf(input.intensityNode));

    m_IntensityName = m_Inputs.size() == 1 ? QString::fromStdString(m_Inputs.front().intensityNode->GetName())
                                           : QStringLiteral("Intensity");
  }

  // the entry appears the first time a fitting image is there, and the choice is kept
  UpdateColorSelection();
  UpdateColors();
}

void m2PlotView::ClearCloud()
{
  m_DimensionNames.clear();
  m_Values.clear();
  m_Inputs.clear();
  m_PixelIndices.clear();
  m_PixelLabels.clear();
  m_PointInputs.clear();
  m_CentroidPositions.clear();
  m_CentroidDescriptions.clear();
  m_PointColors.clear();
  m_PointIntensities.clear();
  m_IntensityName.clear();
  m_LabelCategories.clear();
  m_LabelCategoryNames.clear();
  m_LabelTable = nullptr;
  m_InputCategories.clear();
  m_InputTable = nullptr;
  m_CandidateCount = 0;
  m_Controls->labelInfo->setText(QStringLiteral("Click a point to describe it."));
}

void m2PlotView::RebuildCloud()
{
  ClearCloud();

  const bool embedding = GetSource() == Source::Embedding;
  m_Controls->maskSelection->setEnabled(embedding);
  m_Controls->rgbSelection->setEnabled(embedding);
  m_Controls->intensitySelection->setEnabled(embedding);
  m_Controls->checkRestrictToMask->setEnabled(embedding);
  m_Controls->boxAnalysis->setEnabled(!embedding);

  if (embedding)
    ReadEmbedding();
  else
    ReadLoadings();

  UpdateAxisSelection();
  UpdateColorSelection();
  UpdatePlot();
  UpdateStatus();
}

bool m2PlotView::SameGrid(const mitk::Image *left, const mitk::Image *right)
{
  if (!left || !right)
    return false;

  const auto *leftDimensions = left->GetDimensions();
  const auto *rightDimensions = right->GetDimensions();

  return std::equal(leftDimensions, leftDimensions + 3, rightDimensions);
}

mitk::Image *m2PlotView::ImageOf(const mitk::DataNode *node)
{
  return node ? dynamic_cast<mitk::Image *>(node->GetData()) : nullptr;
}

mitk::Image *m2PlotView::GridImageOf(const mitk::DataNode *node)
{
  if (!node)
    return nullptr;

  // group 0 is the mask everywhere else in m2aia: the valid spectra of an image, the clusters of
  // a clustering
  if (auto segmentation = dynamic_cast<mitk::MultiLabelSegmentation *>(node->GetData()))
    return segmentation->GetNumberOfGroups() > 0 ? segmentation->GetGroupImage(0) : nullptr;

  return dynamic_cast<mitk::Image *>(node->GetData());
}

mitk::MultiLabelSegmentation *m2PlotView::MaskOf(const InputImage &input)
{
  if (input.maskNode.IsNull())
    return nullptr;

  auto segmentation = dynamic_cast<mitk::MultiLabelSegmentation *>(input.maskNode->GetData());
  return segmentation && segmentation->GetNumberOfGroups() > 0 ? segmentation : nullptr;
}

mitk::DataNode *m2PlotView::RootSource(const mitk::DataNode *node) const
{
  auto storage = this->GetDataStorage();
  if (!storage || !node)
    return const_cast<mitk::DataNode *>(node);

  auto current = const_cast<mitk::DataNode *>(node);

  // a cycle in the storage would otherwise turn this into an endless walk; a result is at most a
  // few steps away from the image it was computed from
  for (int step = 0; step < 16; ++step)
  {
    auto sources = storage->GetSources(current, nullptr, true);
    if (sources.IsNull() || sources->empty())
      break;

    current = sources->front();
  }

  return current;
}

mitk::DataNode *m2PlotView::MatchCompanion(const QList<mitk::DataNode::Pointer> &candidates,
                                           const mitk::DataNode *embeddingNode,
                                           const mitk::Image *embedding,
                                           const char *what) const
{
  if (candidates.isEmpty())
    return nullptr;

  // reading the pixel of another grid would take whatever happens to lie at the same offset, so a
  // differing grid is refused rather than silently used
  QList<mitk::DataNode::Pointer> fitting;
  for (const auto &candidate : candidates)
    if (candidate.IsNotNull() && SameGrid(embedding, GridImageOf(candidate)))
      fitting.append(candidate);

  if (fitting.isEmpty())
  {
    MITK_WARN << "None of the selected " << what << "s covers the grid of [" << embeddingNode->GetName()
              << "]; its points are not described by one.";
    return nullptr;
  }

  // several images of one study often share a grid, so the grid alone does not say which mask
  // belongs to which embedding; the image both were computed from does
  auto root = RootSource(embeddingNode);
  for (const auto &candidate : fitting)
    if (RootSource(candidate) == root)
      return candidate;

  if (fitting.size() > 1)
    MITK_WARN << "Several of the selected " << what << "s cover the grid of [" << embeddingNode->GetName()
              << "] and none of them was computed from the same image; [" << fitting.front()->GetName()
              << "] is used.";

  return fitting.front();
}

std::vector<m2PlotView::InputImage> m2PlotView::CollectInputs() const
{
  std::vector<InputImage> inputs;

  const auto masks = m_Controls->maskSelection->GetSelectedNodes();
  const auto rgbImages = m_Controls->rgbSelection->GetSelectedNodes();
  const auto intensityImages = m_Controls->intensitySelection->GetSelectedNodes();

  for (const auto &node : m_Controls->embeddingSelection->GetSelectedNodes())
  {
    if (node.IsNull())
      continue;

    auto image = dynamic_cast<mitk::Image *>(node->GetData());
    if (!image)
      continue;

    const auto numberOfComponents = static_cast<unsigned int>(image->GetPixelType().GetNumberOfComponents());
    if (numberOfComponents < 2 || !IsReadableComponentType(image->GetPixelType().GetComponentType()) ||
        PixelCountOf(image) == 0)
    {
      MITK_WARN << "The image [" << node->GetName() << "] does not hold an embedding this view can read.";
      continue;
    }

    InputImage input;
    input.node = node;
    input.image = image;
    input.maskNode = MatchCompanion(masks, node, image, "mask");
    input.rgbNode = MatchCompanion(rgbImages, node, image, "RGB image");
    input.intensityNode = MatchCompanion(intensityImages, node, image, "intensity image");

    inputs.push_back(input);
  }

  return inputs;
}

void m2PlotView::ReadEmbedding()
{
  m_Inputs = CollectInputs();
  if (m_Inputs.empty())
    return;

  // the embeddings of a combined reduction share their component space, which is what lets their
  // points be drawn on the same axes. Images that were reduced on their own may differ in how
  // many components they carry; the ones they have in common are then what can be compared.
  auto components = std::numeric_limits<unsigned int>::max();
  bool uneven = false;
  for (const auto &input : m_Inputs)
  {
    const auto count = static_cast<unsigned int>(input.image->GetPixelType().GetNumberOfComponents());
    uneven = uneven || (components != std::numeric_limits<unsigned int>::max() && count != components);
    components = std::min(components, count);
  }

  if (uneven)
    MITK_WARN << "The selected embeddings do not carry the same number of components; the first " << components
              << " they have in common are plotted.";

  const bool restrictToMask = m_Controls->checkRestrictToMask->isChecked();

  // what every image offers is counted first, so that the budget of points is shared out over all
  // of them rather than spent on whichever comes first
  for (auto &input : m_Inputs)
  {
    auto *mask = MaskOf(input);
    ForEachCandidate(input.image,
                     mask ? mask->GetGroupImage(0) : nullptr,
                     restrictToMask,
                     [&input](const void *, std::size_t, mitk::MultiLabelSegmentation::LabelValueType)
                     { ++input.candidates; });

    m_CandidateCount += input.candidates;
  }

  const Sampler keep(m_CandidateCount, static_cast<std::size_t>(m_Controls->spinMaxPoints->value()));

  for (unsigned int component = 0; component < components; ++component)
    m_DimensionNames << QStringLiteral("Component %1").arg(component + 1);

  m_Values.assign(components, {});

  std::size_t position = 0;
  for (std::size_t inputIndex = 0; inputIndex < m_Inputs.size(); ++inputIndex)
  {
    auto &input = m_Inputs[inputIndex];
    input.pointBegin = m_PixelIndices.size();

    const auto componentType = input.image->GetPixelType().GetComponentType();
    const auto storedComponents = static_cast<unsigned int>(input.image->GetPixelType().GetNumberOfComponents());
    const auto *dimensions = input.image->GetDimensions();
    const std::size_t sizeX = dimensions[0];
    const std::size_t sizeY = dimensions[1];

    auto *mask = MaskOf(input);
    ForEachCandidate(
      input.image,
      mask ? mask->GetGroupImage(0) : nullptr,
      restrictToMask,
      [&](const void *imageData, std::size_t pixel, mitk::MultiLabelSegmentation::LabelValueType label)
      {
        if (!keep(position++))
          return;

        for (unsigned int component = 0; component < components; ++component)
          m_Values[component].push_back(
            static_cast<float>(ReadComponent(imageData, componentType, pixel * storedComponents + component)));

        itk::Index<3> index;
        index[0] = static_cast<itk::IndexValueType>(pixel % sizeX);
        index[1] = static_cast<itk::IndexValueType>((pixel / sizeX) % sizeY);
        index[2] = static_cast<itk::IndexValueType>(pixel / (sizeX * sizeY));
        m_PixelIndices.push_back(index);

        m_PixelLabels.push_back(label);
        m_PointInputs.push_back(static_cast<int>(inputIndex));
      });

    input.pointEnd = m_PixelIndices.size();
  }

  if (m_PixelIndices.empty())
    return;

  BuildImageTable();

  if (std::any_of(m_Inputs.begin(), m_Inputs.end(), [](const InputImage &input) { return MaskOf(input) != nullptr; }))
    BuildLabelTable();

  // a colouring that only half the cloud has an answer for would say more about which images were
  // described than about the points, so both of these are all or nothing
  const auto hasCompanion = [](const InputImage &input, const mitk::DataNode *node)
  {
    auto *image = ImageOf(node);
    return image && SameGrid(input.image, image);
  };

  if (std::all_of(m_Inputs.begin(),
                  m_Inputs.end(),
                  [&](const InputImage &input) { return hasCompanion(input, input.rgbNode); }))
  {
    std::vector<double> channels(m_PixelIndices.size() * 3, 0.0);
    bool allBytes = true;
    for (const auto &input : m_Inputs)
      allBytes = AppendPointColorChannels(input, ImageOf(input.rgbNode), channels) && allBytes;

    FinishPointColors(channels, allBytes);
  }
  else if (std::any_of(m_Inputs.begin(),
                       m_Inputs.end(),
                       [&](const InputImage &input) { return hasCompanion(input, input.rgbNode); }))
  {
    MITK_WARN << "Only some of the embeddings have an RGB image on their grid; the cloud is not coloured by one.";
  }

  if (std::all_of(m_Inputs.begin(),
                  m_Inputs.end(),
                  [&](const InputImage &input) { return hasCompanion(input, input.intensityNode); }))
  {
    m_PointIntensities.assign(m_PixelIndices.size(), 0.0f);
    for (const auto &input : m_Inputs)
      AppendPointIntensities(input, ImageOf(input.intensityNode));

    m_IntensityName = m_Inputs.size() == 1 ? QString::fromStdString(m_Inputs.front().intensityNode->GetName())
                                           : QStringLiteral("Intensity");
  }
  else if (std::any_of(m_Inputs.begin(),
                       m_Inputs.end(),
                       [&](const InputImage &input) { return hasCompanion(input, input.intensityNode); }))
  {
    MITK_WARN << "Only some of the embeddings have an intensity image on their grid; the cloud is not dimmed by one.";
  }
}

void m2PlotView::AppendPointIntensities(const InputImage &input, const mitk::Image *intensityImage)
{
  if (!intensityImage || m_PointIntensities.size() != m_PixelIndices.size())
    return;

  const auto componentType = intensityImage->GetPixelType().GetComponentType();
  const auto numberOfComponents = static_cast<unsigned int>(intensityImage->GetPixelType().GetNumberOfComponents());
  if (numberOfComponents == 0 || !IsReadableComponentType(componentType))
    return;

  mitk::ImageReadAccessor accessor(intensityImage);
  const void *data = accessor.GetData();

  for (auto point = input.pointBegin; point < input.pointEnd; ++point)
  {
    const auto pixel = OffsetOf(m_PixelIndices[point], intensityImage);
    m_PointIntensities[point] =
      static_cast<float>(ReadComponent(data, componentType, pixel * numberOfComponents));
  }
}

bool m2PlotView::AppendPointColorChannels(const InputImage &input,
                                          const mitk::Image *rgbImage,
                                          std::vector<double> &channels) const
{
  if (!rgbImage || channels.size() != m_PixelIndices.size() * 3)
    return true;

  const auto componentType = rgbImage->GetPixelType().GetComponentType();
  const auto numberOfComponents = static_cast<unsigned int>(rgbImage->GetPixelType().GetNumberOfComponents());
  if (numberOfComponents < 3 || !IsReadableComponentType(componentType))
    return true;

  mitk::ImageReadAccessor accessor(rgbImage);
  const void *data = accessor.GetData();

  // the pixels the points came from are already known, so only they are read
  for (auto point = input.pointBegin; point < input.pointEnd; ++point)
  {
    const auto pixel = OffsetOf(m_PixelIndices[point], rgbImage);
    for (unsigned int channel = 0; channel < 3; ++channel)
      channels[point * 3 + channel] = ReadComponent(data, componentType, pixel * numberOfComponents + channel);
  }

  return componentType == itk::IOComponentEnum::UCHAR;
}

void m2PlotView::FinishPointColors(const std::vector<double> &channels, bool allBytes)
{
  const auto pointCount = m_PixelIndices.size();
  if (channels.size() != pointCount * 3 || pointCount == 0)
    return;

  // an image of bytes already holds the colour it is meant to show; anything else - the three
  // components a UMAP produces before they are rendered as RGB - is stretched onto that range
  // per channel. The stretch runs over all inputs together, so the same value is the same colour
  // in every image of a combined embedding.
  double scale[3] = {1.0, 1.0, 1.0};
  double offset[3] = {0.0, 0.0, 0.0};
  if (!allBytes)
  {
    for (unsigned int channel = 0; channel < 3; ++channel)
    {
      auto minimum = std::numeric_limits<double>::max();
      auto maximum = std::numeric_limits<double>::lowest();
      for (std::size_t point = 0; point < pointCount; ++point)
      {
        const auto value = channels[point * 3 + channel];
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
      }

      // a channel that is flat carries nothing to see; it is drawn as a middle grey rather than
      // divided by zero
      if (maximum > minimum)
      {
        scale[channel] = 255.0 / (maximum - minimum);
        offset[channel] = -minimum * scale[channel];
      }
      else
      {
        scale[channel] = 0.0;
        offset[channel] = 127.0;
      }
    }
  }

  m_PointColors.resize(pointCount * 3);
  for (std::size_t point = 0; point < pointCount; ++point)
    for (unsigned int channel = 0; channel < 3; ++channel)
    {
      const auto value = channels[point * 3 + channel] * scale[channel] + offset[channel];
      m_PointColors[point * 3 + channel] =
        static_cast<unsigned char>(std::lround(std::min(255.0, std::max(0.0, value))));
    }
}

void m2PlotView::BuildImageTable()
{
  if (m_Inputs.size() < 2 || m_PointInputs.size() != m_PixelIndices.size())
    return;

  auto table = vtkSmartPointer<vtkLookupTable>::New();
  table->SetNumberOfTableValues(static_cast<vtkIdType>(m_Inputs.size()));
  table->SetTableRange(0.0, static_cast<double>(m_Inputs.size() - 1));
  table->IndexedLookupOn();

  for (std::size_t i = 0; i < m_Inputs.size(); ++i)
  {
    double red = 0.0;
    double green = 0.0;
    double blue = 0.0;
    CategoryColor(i, red, green, blue);

    table->SetTableValue(static_cast<vtkIdType>(i), red, green, blue, 1.0);
    table->SetAnnotation(vtkVariant(static_cast<double>(i)), m_Inputs[i].node->GetName().c_str());
  }

  m_InputCategories.reserve(m_PointInputs.size());
  for (const auto input : m_PointInputs)
    m_InputCategories.push_back(static_cast<float>(input));

  m_InputTable = table;
}

void m2PlotView::BuildLabelTable()
{
  // the label values of a segmentation are unique but need not be consecutive, and two masks name
  // their labels independently of each other, so the points carry a dense category index instead
  // and the table names the categories. Labels that share a name share a category: the point of
  // drawing several images in one cloud is to see whether the same class lands in the same place.
  std::map<QString, int> categories;
  std::vector<double> red;
  std::vector<double> green;
  std::vector<double> blue;

  // one lookup per label rather than one per point; a mask of a million pixels names a handful of
  // classes
  std::vector<std::map<mitk::MultiLabelSegmentation::LabelValueType, int>> resolved(m_Inputs.size());

  m_LabelCategories.resize(m_PixelLabels.size());

  for (std::size_t point = 0; point < m_PixelLabels.size(); ++point)
  {
    const auto inputIndex = static_cast<std::size_t>(m_PointInputs[point]);
    const auto label = m_PixelLabels[point];

    auto &cache = resolved[inputIndex];
    const auto known = cache.find(label);
    if (known != cache.end())
    {
      m_LabelCategories[point] = static_cast<float>(known->second);
      continue;
    }

    QString name;
    double labelRed = 0.55;
    double labelGreen = 0.55;
    double labelBlue = 0.58;

    auto *segmentation = MaskOf(m_Inputs[inputIndex]);
    if (label == mitk::MultiLabelSegmentation::UNLABELED_VALUE)
    {
      name = QStringLiteral("Unlabelled");
    }
    else if (auto mitkLabel = segmentation ? segmentation->GetLabel(label) : nullptr)
    {
      name = QString::fromStdString(mitkLabel->GetName());
      const auto &color = mitkLabel->GetColor();
      labelRed = color.GetRed();
      labelGreen = color.GetGreen();
      labelBlue = color.GetBlue();
    }

    if (name.isEmpty())
      name = QStringLiteral("Label %1").arg(label);

    auto category = categories.find(name);
    if (category == categories.end())
    {
      category = categories.emplace(name, static_cast<int>(m_LabelCategoryNames.size())).first;
      m_LabelCategoryNames << name;
      red.push_back(labelRed);
      green.push_back(labelGreen);
      blue.push_back(labelBlue);
    }

    cache[label] = category->second;
    m_LabelCategories[point] = static_cast<float>(category->second);
  }

  if (m_LabelCategoryNames.isEmpty())
  {
    m_LabelCategories.clear();
    return;
  }

  auto table = vtkSmartPointer<vtkLookupTable>::New();
  table->SetNumberOfTableValues(static_cast<vtkIdType>(m_LabelCategoryNames.size()));
  table->SetTableRange(0.0, static_cast<double>(m_LabelCategoryNames.size() - 1));
  table->IndexedLookupOn();

  for (int i = 0; i < m_LabelCategoryNames.size(); ++i)
  {
    table->SetTableValue(static_cast<vtkIdType>(i), red[i], green[i], blue[i], 1.0);
    table->SetAnnotation(vtkVariant(static_cast<double>(i)), m_LabelCategoryNames[i].toUtf8().constData());
  }

  m_LabelTable = table;
}

QString m2PlotView::AnalysisOfFeature(const QString &featureName)
{
  // an analysis names its features "<image>.<method>.c<component>"; everything before the
  // component is the analysis. A name of another shape stands for itself, so a value attached
  // from elsewhere is still plottable.
  const auto separator = featureName.lastIndexOf(QLatin1Char('.'));
  if (separator <= 0)
    return featureName;

  const auto component = featureName.mid(separator + 1);
  if (component.size() < 2 || component[0] != QLatin1Char('c'))
    return featureName;

  bool isNumber = false;
  component.mid(1).toInt(&isNumber);

  return isNumber ? featureName.left(separator) : featureName;
}

QStringList m2PlotView::GetAnalyses() const
{
  QStringList analyses;

  auto node = m_Controls->centroidSelection->GetSelectedNode();
  if (node.IsNull())
    return analyses;

  auto intervals = dynamic_cast<m2::IntervalVector *>(node->GetData());
  if (!intervals)
    return analyses;

  for (const auto &name : intervals->GetFeatureNames())
  {
    const auto analysis = AnalysisOfFeature(QString::fromStdString(name));
    if (!analyses.contains(analysis))
      analyses << analysis;
  }

  return analyses;
}

QStringList m2PlotView::GetAnalysisFeatures(const QString &analysis) const
{
  QStringList features;

  auto node = m_Controls->centroidSelection->GetSelectedNode();
  if (node.IsNull())
    return features;

  auto intervals = dynamic_cast<m2::IntervalVector *>(node->GetData());
  if (!intervals)
    return features;

  // the order the features were attached in is the order of the components
  for (const auto &name : intervals->GetFeatureNames())
  {
    const auto feature = QString::fromStdString(name);
    if (AnalysisOfFeature(feature) == analysis)
      features << feature;
  }

  return features;
}

void m2PlotView::UpdateAnalysisSelection()
{
  const auto analyses = GetAnalyses();
  const auto previous = m_Controls->boxAnalysis->currentText();

  m_Updating = true;
  m_Controls->boxAnalysis->clear();
  m_Controls->boxAnalysis->addItems(analyses);

  const auto index = analyses.indexOf(previous);
  m_Controls->boxAnalysis->setCurrentIndex(index < 0 ? 0 : index);
  m_Updating = false;
}

void m2PlotView::ReadLoadings()
{
  UpdateAnalysisSelection();

  auto node = m_Controls->centroidSelection->GetSelectedNode();
  if (node.IsNull())
    return;

  auto intervals = dynamic_cast<m2::IntervalVector *>(node->GetData());
  if (!intervals)
    return;

  const auto analysis = m_Controls->boxAnalysis->currentText();
  const auto features = GetAnalysisFeatures(analysis);
  if (features.isEmpty())
    return;

  std::vector<std::string> keys;
  for (const auto &feature : features)
  {
    keys.push_back(feature.toStdString());

    // the component is what the axis stands for; the analysis is already named by its own box
    const auto suffix = feature.mid(analysis.size() + 1);
    bool isNumber = false;
    const auto component = suffix.size() > 1 ? suffix.mid(1).toInt(&isNumber) : 0;
    m_DimensionNames << (isNumber ? QStringLiteral("Component %1").arg(component) : feature);
  }

  m_Values.assign(keys.size(), {});

  const auto &data = intervals->GetIntervals();
  m_CandidateCount = data.size();

  for (const auto &interval : data)
  {
    // a centroid this analysis never described has no place in its plot; it would otherwise be
    // drawn at the origin as a loading of zero it never produced
    const bool complete = std::all_of(
      keys.begin(), keys.end(), [&interval](const std::string &key) { return interval.HasFeature(key); });
    if (!complete)
      continue;

    for (std::size_t dimension = 0; dimension < keys.size(); ++dimension)
      m_Values[dimension].push_back(static_cast<float>(interval.GetFeature(keys[dimension])));

    m_CentroidPositions.push_back(interval.x.mean());
    m_CentroidDescriptions.push_back(QString::fromStdString(interval.description));
  }
}

void m2PlotView::UpdateAxisSelection()
{
  const auto restore = [this](QComboBox *box, int fallback, bool allowNone)
  {
    const auto previous = box->currentIndex() >= 0 ? box->currentData().toInt() : -2;

    box->clear();
    if (allowNone)
      box->addItem(QStringLiteral("— (plot in the plane)"), -1);

    for (int dimension = 0; dimension < m_DimensionNames.size(); ++dimension)
      box->addItem(m_DimensionNames[dimension], dimension);

    int index = box->findData(previous);
    if (index < 0)
      index = box->findData(fallback);
    if (index < 0)
      index = box->count() > 0 ? 0 : -1;

    box->setCurrentIndex(index);
  };

  const auto dimensions = m_DimensionNames.size();

  m_Updating = true;
  restore(m_Controls->boxAxisX, 0, false);
  restore(m_Controls->boxAxisY, dimensions > 1 ? 1 : 0, false);
  // a reduction of three or more components is shown in space right away; that is what the third
  // one is for
  restore(m_Controls->boxAxisZ, dimensions > 2 ? 2 : -1, true);
  m_Updating = false;
}

void m2PlotView::UpdateColorSelection()
{
  const auto previous = m_Controls->boxColor->currentIndex() >= 0
                          ? m_Controls->boxColor->currentData().toString()
                          : QString();

  m_Updating = true;
  m_Controls->boxColor->clear();

  QString fallback = QString::fromLatin1(COLOR_UNIFORM);

  if (GetSource() == Source::Embedding)
  {
    // which image a point came from is what a combined embedding is looked at for, so it leads
    // the list and is what the cloud is drawn as until something else is asked for
    if (m_InputTable)
    {
      m_Controls->boxColor->addItem(QStringLiteral("Image"), QString::fromLatin1(COLOR_INPUT));
      fallback = QString::fromLatin1(COLOR_INPUT);
    }

    if (!m_PointColors.empty())
    {
      m_Controls->boxColor->addItem(QStringLiteral("RGB image"), QString::fromLatin1(COLOR_RGB));
      if (fallback == QLatin1String(COLOR_UNIFORM))
        fallback = QString::fromLatin1(COLOR_RGB);
    }

    if (m_LabelTable)
    {
      m_Controls->boxColor->addItem(QStringLiteral("Mask label"), QString::fromLatin1(COLOR_MASK));
      if (fallback == QLatin1String(COLOR_UNIFORM))
        fallback = QString::fromLatin1(COLOR_MASK);
    }

    if (!m_PixelIndices.empty())
      m_Controls->boxColor->addItem(QStringLiteral("Pixel position (RGB)"), QString::fromLatin1(COLOR_PIXEL_RGB));
  }
  else if (!m_CentroidPositions.empty())
  {
    m_Controls->boxColor->addItem(QStringLiteral("m/z"), QString::fromLatin1(COLOR_POSITION));
    fallback = QString::fromLatin1(COLOR_POSITION);
  }

  for (int dimension = 0; dimension < m_DimensionNames.size(); ++dimension)
    m_Controls->boxColor->addItem(m_DimensionNames[dimension],
                                  QString::fromLatin1(COLOR_DIMENSION) + QString::number(dimension));

  m_Controls->boxColor->addItem(QStringLiteral("Nothing"), QString::fromLatin1(COLOR_UNIFORM));

  int index = previous.isEmpty() ? -1 : m_Controls->boxColor->findData(previous);
  if (index < 0)
    index = m_Controls->boxColor->findData(fallback);
  if (index < 0)
    index = 0;

  m_Controls->boxColor->setCurrentIndex(index);
  m_Updating = false;
}

int m2PlotView::GetAxisDimension(int axis) const
{
  const QComboBox *box = axis == 0   ? m_Controls->boxAxisX
                         : axis == 1 ? m_Controls->boxAxisY
                                     : m_Controls->boxAxisZ;

  if (box->currentIndex() < 0)
    return -1;

  const auto dimension = box->currentData().toInt();
  return dimension >= 0 && dimension < static_cast<int>(m_Values.size()) ? dimension : -1;
}

void m2PlotView::UpdatePlot()
{
  const auto x = GetAxisDimension(0);
  const auto y = GetAxisDimension(1);
  const auto z = GetAxisDimension(2);

  if (m_Values.empty() || x < 0 || y < 0)
  {
    m_PointCloud->Clear();
    return;
  }

  static const std::vector<float> none;

  m_PointCloud->SetProjection2D(z < 0);
  m_PointCloud->SetPoints(m_Values[x], m_Values[y], z < 0 ? none : m_Values[z]);
  m_PointCloud->SetAxisTitles(m_DimensionNames[x], m_DimensionNames[y], z < 0 ? QString() : m_DimensionNames[z]);

  UpdateColors();
}

vtkSmartPointer<vtkScalarsToColors> m2PlotView::MakeContinuousTable(double minimum, double maximum)
{
  if (!(maximum > minimum))
  {
    maximum = minimum + 1.0;
  }

  // an ordered ramp from dark blue through green to yellow; it keeps its order when read as
  // brightness alone, which a rainbow does not
  auto table = vtkSmartPointer<vtkColorTransferFunction>::New();
  table->SetColorSpaceToLab();
  table->AddRGBPoint(minimum, 0.267, 0.005, 0.329);
  table->AddRGBPoint(minimum + 0.25 * (maximum - minimum), 0.229, 0.322, 0.545);
  table->AddRGBPoint(minimum + 0.50 * (maximum - minimum), 0.128, 0.567, 0.551);
  table->AddRGBPoint(minimum + 0.75 * (maximum - minimum), 0.370, 0.789, 0.383);
  table->AddRGBPoint(maximum, 0.993, 0.906, 0.144);

  return table;
}

bool m2PlotView::IsBrightnessModulated() const
{
  return GetSource() == Source::Embedding && m_Controls->checkIntensityBrightness->isChecked() &&
         m_PointIntensities.size() == (m_Values.empty() ? 0 : m_Values.front().size()) &&
         !m_PointIntensities.empty();
}

std::vector<unsigned char> m2PlotView::ComputePixelPositionColors() const
{
  std::vector<unsigned char> rgb(m_PixelIndices.size() * 3, 0);
  if (m_PixelIndices.empty())
    return rgb;

  // every axis is stretched over the points that are drawn rather than over the whole grid, so a
  // cloud restricted to a mask still uses the full range of colours. Several images are stretched
  // over their common extent, which is what puts the same place of two images of the same size
  // into the same colour.
  itk::IndexValueType minimum[3];
  itk::IndexValueType maximum[3];
  for (int axis = 0; axis < 3; ++axis)
  {
    minimum[axis] = std::numeric_limits<itk::IndexValueType>::max();
    maximum[axis] = std::numeric_limits<itk::IndexValueType>::lowest();
  }

  for (const auto &index : m_PixelIndices)
    for (int axis = 0; axis < 3; ++axis)
    {
      minimum[axis] = std::min(minimum[axis], index[axis]);
      maximum[axis] = std::max(maximum[axis], index[axis]);
    }

  for (std::size_t point = 0; point < m_PixelIndices.size(); ++point)
    for (int axis = 0; axis < 3; ++axis)
    {
      // an axis of no extent - the third one of a slice - carries nothing and stays dark rather
      // than tinting every point of a two dimensional image the same way
      if (maximum[axis] <= minimum[axis])
        continue;

      const auto position = static_cast<double>(m_PixelIndices[point][axis] - minimum[axis]) /
                            static_cast<double>(maximum[axis] - minimum[axis]);
      rgb[point * 3 + axis] = static_cast<unsigned char>(std::lround(255.0 * position));
    }

  return rgb;
}

bool m2PlotView::ComputeBaseColors(std::vector<unsigned char> &rgb,
                                   vtkSmartPointer<vtkScalarsToColors> &legend,
                                   QString &legendTitle) const
{
  rgb.clear();
  legend = nullptr;
  legendTitle.clear();

  const auto pointCount = m_Values.empty() ? std::size_t{0} : m_Values.front().size();
  if (pointCount == 0 || m_Controls->boxColor->currentIndex() < 0)
    return false;

  const auto mode = m_Controls->boxColor->currentData().toString();

  // the colourings that already are colours
  if (mode == QLatin1String(COLOR_RGB) && m_PointColors.size() == pointCount * 3)
  {
    rgb = m_PointColors;
    return true;
  }

  if (mode == QLatin1String(COLOR_PIXEL_RGB) && m_PixelIndices.size() == pointCount)
  {
    rgb = ComputePixelPositionColors();
    return true;
  }

  // the colourings that are a value and a table; the table is asked for the colour of every
  // point here instead of leaving that to the renderer, so that the result can be dimmed
  std::vector<float> values;

  if (mode == QLatin1String(COLOR_INPUT) && m_InputTable && m_InputCategories.size() == pointCount)
  {
    values = m_InputCategories;
    legend = m_InputTable;
    legendTitle = QStringLiteral("Image");
  }
  else if (mode == QLatin1String(COLOR_MASK) && m_LabelTable && m_LabelCategories.size() == pointCount)
  {
    values = m_LabelCategories;
    legend = m_LabelTable;
    legendTitle = QStringLiteral("Label");
  }
  else if (mode == QLatin1String(COLOR_POSITION) && m_CentroidPositions.size() == pointCount)
  {
    values.reserve(pointCount);
    for (const auto position : m_CentroidPositions)
      values.push_back(static_cast<float>(position));
    legendTitle = QStringLiteral("m/z");
  }
  else if (mode.startsWith(QLatin1String(COLOR_DIMENSION)))
  {
    const auto dimension = mode.mid(static_cast<int>(std::strlen(COLOR_DIMENSION))).toInt();
    if (dimension < 0 || dimension >= static_cast<int>(m_Values.size()))
      return false;

    values = m_Values[dimension];
    legendTitle = m_DimensionNames[dimension];
  }
  else
  {
    // no colouring, or one that is a single colour; the caller paints it uniformly
    return false;
  }

  if (values.size() != pointCount)
    return false;

  if (legend == nullptr)
  {
    const auto range = std::minmax_element(values.begin(), values.end());
    legend = MakeContinuousTable(*range.first, *range.second);
  }

  rgb.resize(pointCount * 3);
  for (std::size_t point = 0; point < pointCount; ++point)
  {
    double color[3] = {0.0, 0.0, 0.0};
    legend->GetColor(values[point], color);
    for (int channel = 0; channel < 3; ++channel)
      rgb[point * 3 + channel] = static_cast<unsigned char>(std::lround(255.0 * color[channel]));
  }

  return true;
}

void m2PlotView::ApplyIntensityBrightness(std::vector<unsigned char> &rgb) const
{
  const auto pointCount = m_PointIntensities.size();
  if (rgb.size() != pointCount * 3 || pointCount == 0)
    return;

  // the range runs over all inputs together: a combined embedding is dimmed on one common scale,
  // so what the maps of two images say stays comparable
  const auto range = RobustRange(m_PointIntensities, 0.02, 0.98);
  const auto minimum = range.first;
  const auto maximum = range.second;

  const auto floor = std::min(1.0, std::max(0.0, m_Controls->spinBrightnessFloor->value()));
  const auto emphasis = std::max(0.1, m_Controls->spinEmphasis->value());

  // A point that is dimmed loses its chroma along with its light. Brightness alone is a weak
  // signal in a cloud that is already made of colours: a half dimmed red and a full red read as
  // the same region seen through different lighting, whereas a grey and a red read as two
  // different things. Draining the colour as well leaves the points the image answers for as the
  // only saturated ones in the plot. A third of the chroma is kept, which is still enough to tell
  // the regions of the cloud apart where nothing is highlighted.
  constexpr double CHROMA_FLOOR = 0.35;

  for (std::size_t point = 0; point < pointCount; ++point)
  {
    // an image that is flat carries no contrast; every point then keeps its colour rather than
    // being dimmed by an arbitrary amount
    const auto normalized =
      maximum > minimum
        ? std::min(1.0,
                   std::max(0.0, (static_cast<double>(m_PointIntensities[point]) - minimum) / (maximum - minimum)))
        : 1.0;

    // the ramp is bent rather than straight: with an exponent above one a point has to be near
    // the top of the range to stay bright, so a query is answered by a few points standing out of
    // a dark cloud instead of by the whole cloud being a little brighter on one side
    const auto weight = emphasis == 1.0 ? normalized : std::pow(normalized, emphasis);
    const auto brightness = floor + (1.0 - floor) * weight;
    const auto chroma = CHROMA_FLOOR + (1.0 - CHROMA_FLOOR) * weight;

    const auto grey = 0.299 * rgb[point * 3 + 0] + 0.587 * rgb[point * 3 + 1] + 0.114 * rgb[point * 3 + 2];

    for (int channel = 0; channel < 3; ++channel)
    {
      auto &value = rgb[point * 3 + channel];
      const auto faded = grey + (static_cast<double>(value) - grey) * chroma;
      value = static_cast<unsigned char>(std::lround(std::min(255.0, std::max(0.0, brightness * faded))));
    }
  }
}

void m2PlotView::UpdateColors()
{
  static const QColor plain(140, 190, 240);

  if (m_Values.empty() || m_Controls->boxColor->currentIndex() < 0)
  {
    m_PointCloud->SetUniformColor(plain);
    return;
  }

  const auto mode = m_Controls->boxColor->currentData().toString();
  const auto pointCount = m_Values.front().size();

  if (IsBrightnessModulated())
  {
    // the colour says what a point is, the brightness how strongly the intensity image answers
    // for it; the two are read together, so the colours have to be built here and multiplied
    std::vector<unsigned char> rgb;
    vtkSmartPointer<vtkScalarsToColors> legend;
    QString legendTitle;

    if (!ComputeBaseColors(rgb, legend, legendTitle))
    {
      rgb.assign(pointCount * 3, 0);
      for (std::size_t point = 0; point < pointCount; ++point)
      {
        rgb[point * 3 + 0] = static_cast<unsigned char>(plain.red());
        rgb[point * 3 + 1] = static_cast<unsigned char>(plain.green());
        rgb[point * 3 + 2] = static_cast<unsigned char>(plain.blue());
      }
    }

    ApplyIntensityBrightness(rgb);
    m_PointCloud->SetColors(rgb, legend, legendTitle);
    return;
  }

  // without the dimming the renderer maps the values itself, which keeps the scalar bar exact
  if (mode == QLatin1String(COLOR_INPUT) && m_InputTable)
  {
    m_PointCloud->SetScalars(m_InputCategories, m_InputTable, QStringLiteral("Image"));
    return;
  }

  if (mode == QLatin1String(COLOR_MASK) && m_LabelTable)
  {
    m_PointCloud->SetScalars(m_LabelCategories, m_LabelTable, QStringLiteral("Label"));
    return;
  }

  if (mode == QLatin1String(COLOR_RGB) && !m_PointColors.empty())
  {
    m_PointCloud->SetColors(m_PointColors);
    return;
  }

  if (mode == QLatin1String(COLOR_PIXEL_RGB) && !m_PixelIndices.empty())
  {
    m_PointCloud->SetColors(ComputePixelPositionColors());
    return;
  }

  std::vector<float> values;
  QString title;

  if (mode == QLatin1String(COLOR_POSITION))
  {
    values.reserve(m_CentroidPositions.size());
    for (const auto position : m_CentroidPositions)
      values.push_back(static_cast<float>(position));
    title = QStringLiteral("m/z");
  }
  else if (mode.startsWith(QLatin1String(COLOR_DIMENSION)))
  {
    const auto dimension = mode.mid(static_cast<int>(std::strlen(COLOR_DIMENSION))).toInt();
    if (dimension < 0 || dimension >= static_cast<int>(m_Values.size()))
    {
      m_PointCloud->SetUniformColor(plain);
      return;
    }
    values = m_Values[dimension];
    title = m_DimensionNames[dimension];
  }
  else
  {
    m_PointCloud->SetUniformColor(plain);
    return;
  }

  if (values.empty())
  {
    m_PointCloud->SetUniformColor(plain);
    return;
  }

  const auto range = std::minmax_element(values.begin(), values.end());
  m_PointCloud->SetScalars(values, MakeContinuousTable(*range.first, *range.second), title);
}

void m2PlotView::UpdateStatus()
{
  const std::size_t drawn = m_Values.empty() ? 0 : m_Values.front().size();

  if (drawn == 0)
  {
    m_Controls->labelStatus->setText(GetSource() == Source::Embedding
                                       ? QStringLiteral("No points. Select one or more embedding images with at "
                                                        "least two components.")
                                       : QStringLiteral("No points. Select a centroid list an analysis has "
                                                        "attached loadings to."));
    return;
  }

  QString text = QStringLiteral("%1 points").arg(FormatCount(drawn));
  if (m_Inputs.size() > 1)
    text += QStringLiteral(" from %1 images").arg(m_Inputs.size());
  if (drawn < m_CandidateCount)
    text += QStringLiteral(", sampled from %1").arg(FormatCount(m_CandidateCount));
  text += QLatin1Char('.');

  m_Controls->labelStatus->setText(text);
}

void m2PlotView::OnPointPicked(int index)
{
  if (index < 0)
  {
    m_Controls->labelInfo->setText(QStringLiteral("No point under the cursor."));
    return;
  }

  QStringList parts;

  if (GetSource() == Source::Embedding)
  {
    if (index >= static_cast<int>(m_PixelIndices.size()))
      return;

    // which image the pixel lies in is the first thing to say about it while more than one is
    // drawn; its index means nothing without it
    if (m_Inputs.size() > 1 && index < static_cast<int>(m_PointInputs.size()))
      parts << QString::fromStdString(m_Inputs[m_PointInputs[index]].node->GetName());

    const auto &pixel = m_PixelIndices[index];
    parts << QStringLiteral("Pixel (%1, %2, %3)").arg(pixel[0]).arg(pixel[1]).arg(pixel[2]);

    if (index < static_cast<int>(m_LabelCategories.size()))
    {
      const auto category = static_cast<int>(m_LabelCategories[index]);
      if (category >= 0 && category < m_LabelCategoryNames.size())
        parts << m_LabelCategoryNames[category];
    }
  }
  else
  {
    if (index >= static_cast<int>(m_CentroidPositions.size()))
      return;

    parts << QStringLiteral("m/z %1").arg(m_CentroidPositions[index], 0, 'f', 4);
    if (!m_CentroidDescriptions[index].isEmpty())
      parts << m_CentroidDescriptions[index];
  }

  for (int axis = 0; axis < 3; ++axis)
  {
    const auto dimension = GetAxisDimension(axis);
    if (dimension >= 0 && index < static_cast<int>(m_Values[dimension].size()))
      parts << QStringLiteral("%1 = %2").arg(m_DimensionNames[dimension]).arg(m_Values[dimension][index], 0, 'g', 4);
  }

  m_Controls->labelInfo->setText(parts.join(QStringLiteral("\n")));
}
