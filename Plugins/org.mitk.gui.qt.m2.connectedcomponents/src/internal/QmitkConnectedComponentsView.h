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

#include <QmitkAbstractView.h>
#include <QmitkSingleNodeSelectionWidget.h>
#include <QmitkMultiNodeSelectionWidget.h>
#include <mitkImage.h>
#include <mitkLabelSetImage.h>  // defines mitk::MultiLabelSegmentation

#include <ui_ConnectedComponentsViewControls.h>

#include <vector>

class QmitkGroupWidget;

/**
 * @brief MITK plugin view for connected component analysis with group-based
 *        size comparison.
 *
 * Workflow:
 *  1. Select a binary or multilabel segmentation image.
 *  2. Apply optional morphological cleanup (erosion, dilation, closing,
 *     fill holes).
 *  3. Run ITK connected component labeling.
 *  4. Define size-based groups; each group is assigned a color that maps to
 *     mitk::Label entries in the resulting MultiLabelSegmentation.
 *  5. Three groups are automatically suggested from size quantiles.
 */
class QmitkConnectedComponentsView : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;
  void CreateQtPartControl(QWidget *parent) override;

private slots:
  // Morphology
  void OnErode();
  void OnDilate();
  void OnClose();
  void OnFillHoles();

  // Connected Components
  void OnCreateConnectedComponents();

  // Groups
  void OnSuggestGroups();
  void OnAddGroup();
  void OnGroupChanged();
  void OnRemoveGroup(QmitkGroupWidget *widget);

  // Apply
  void OnApplyGroups();

  // CSV Export
  void OnExportCSV();

private:
  void SetFocus() override;

  // Helper
  void UpdateGroupCountLabels();
  void ClearGroups();
  void AddGroupWidget(const QString &name, int minSize, int maxSize, const QColor &color, int absoluteMax = 999999);
  mitk::Image::Pointer ApplyMorphologyToImage(mitk::Image::Pointer inputImage, const std::string &operation, int radius);

  /** Commits a morphology result back to the data-node.
   *  - mitk::MultiLabelSegmentation → converts binary result to LabelValueType and
   *    adds the result as a new group via seg->AddGroup().
   *  - plain mitk::Image             → replaces node data directly.
   */
  void CommitMorphologyResult(mitk::DataNode *node, mitk::Image::Pointer result);

  /** Returns the workable mitk::Image from the selected node:
   *  - mitk::MultiLabelSegmentation  → GetGroupImage(0)
   *  - plain mitk::Image             → the image itself
   *  - anything else                 → nullptr
   */
  mitk::Image* ResolveInputImage(mitk::DataNode* node) const;

  /**
   * Write a CSV file.
   * @param pixelWise  true → one row per voxel; false → one row per cluster (object-wise means).
   */
  void ExportCSV(bool pixelWise);

  // UI
  QWidget *m_Parent = nullptr;
  Ui::ConnectedComponentsViewControls m_Controls;

  // Group widgets list (owned by scroll-area widget)
  std::vector<QmitkGroupWidget *> m_GroupWidgets;

  // Current connected-component result
  mitk::DataNode::Pointer m_CCNode;

  // Component sizes (sorted descending after relabeling)
  std::vector<unsigned long> m_ComponentSizes;  // index 0 = largest component

  // Pre-calculated working image (morphology applied, before CC)
  mitk::Image::Pointer m_WorkingImage;
};
