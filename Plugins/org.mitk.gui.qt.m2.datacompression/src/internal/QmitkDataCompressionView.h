/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#pragma once

#include <berryISelectionListener.h>
#include <QmitkAbstractView.h>
#include <QmitkSingleNodeSelectionWidget.h>
#include <ui_DataCompressionViewControls.h>
#include <mitkImage.h>
#include <mitkLabelSetImage.h>

#include <m2EmbeddingFilterBase.h>
#include <m2SpectralFeatureMatrix.h>

#include <QFutureWatcher>

#include <functional>
#include <string>
#include <utility>
#include <vector>

/** Property every result of this view carries; it names the method that produced it and is what
    the export looks for, so a new method does not have to be added to it. */
#define M2AIA_DIMENSION_REDUCTION_METHOD_PROPERTY "m2aia.dimensionreduction.method"

class QmitkDataCompressionView : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;
  void CreateQtPartControl(QWidget *parent) override;
  ~QmitkDataCompressionView() override;

signals:
  /** Emitted from the thread a method runs on; connected queued so that the UI is only touched
      on the GUI thread. */
  void MethodProgress(unsigned int done, unsigned int total);

private slots:
  void OnStartPCA();
  void OnStartTSNE();
  void OnStartKMeans();
  void OnStartNMF();
  void OnStartICA();
  void OnStartPlsDa();
  void OnSaveDataCompressionResults();
  /** Repopulates the label value selection with the unique label values of the currently selected masks. */
  void OnMaskSelectionChanged();
  /** Publishes the results of the method that just finished. */
  void OnMethodFinished();
  void OnCancelMethod();
  void OnMethodProgress(unsigned int done, unsigned int total);
  /** Starts the run of the next image waiting for the current method, if there is one. Methods
      that treat every image on its own are queued instead of run at once, so that only one of
      them occupies a thread and the feature matrix of only one image exists at a time. */
  void StartNextPendingRun();

private:
  /** An image of the selection together with the mask restricting it. */
  using Input = std::pair<mitk::DataNode::ConstPointer, mitk::Image::Pointer>;

  /** One image produced by a method, ready to be added to the data storage. */
  struct MethodResult
  {
    mitk::DataNode::ConstPointer sourceNode;
    mitk::Image::Pointer image;
    std::string name;
  };

  /** How the results of a method are named and published. */
  struct MethodOptions
  {
    /** Appended to the name of the source node, or used as the whole name if
        prefixWithSourceName is false. */
    std::string resultName;
    bool prefixWithSourceName = true;
    /** Rescale the embedding onto [0,255] and publish it as an RGB image. */
    bool convertToRGB = false;
    /** Resample the result to the grid of the source image; needed when the features were built
        with a shrink factor. */
    bool resampleToSource = false;
    /** Number of clusters, for the labels of a clustering result. */
    unsigned int numberOfClusters = 0;
    /** Whether the components should also be attached to the peak list as features. */
    bool publishLoadings = false;
    /** The peak list the features were built from; the loadings are attached to it. */
    mitk::DataNode::ConstPointer peakListNode;
  };

  /** The components of a method expressed over the peaks they were computed from. */
  struct MethodLoadings
  {
    /** The image the method ran on; it names the features so their origin stays visible. */
    mitk::DataNode::ConstPointer sourceNode;
    /** The peak list the values are attached to. */
    mitk::DataNode::ConstPointer peakListNode;
    /** Rows are the components, columns the peaks. */
    Eigen::MatrixXf values;
  };

  /** Everything one run of a method produces. Collected on the worker thread, turned into data
      nodes on the GUI thread. */
  struct MethodRun
  {
    std::string methodName;
    std::vector<MethodResult> results;
    bool producesLabels = false;
    unsigned int numberOfClusters = 0;
    std::vector<MethodLoadings> loadings;
  };

  /** The label value currently chosen in the label value selection, 0 if there is none. */
  mitk::MultiLabelSegmentation::LabelValueType GetSelectedMaskLabelValue() const;

  /** The selected mask that belongs to the given image node, null if there is none. */
  mitk::DataNode::ConstPointer GetMaskNode(const mitk::DataNode *imageNode);

  /** Mask restricting the given image to the pixels of the chosen label value.
      Falls back to the segmentation of the image if no mask is selected for it and
      returns null if the selected mask does not contain the chosen label value. */
  mitk::Image::Pointer GetMaskImage(const mitk::DataNode *imageNode);

  /** The images of the selection that can take part in a run, paired with their mask. Images
      without initialized access or without a usable mask are left out. */
  std::vector<Input> GetUsableInputs();

  /** The images of the selection paired with the group image of the mask that belongs to them,
      with its label values kept so that every label is a class. Images whose mask does not mark
      at least two classes are left out; the label value zero is the background and is not a
      class. Only the supervised method uses this. */
  std::vector<Input> GetSupervisedInputs();

  /** The peaks of the selected peak list; empty if none is selected. */
  std::vector<m2::Interval> GetSelectedIntervals() const;
  std::string GetSelectedPeakListName() const;
  /** The selected peak list node, null if none is selected. */
  mitk::DataNode::ConstPointer GetSelectedPeakListNode() const;

  /** Runs the method on a worker thread; the results are published in OnMethodFinished. */
  /** Returns whether the run was actually started. */
  bool StartMethod(m2::EmbeddingFilterBase::Pointer filter,
                   m2::SpectralFeatureMatrix features,
                   std::vector<mitk::DataNode::ConstPointer> sourceNodes,
                   const MethodOptions &options);

  /** Turns the output of a finished filter into images; runs on the worker thread. */
  MethodRun CollectResults(m2::EmbeddingFilterBase *filter,
                           const std::vector<mitk::DataNode::ConstPointer> &sourceNodes,
                           const MethodOptions &options) const;

  /** Adds an embedding as a child of its source node, replacing an earlier result of the same name. */
  void PublishImage(const MethodResult &result, const std::string &methodName);
  /** Adds a cluster assignment as a segmentation below its source node. */
  void PublishSegmentation(const MethodResult &result, const std::string &methodName, unsigned int numberOfClusters);
  /** Attaches one feature per component to the peak list, so that the feature list view can show
      what every centroid contributes to every component of every method side by side. */
  void AttachLoadings(const MethodLoadings &loadings, const std::string &methodName);

  /** Starts a method that produces an embedding for every selected image on its own. */
  void StartPerImageMethod(const std::string &resultName,
                           unsigned int numberOfComponents,
                           std::vector<Input> inputs,
                           std::function<m2::EmbeddingFilterBase::Pointer()> createFilter);

  void SetMethodRunning(bool running);

  mitk::Image::Pointer ResampleVectorImage(mitk::Image::Pointer lowResImage, mitk::Image::Pointer referenceImage);
  void SetFocus() override;
  QWidget *m_Parent;

  std::vector<Input> m_PendingInputs;
  /** Builds the features and starts the current method for one image. */
  std::function<void(const Input &)> m_StartRunForInput;

  QFutureWatcher<MethodRun> m_MethodWatcher;
  m2::EmbeddingFilterBase::Pointer m_RunningFilter;
  MethodOptions m_RunningOptions;

  Ui::DataCompressionViewControls m_Controls;
};
