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
#include <berryISelectionListener.h>
#include <m2IntervalVector.h>
// #include <QTableWidgetItem>

class QTableWidgetItem;
class QTableWidget;
class QLabel;

/**
  \brief m2FeatureListsView

  Shows all m2::IntervalVector data nodes selected in the DataManager as a
  tabular list. Each row represents one m2::Interval entry with columns:
  Node, #, m/z (mean), m/z (min), m/z (max), Intensity (mean), Description, Color.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class m2FeatureListsView : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;

protected:
  void CreateQtPartControl(QWidget *parent) override;
  void SetFocus() override;

  /// \brief Called by QmitkAbstractView when the DataManager selection changes.
  void OnSelectionChanged(berry::IWorkbenchPart::Pointer source,
                          const QList<mitk::DataNode::Pointer> &nodes) override;

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

private:
  /// Repopulate m_Table from the given list of nodes.
  void PopulateTable(const QList<mitk::DataNode::Pointer> &nodes);

  /// For each node whose IntervalVector has duplicate x.mean() values, look for
  /// a folder named after the parent imzML file (without extension) and read all
  /// .nrrd headers found there. Collected feature records are printed via MITK_INFO.
  void ScanFeatureNrrdFolder(const QList<mitk::DataNode::Pointer> &nodes);

  QTableWidget *m_Table = nullptr;
  QLabel *m_InfoLabel = nullptr;
};
