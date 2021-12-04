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

#include <qlegendmarker.h>
#include <qscatterseries.h>
#include <qslider.h>
#include <berryISelectionListener.h>
#include <QmitkAbstractView.h>
#include <m2SpectrumImageBase.h>
#include "m2ChartView.h"
#include "m2Crosshair.h"
#include "ui_m2Spectrum.h"

class QMenu;
class QActionGroup;
namespace QtCharts
{
  class QValueAxis;
}

class m2Spectrum : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:
  static const std::string VIEW_ID;
  ~m2Spectrum(){}

protected:
  virtual void CreateQtPartControl(QWidget *parent) override;
  void CreateQChartView();
  void CreateQChartViewMenu();
  m2Crosshair *m_Crosshair;

  std::map<const mitk::DataNode *, QtCharts::QLineSeries *> m_PeakSeries;
  std::map<const mitk::DataNode *, std::map<m2::OverviewSpectrumType, QVector<QPointF>>> m_PeakTypeData;

  std::map<const mitk::DataNode *, QtCharts::QLineSeries *> m_LineSeries;
  std::map<const mitk::DataNode *, std::map<m2::OverviewSpectrumType, std::vector<QVector<QPointF>>>>
    m_LineTypeLevelData;
  std::map<const mitk::DataNode *, unsigned> m_Level;

  std::map<const mitk::DataNode *, QtCharts::QScatterSeries *> m_ScatterSeries;

  void OnUpdateScatterSeries(const mitk::DataNode *);
  void UpdateLineSeriesWindow(const mitk::DataNode *);
  void UpdateAxisLabels(const mitk::DataNode *, bool remove = false);
  void UpdateZoomLevel(const mitk::DataNode *);

  void SetDefaultLineSeriesStyle(QtCharts::QLineSeries *);
  void SetDefaultScatterSeriesStyle(QtCharts::QScatterSeries *);

  void CreateLevelData(const mitk::DataNode *node);
  void CreatePeakData(const mitk::DataNode *node);

  struct BiasedSereisContainer : public std::map<m2::OverviewSpectrumType, QtCharts::QXYSeries *>
  {
    std::vector<double> GetXAxis;
    QColor Color = {0, 0, 0, 0};
  };
  void UpdateSelectedArea();

protected slots:
  void OnDataNodeReceived(const mitk::DataNode *node);

  void OnMassRangeChanged(qreal x, qreal tol);
  void OnSeriesFocused(const mitk::DataNode *node);
  void OnLegnedHandleMarkerClicked();
  void OnResetView();

  void OnAxisXTicksChanged(int v);
  void OnAxisYTicksChanged(int v);

  void OnMousePress(QPoint pos, qreal mz, qreal intValue, Qt::MouseButton button, Qt::KeyboardModifiers mod);
  void OnMouseMove(QPoint pos, qreal mz, qreal intValue, Qt::MouseButton button, Qt::KeyboardModifiers mod);
  void OnMouseRelease(QPoint pos, qreal mz, qreal intValue, Qt::MouseButton button, Qt::KeyboardModifiers mod);
  void OnMouseDoubleClick(QPoint pos, qreal mz, qreal intValue, Qt::MouseButton button, Qt::KeyboardModifiers mod);
  void OnMouseWheel(QPoint pos, qreal mz, qreal intValue, int angle, Qt::KeyboardModifiers mod);
  void OnOverviewSpectrumChanged(const mitk::DataNode *node, m2::OverviewSpectrumType specType);

  void OnRangeChangedAxisX(qreal min, qreal max);
  void OnRangeChangedAxisY(qreal min, qreal max);

protected:
  unsigned int m_yAxisTicks = 4;
  unsigned int m_xAxisTicks = 9;

  const std::unordered_map<mitk::DataNode *, BiasedSereisContainer> &GetDataAssociatedSeries()
  {
    return m_DataAssociatedSeriesMap;
  }

  void UpdateSeriesMinMaxValues();

  void SetSelectedAreaStartX(double v) { m_SelectedAreaStartX = v; }
  void SetSelectedAreaEndX(double v) { m_SelectedAreaEndX = v; }
  void DrawSelectedArea();

protected:
  std::unordered_map<mitk::DataNode *, BiasedSereisContainer> m_DataAssociatedSeriesMap;
  QtCharts::QLineSeries *m_SelectedAreaLeft = nullptr;
  QtCharts::QLineSeries *m_SelectedAreaRight = nullptr;
  QtCharts::QLineSeries *m_SelectedAreaLower = nullptr;
  double m_SelectedAreaStartX = 0;
  double m_SelectedAreaEndX = 0;

  char m_StatusBarTextBuffer[500];
  double m_CurrentMousePosMz = 0;
  double m_CurrentVisibleDataPoints = 0;

  m2::OverviewSpectrumType m_CurrentOverviewSpectrumType = m2::OverviewSpectrumType::Mean;
  bool m_CurrentOverviewSpectrumTypeChanged = false;

  virtual void SetFocus() override {}
  Ui::imsSpectrumControls m_Controls;

  std::unique_ptr<QtCharts::QScatterSeries> m_LastMzMarker;
  std::vector<std::pair<double, double>> m_AlignmentRegions;

  // virtual void NodeAdded(const mitk::DataNode *node) override;
  virtual void NodeRemoved(const mitk::DataNode *node) override;

  QtCharts::QChart *m_chart = nullptr;
  QtCharts::QAbstractSeries *m_IonImageIndicator = nullptr;

  double m_LastDa = 1;
  double m_LastPPM = 75;

  double m_LastMz = -1;
  double m_LastTol = -1;

  double m_CurrentMaxIntensity;
  double m_CurrentMaxMZ;
  double m_CurrentMinMZ;

  double m_MouseDragCenterPos = 0;
  double m_MouseDragLowerDelta = 0;
  double m_MouseDragUpperDelta = 0;
  bool m_DraggingActive = false;
  QRectF m_DragBounds = QRectF(0, 0, 0, 0);
  QFuture<void> m_DragFuture;

  bool m_RangeSelectionStarted = false;

private:
  QMenu *m_Menu;
  QAction *m_SpectrumSkyline;
  QAction *m_SpectrumMean;
  QAction *m_SpectrumSum;
  QActionGroup *m_SpectrumSelectionGroup;
  QAction *m_ShowLegend;
  QMenu *m_FocusMenu;
  QAction *m_ShowAxesTitles;
  QSlider *m_TickCountX;
  QSlider *m_TickCountY;
  QtCharts::QValueAxis *m_xAxis;
  QtCharts::QValueAxis *m_yAxis;

  QVector<QString> m_xAxisTitels;

};
