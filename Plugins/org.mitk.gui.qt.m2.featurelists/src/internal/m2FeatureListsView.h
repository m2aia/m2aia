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

#include "m2LoadingsWidget.h"

#include <QmitkAbstractView.h>
#include <QStringList>
#include <berryISelectionListener.h>
#include <m2IntervalVector.h>

#include <atomic>
#include <utility>
#include <vector>
// #include <QTableWidgetItem>

class QTableWidgetItem;
class QTableWidget;
class QLabel;

/**
  \brief m2FeatureListsView

  Shows all m2::IntervalVector data nodes selected in the DataManager as a
  tabular list. Each row represents one m2::Interval entry with columns:
  Node, #, m/z (mean), m/z (min), m/z (max), Intensity (mean), Description, Color.

  Beyond those, one column is added per analysis whose loadings the shown interval vectors carry.
  A feature is a named value attached to a centroid by some analysis, such as the loading of a
  principal component; the features of one analysis are drawn together in a single cell as a
  miniature plot by m2LoadingsWidget. Each analysis gets its own column so that its components are
  scaled against each other rather than against those of an unrelated method, and so that a
  centroid described by thirty components still occupies one row of ordinary height. The colours
  of a column follow from the method its features came from, so a PCA always looks like a PCA. See
  m2::IntervalVector::SetFeature.

  The view follows the data it shows: an interval vector that is changed while it is displayed
  rebuilds the table, so a result attached by another view appears without reselecting anything.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class m2FeatureListsView : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;
  ~m2FeatureListsView() override;

protected:
  void CreateQtPartControl(QWidget *parent) override;
  void SetFocus() override;

  /// \brief Called by QmitkAbstractView when the DataManager selection changes.
  void OnSelectionChanged(berry::IWorkbenchPart::Pointer source,
                          const QList<mitk::DataNode::Pointer> &nodes) override;

  /// Drops a node that disappeared from the data storage and rebuilds the table.
  void NodeRemoved(const mitk::DataNode *node) override;

private slots:
  /// Emit UpdateImage for the interval in the clicked row.
  void OnCellClicked(int row, int col);
  /// Emit UpdateImage for the interval in the currently selected row.
  void OnCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
  /// Write edited Description back to the underlying m2::IntervalVector.
  void OnItemChanged(QTableWidgetItem *item);
  /// Open a QColorDialog when the Color cell is double-clicked.
  void OnCellDoubleClicked(int row, int col);
  /// Show header context menu to toggle column visibility.
  void OnHeaderContextMenu(const QPoint &pos);
  /// Rebuild the table after one of the shown interval vectors was changed.
  void OnDisplayedDataModified();

private:
  /// Repopulate m_Table from the given list of nodes.
  void PopulateTable(const QList<mitk::DataNode::Pointer> &nodes);

  /// One column of loadings: the features of a single analysis, in the order they were attached.
  struct LoadingGroup
  {
    /// What the column is called, for example "image.PCA".
    QString name;
    /// The method the features came from, for example "PCA"; it decides the colours.
    QString method;
    /// Full names of the features, which is what the tooltips show.
    QStringList features;
    /// The largest absolute value each of those features reaches over the shown nodes. Scaling a
    /// component against its own extreme keeps it comparable between centroids; against a scale
    /// shared with every other component a weak one would simply vanish.
    std::vector<double> scales;
    /// The colours every cell of this column is drawn with.
    m2LoadingsWidget::ColorTable colors;
  };

  /// Groups the features of the given nodes by the analysis they came from, in the order they
  /// first appear. A feature named "<something>.c<number>" belongs to the group "<something>";
  /// any other name forms a group of its own.
  std::vector<LoadingGroup> CollectLoadingGroups(const QList<mitk::DataNode::Pointer> &nodes) const;

  /// The part of a feature name that names the analysis it came from.
  static QString GroupOfFeature(const QString &featureName);

  /// The method within a group name, which is what its colours are chosen from. The analyses name
  /// their features "<image>.<method>.c<number>", so the method is the last part of the group.
  static QString MethodOfGroup(const QString &groupName);

  /// Listen to the interval vectors that are shown, so that the table follows their changes.
  void AttachModifiedObservers();
  void DetachModifiedObservers();
  /// Called by ITK when an observed interval vector changes; may run on any thread.
  void OnObservedDataModified();

  /// For each node whose IntervalVector has duplicate x.mean() values, look for
  /// a folder named after the parent imzML file (without extension) and read all
  /// .nrrd headers found there. Collected feature records are printed via MITK_INFO.
  void ScanFeatureNrrdFolder(const QList<mitk::DataNode::Pointer> &nodes);

  QTableWidget *m_Table = nullptr;
  QLabel *m_InfoLabel = nullptr;

  /// The nodes the table currently shows, kept so that it can be rebuilt without a new selection.
  QList<mitk::DataNode::Pointer> m_DisplayedNodes;
  /// The observed interval vectors and the observer tag on each of them. The smart pointer keeps
  /// the object alive so that the observer can always be removed again.
  std::vector<std::pair<m2::IntervalVector::Pointer, unsigned long>> m_ModifiedObservers;
  /// One per loadings column, in the order the columns follow the fixed ones.
  std::vector<LoadingGroup> m_LoadingGroups;
  /// Guards against rebuilding the table from within a rebuild.
  bool m_Populating = false;
  /// Collapses a burst of changes - attaching several features one after another, for instance -
  /// into a single rebuild. Set from the thread the change comes from.
  std::atomic<bool> m_RebuildPending{false};
};
