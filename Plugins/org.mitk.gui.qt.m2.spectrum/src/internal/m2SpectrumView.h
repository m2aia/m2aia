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
#include <mitkIPreferences.h>

#include <itkCommand.h>
#include <m2SpectrumImage.h>

#include "m2ChartView.h"
#include "m2SeriesDataProvider.h"
#include "ui_m2SpectrumView.h"

#include <QLineSeries>
#include <QValueAxis>
#include <qlegendmarker.h>
#include <qslider.h>

class QMenu;
class QGraphicsItem;
class QGraphicsItemGroup;
class QGraphicsSimpleTextItem;

namespace itk
{
  /**
   * @brief ITK command that carries the originating DataNode alongside the
   *        standard member-function callback.
   *
   * This lets a single handler distinguish which node triggered an event
   * without needing lambdas or extra context maps.
   */
  template <class T>
  class NodeMemberCommand : public itk::MemberCommand<T>
  {
  public:
    const mitk::DataNode *m_Node = nullptr;

    typedef NodeMemberCommand Self;
    typedef itk::SmartPointer<Self> Pointer;

    itkNewMacro(Self);
    itkTypeMacro(NodeMemberCommand, itk::MemberCommand);

    void SetNode(const mitk::DataNode *node) { m_Node = node; }

    void Execute(itk::Object *, const itk::EventObject &event) override
    {
      itk::MemberCommand<T>::Execute(m_Node, event);
    }

    void Execute(const itk::Object *, const itk::EventObject &event) override
    {
      itk::MemberCommand<T>::Execute(m_Node, event);
    }
  };
} // namespace itk

class m2SpectrumView : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;
  ~m2SpectrumView() override = default;

protected:
  // -----------------------------------------------------------------------
  // QmitkAbstractView interface
  // -----------------------------------------------------------------------
  void CreateQtPartControl(QWidget *parent) override;
  void NodeAdded(const mitk::DataNode *node) override;
  void NodeRemoved(const mitk::DataNode *node) override;
  void SetFocus() override {}

  // -----------------------------------------------------------------------
  // ITK event callbacks (called through NodeMemberCommand)
  // -----------------------------------------------------------------------
  void OnPropertyListChanged(const itk::Object *caller, const itk::EventObject &event);
  void OnDataModified(const itk::Object *caller, const itk::EventObject &event);

  // -----------------------------------------------------------------------
  // Chart setup helpers
  // -----------------------------------------------------------------------
  void CreateQChartView();
  void CreateQChartViewMenu();

  // -----------------------------------------------------------------------
  // Chart update helpers
  // -----------------------------------------------------------------------

  /**
   * Regenerate all visible series for the current x-axis range and
   * recompute the y extrema.
   */
  void UpdateAllSeries();

  /**
   * Apply or remove per-series Y normalisation.
   *
   * When m_NormalizeSeries is true each provider's scale factor is set to
   * 1 / max(ys) so all series share the 0–1 range on the Y axis.
   * When false every scale is reset to 1.0 (absolute intensities).
   */
  void ApplyNormalization();

  /**
   * Scan all data providers for the global min/max x and y extents across
   * the full data range (all nodes, ignoring the current zoom window).
   */
  void UpdateGlobalMinMaxValues();

  /**
   * Scan visible data providers for the local y min/max within the
   * currently visible x range.
   */
  void UpdateCurrentMinMaxY();

  /**
   * Adjust the y-axis range to the local extrema found by
   * UpdateCurrentMinMaxY(), optionally clamped by preference flags.
   */
  void AutoZoomUseLocalExtremaY();

  /**
   * Make a series and its legend marker visible/invisible, adjusting the
   * marker alpha to indicate the toggled state.
   */
  void SetSeriesVisible(QAbstractSeries *series, bool visible);

  // -----------------------------------------------------------------------
  // Selection area (rubber-band range marker)
  // -----------------------------------------------------------------------

  /**
   * Draw (or redraw) the three line-series that make up the selection
   * indicator on the chart (horizontal baseline + two vertical borders).
   * Creates the series the first time it is called.
   */
  void DrawSelectedArea();

  /**
   * Update the coordinates of the already-existing selection-area series
   * using m_SelectedAreaX and the current y-axis range.
   */
  void UpdateSelectedArea();

  /// Set the left edge of the current selection in data coordinates.
  void SetSelectedAreaStartX(double v) { m_SelectedAreaX[0] = v; }

  /// Set the right edge of the current selection in data coordinates.
  void SetSelectedAreaEndX(double v) { m_SelectedAreaX[1] = v; }

  // -----------------------------------------------------------------------
  // Sampling-point overlay
  // -----------------------------------------------------------------------

  /**
   * Rebuild the QGraphicsRectItem overlay that shows individual data points
   * within the current selection range when the preference is enabled.
   */
  void UpdateSamplingPointOverlay();

protected slots:
  // -----------------------------------------------------------------------
  // Slots connected to UIUtils / shortcuts
  // -----------------------------------------------------------------------
  void OnMassRangeChanged(qreal centerMz, qreal tol);

  // -----------------------------------------------------------------------
  // Slots connected to mouse events on the chart view
  // -----------------------------------------------------------------------
  void OnMousePress(QPointF pos, qreal mz, qreal intValue,
                    Qt::MouseButton button, Qt::KeyboardModifiers mod);
  void OnMouseMove(QPointF pos, qreal mz, qreal intValue,
                   Qt::MouseButton button, Qt::KeyboardModifiers mod);
  void OnMouseRelease(QPointF pos, qreal mz, qreal intValue,
                      Qt::MouseButton button, Qt::KeyboardModifiers mod);
  void OnMouseDoubleClick(QPointF pos, qreal mz, qreal intValue,
                          Qt::MouseButton button, Qt::KeyboardModifiers mod);
  void OnMouseWheel(QPointF pos, qreal mz, qreal intValue,
                    int angleDelta, Qt::KeyboardModifiers mod);

  // -----------------------------------------------------------------------
  // Slots connected to axis range signals
  // -----------------------------------------------------------------------
  void OnRangeChangedAxisX(qreal min, qreal max);
  void OnRangeChangedAxisY(qreal min, qreal max);

  // -----------------------------------------------------------------------
  // Menu / UI slots
  // -----------------------------------------------------------------------
  void OnLegendMarkerClicked();
  void OnAxisXTicksChanged(int v);
  void OnAxisYTicksChanged(int v);

private:
  // -----------------------------------------------------------------------
  // UI
  // -----------------------------------------------------------------------
  Ui_m2SpectrumViewControls m_Controls;

  QMenu *m_Menu = nullptr;
  QAction *m_ShowAxesTitles = nullptr;
  QAction *m_ShowLegend = nullptr;
  QAction *m_ScaleAll = nullptr;
  QAction *m_NormalizeAction = nullptr;
  QSlider *m_TickCountX = nullptr;
  QSlider *m_TickCountY = nullptr;

  QChart *m_Chart = nullptr;
  QValueAxis *m_xAxis = nullptr;
  QValueAxis *m_yAxis = nullptr;

  QGraphicsSimpleTextItem *m_Crosshair = nullptr;

  // -----------------------------------------------------------------------
  // Preferences
  // -----------------------------------------------------------------------
  mitk::IPreferences *m_M2aiaPreferences = nullptr;

  // -----------------------------------------------------------------------
  // Per-node data
  // -----------------------------------------------------------------------

  /// Maps each registered DataNode to its spectrum data provider.
  std::map<const mitk::DataNode *, std::shared_ptr<m2::SeriesDataProvider>> m_DataProvider;

  /// Maps each DataNode to the QGraphicsItemGroup used for the sampling-point
  /// overlay that is drawn on top of the chart scene.
  std::map<const mitk::DataNode *, QGraphicsItemGroup *> m_NodeGraphicItems;

  /// Maps each DataNode to the ITK observer tags registered on it, so that
  /// they can be removed precisely in NodeRemoved().
  std::map<const mitk::DataNode *, std::vector<unsigned long>> m_NodeObserverTags;

  // -----------------------------------------------------------------------
  // Axis / view state
  // -----------------------------------------------------------------------

  /// Desired number of major tick marks on each axis.
  std::array<unsigned int, 2> m_AxisTicks = {9u, 4u};

  /// Global extents: full data range across all loaded nodes.
  double m_GlobalMinX = 0.0;
  double m_GlobalMaxX = 1.0;
  double m_GlobalMinY = 0.0;
  double m_GlobalMaxY = 1.0;

  /// Local extents: extrema within the currently visible x window.
  double m_LocalMinY = 0.0;
  double m_LocalMaxY = 1.0;

  // -----------------------------------------------------------------------
  // Selection area state
  // -----------------------------------------------------------------------

  /// Data-space x coordinates [left, right] of the current selection.
  std::array<double, 2> m_SelectedAreaX = {0.0, 0.0};

  /// The three line series that draw the selection indicator.
  /// [0] = left vertical border, [1] = horizontal baseline, [2] = right border.
  std::array<QLineSeries *, 3> m_SelectedArea = {nullptr, nullptr, nullptr};

  // -----------------------------------------------------------------------
  // Interaction state
  // -----------------------------------------------------------------------

  /// Used for middle-button panning: pivot mz value when drag started.
  double m_DragPivotMz = 0.0;
  /// Distance from pivot to the left axis edge when drag started.
  double m_DragLeftDelta = 0.0;
  /// Distance from pivot to the right axis edge when drag started.
  double m_DragRightDelta = 0.0;
  bool m_DraggingActive = false;

  /// True while the user is performing an Alt+drag range selection.
  bool m_RangeSelectionActive = false;

  /// True when per-series normalisation (0–1 scaling) is active.
  bool m_NormalizeSeries = false;
};
