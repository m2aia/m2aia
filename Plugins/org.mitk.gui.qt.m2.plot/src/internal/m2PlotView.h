/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef m2PlotView_h
#define m2PlotView_h

#include <QmitkAbstractView.h>
#include <QmitkAbstractNodeSelectionWidget.h>

#include <mitkImage.h>
#include <mitkLabelSetImage.h>

#include <itkCommand.h>
#include <itkIndex.h>
#include <vtkSmartPointer.h>

#include <QStringList>

#include <map>
#include <string>
#include <vector>

class m2PointCloudWidget;
class vtkScalarsToColors;

namespace Ui
{
  class m2PlotViewControls;
}

/**
  \brief An interactive point cloud of a dimensionality reduction, in two or three dimensions.

  A reduction produces two things that are read in the same space but do not live in the same
  data: an embedding, one point per pixel, and loadings, one value per centroid and component.
  This view plots either of them as a point cloud whose axes are any two or three of the
  components, so a PCA is looked at as the score plot of its pixels and as the loading plot of its
  peaks without leaving the view.

  The selections are what the plot is built from. An embedding image is a multi component image
  such as the one the Data Compression view publishes; a component of it is one axis. Several of
  them can be selected at once, which is what a combined reduction is looked at with: a run over
  more than one image publishes one embedding per image in the same component space, and drawing
  them in one cloud is what says whether the images share that space or sit apart in it.

  Every embedding is read together with the images that describe it. The mask says what a pixel
  is and colours the cloud by label; an RGB image gives a point the colour of its pixel; a single
  channel image dims the cloud by its value. Each of those is a selection of its own, and every
  embedding is paired with the entry of that selection that covers its grid, so a combined
  embedding is described by handing the view the mask of every image it was computed from.

  The centroid list carries the loadings as features named "<image>.<method>.c<component>",
  attached by the method that computed them, and one such analysis at a time is what the loading
  plot draws.

  Clicking a point names it: a pixel by its image, its index and its label, a centroid by its m/z.
  Everything else - which components are on which axis, what the colour means, how many points are
  drawn - is a control on the left and takes effect at once, without reading the images again.
*/
class m2PlotView : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;

  m2PlotView();
  ~m2PlotView() override;

  void CreateQtPartControl(QWidget *parent) override;
  void SetFocus() override;

private slots:
  /** One of the data selections changed; everything is read again. */
  void OnInputChanged();
  /** Switched between the embedding and the loadings. */
  void OnSourceChanged();
  /** Another analysis of the centroid list was chosen for the loading plot. */
  void OnAnalysisChanged();
  /** Another component was put on an axis. */
  void OnAxisChanged();
  void OnColorChanged();
  void OnPointSizeChanged(int size);
  /** The intensity images changed; the plot follows the new ones from now on. */
  void OnIntensitySelectionChanged();
  void OnResetCamera();
  /** Describes the point that was clicked in the info line. */
  void OnPointPicked(int index);

private:
  /** What the points of the cloud are. */
  enum class Source
  {
    /** One point per pixel of an embedding image. */
    Embedding,
    /** One point per centroid of the peak list, placed by its loadings. */
    Loadings
  };

  /** One embedding image and the images describing it, together with the stretch of the cloud
      its pixels became. The companions are kept as nodes rather than as images because a view
      that recomputes in place puts a new image on the node it publishes to. */
  struct InputImage
  {
    mitk::DataNode::Pointer node;
    mitk::Image::Pointer image;
    mitk::DataNode::Pointer maskNode;
    mitk::DataNode::Pointer rgbNode;
    mitk::DataNode::Pointer intensityNode;

    /** Pixels of this image that took part in the reduction, before the sampling. */
    std::size_t candidates = 0;
    /** The points this image contributed, as [pointBegin, pointEnd). */
    std::size_t pointBegin = 0;
    std::size_t pointEnd = 0;
  };

  Source GetSource() const;

  /** Reads the current selection into m_Values and the per point book keeping, then rebuilds the
      axis and colour choices and the plot. Everything the controls can change afterwards is
      answered from m_Values, so the images are read once per selection. */
  void RebuildCloud();
  void ReadEmbedding();
  void ReadLoadings();

  /** The selected embeddings, each paired with the mask, the RGB image and the intensity image
      that cover its grid. Images this view cannot read are left out and reported. */
  std::vector<InputImage> CollectInputs() const;

  /** The entry of a companion selection that describes the given embedding: one whose grid
      matches, and among those the one that was derived from the same image. Null when none of
      the selected nodes covers the grid, which is reported once per embedding. */
  mitk::DataNode *MatchCompanion(const QList<mitk::DataNode::Pointer> &candidates,
                                 const mitk::DataNode *embeddingNode,
                                 const mitk::Image *embedding,
                                 const char *what) const;

  /** The image a node of a companion selection is compared and read by: the group 0 image of a
      segmentation, the image itself of anything else. */
  static mitk::Image *GridImageOf(const mitk::DataNode *node);

  /** The image the given node was computed from, by walking the data storage up to the node that
      has no source of its own. Two nodes with the same root describe the same measurement, which
      is what pairs an embedding with its own mask rather than with the mask of another image. */
  mitk::DataNode *RootSource(const mitk::DataNode *node) const;

  /** The mask of an input, or null when none was matched or it carries no group. */
  static mitk::MultiLabelSegmentation *MaskOf(const InputImage &input);
  /** The image a companion node of an input currently carries, or null. */
  static mitk::Image *ImageOf(const mitk::DataNode *node);

  /** Takes the raw colour channels of the points of one input from its RGB image and writes them
      into the given buffer. Returns whether that image already holds bytes. */
  bool AppendPointColorChannels(const InputImage &input,
                                const mitk::Image *rgbImage,
                                std::vector<double> &channels) const;

  /** Turns the collected channels into the colour of every point. Images of bytes already hold
      the colour they are meant to show; anything else - the three components of a reduction
      before they are rendered as RGB - is stretched onto that range per channel, over all inputs
      at once so that a combined embedding keeps one common scale. */
  void FinishPointColors(const std::vector<double> &channels, bool allBytes);

  /** Takes the value of the points of one input from its single channel image - an ion image, or
      a similarity map of the Embedding Search view. */
  void AppendPointIntensities(const InputImage &input, const mitk::Image *intensityImage);

  /** Drops everything that was read, leaving an empty plot. */
  void ClearCloud();

  /** Builds the categorical colour table naming the labels the points carry, together with the
      dense category index every point is coloured by. Labels of different masks that share a name
      share a category, so the same class of two images is one entry of the legend. */
  void BuildLabelTable();

  /** Builds the categorical colour table naming the images the points came from. Only of use
      while more than one embedding is drawn. */
  void BuildImageTable();

  /** Follows the given nodes, so that a map computed while the plot is open colours the cloud at
      once instead of after the next selection. Both a node and the image it carries are watched:
      a view that recomputes in place - an ion image - modifies the image, while one that
      publishes a new result - the Embedding Search view - puts a new image on the node. */
  void ObserveIntensityNodes(const QList<mitk::DataNode::Pointer> &nodes);
  void ReleaseIntensityObservers();

  /** Called when a followed node or image was modified; asks for a refresh rather than reading
      the image on the spot. */
  void OnIntensityModified();

  /** Reads the intensity images again and recolours the cloud. Runs from the event loop, once the
      operation that modified the image has finished. */
  void RefreshIntensityColors();

  /** Whether two images hold the same number of pixels in every direction. */
  static bool SameGrid(const mitk::Image *left, const mitk::Image *right);

  /** The analyses the selected centroid list carries loadings of, as the prefixes their features
      share. Empty when it carries no loadings. */
  QStringList GetAnalyses() const;

  /** The features of one analysis, in the order of their components. */
  QStringList GetAnalysisFeatures(const QString &analysis) const;

  /** The analysis a loading feature belongs to; the feature name itself for a name that does not
      follow the "<analysis>.c<component>" shape, so a value from elsewhere is still plottable. */
  static QString AnalysisOfFeature(const QString &featureName);

  /** Repopulates the axis boxes with the dimensions of the current cloud, keeping the previous
      choice wherever the dimension still exists. */
  void UpdateAxisSelection();
  void UpdateColorSelection();
  void UpdateAnalysisSelection();

  /** Hands the chosen dimensions to the widget. */
  void UpdatePlot();
  /** Hands the chosen colouring to the widget. */
  void UpdateColors();

  /** The colour of every point under the current choice, three values per point, together with
      the table and title that say what those colours mean. Used when the colours have to be
      computed here rather than mapped by the renderer, which is what dimming them by the
      intensity image needs. False when the choice yields no colours. */
  bool ComputeBaseColors(std::vector<unsigned char> &rgb,
                         vtkSmartPointer<vtkScalarsToColors> &legend,
                         QString &legendTitle) const;

  /** Dims every colour by the value the intensity image gives its point. */
  void ApplyIntensityBrightness(std::vector<unsigned char> &rgb) const;

  /** Whether the intensity images should dim the colours right now. */
  bool IsBrightnessModulated() const;

  /** The position of every point in its image, each axis stretched onto one colour channel. */
  std::vector<unsigned char> ComputePixelPositionColors() const;
  void UpdateStatus();

  /** The dimension chosen in the given box, or -1 for "none". */
  int GetAxisDimension(int axis) const;

  /** A perceptually ordered ramp over the given range, for a continuous value. */
  static vtkSmartPointer<vtkScalarsToColors> MakeContinuousTable(double minimum, double maximum);

  Ui::m2PlotViewControls *m_Controls = nullptr;
  m2PointCloudWidget *m_PointCloud = nullptr;

  /** Names of the dimensions of the current cloud, in the order of m_Values. */
  QStringList m_DimensionNames;
  /** One vector per dimension, each with one value per point. */
  std::vector<std::vector<float>> m_Values;

  /** The embeddings the current cloud was read from, in the order their points were appended. */
  std::vector<InputImage> m_Inputs;

  /** Which pixel every point of an embedding cloud came from, what the mask calls it, and which
      of the inputs it belongs to. */
  std::vector<itk::Index<3>> m_PixelIndices;
  std::vector<mitk::MultiLabelSegmentation::LabelValueType> m_PixelLabels;
  std::vector<int> m_PointInputs;

  /** What every point of a loading cloud stands for. */
  std::vector<double> m_CentroidPositions;
  std::vector<QString> m_CentroidDescriptions;

  /** The colour every point takes from the RGB image of its input, three values per point. Empty
      unless every input has one. */
  std::vector<unsigned char> m_PointColors;

  /** The value every point takes from the intensity image of its input, one per point. Empty
      unless every input has one. */
  std::vector<float> m_PointIntensities;
  /** What those values are, so the scalar bar says which map is shown. */
  QString m_IntensityName;

  /** The nodes the plot follows, and the observers on them and on the images they carry. */
  struct WatchedNode
  {
    mitk::DataNode::Pointer node;
    mitk::BaseData::Pointer data;
    unsigned long nodeTag = 0;
    unsigned long dataTag = 0;
  };
  std::vector<WatchedNode> m_IntensityWatches;
  /** True while a refresh is already queued, so a burst of modifications costs one pass. */
  bool m_IntensityRefreshPending = false;

  /** The label of every point as a dense category index, and the table naming those categories.
      Both are empty when the cloud is not coloured by a mask. */
  std::vector<float> m_LabelCategories;
  vtkSmartPointer<vtkScalarsToColors> m_LabelTable;
  /** The name of every category, for the line describing a picked point. */
  QStringList m_LabelCategoryNames;

  /** The input of every point as a category index, and the table naming the images. Both are
      empty while a single embedding is drawn, where the colouring carries nothing. */
  std::vector<float> m_InputCategories;
  vtkSmartPointer<vtkScalarsToColors> m_InputTable;

  /** How many points the data offered before the sampling that keeps the plot interactive. */
  std::size_t m_CandidateCount = 0;

  /** True while the combo boxes are repopulated, so their signals do not start a rebuild. */
  bool m_Updating = false;
};

#endif // m2PlotView_h
