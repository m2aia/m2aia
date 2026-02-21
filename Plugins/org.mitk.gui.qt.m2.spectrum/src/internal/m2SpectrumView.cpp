/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include "m2SpectrumView.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <memory>

// Qt
#include <QColor>
#include <QComboBox>
#include <QFileDialog>
#include <QGraphicsItemGroup>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QLabel>
#include <QLineSeries>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QShortcut>
#include <QValueAxis>
#include <QWidgetAction>
#include <QXYSeries>

// MITK
#include <QmitkIOUtil.h>
#include <mitkColorProperty.h>
#include <mitkCoreServices.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>
#include <mitkNodePredicateDataType.h>
#include <mitkStatusBar.h>

// M2aia
#include <m2CoreCommon.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2SpectrumImage.h>
#include <m2UIUtils.h>

// Internal
#include "m2SeriesDataProvider.h"

const std::string m2SpectrumView::VIEW_ID = "org.mitk.views.m2.spectrum";

// ============================================================================
// Selection area
// ============================================================================

void m2SpectrumView::DrawSelectedArea()
{
  if (!m_xAxis || !m_yAxis)
    return;

  auto *chart = m_Controls.chartView->chart();

  // Create the three overlay line-series on the first call.
  if (!m_SelectedArea[1])
  {
    // --- Baseline (horizontal, drawn at y-min, spanning the selection) ---
    m_SelectedArea[1] = new QLineSeries();
    chart->addSeries(m_SelectedArea[1]);
    m_SelectedArea[1]->attachAxis(m_xAxis);
    m_SelectedArea[1]->attachAxis(m_yAxis);
    {
      QPen pen(QColor(0, 155, 0, 120));
      pen.setWidthF(2.0);
      pen.setMiterLimit(0);
      m_SelectedArea[1]->setPen(pen);
    }
    m_SelectedArea[1]->setName("Selection");

    // --- Left vertical border ---
    m_SelectedArea[0] = new QLineSeries();
    chart->addSeries(m_SelectedArea[0]);
    m_SelectedArea[0]->attachAxis(m_xAxis);
    m_SelectedArea[0]->attachAxis(m_yAxis);
    {
      QPen pen(QColor(0, 155, 0, 255));
      pen.setWidthF(0.75);
      m_SelectedArea[0]->setPen(pen);
    }
    chart->legend()->markers(m_SelectedArea[0])[0]->setVisible(false);

    // --- Right vertical border ---
    m_SelectedArea[2] = new QLineSeries();
    chart->addSeries(m_SelectedArea[2]);
    m_SelectedArea[2]->attachAxis(m_xAxis);
    m_SelectedArea[2]->attachAxis(m_yAxis);
    m_SelectedArea[2]->setPen(m_SelectedArea[0]->pen());
    chart->legend()->markers(m_SelectedArea[2])[0]->setVisible(false);

    // Connect any newly added legend markers so they can be toggled.
    for (auto *marker : chart->legend()->markers())
      QObject::connect(marker, SIGNAL(clicked()), this,
                       SLOT(OnLegendMarkerClicked()), Qt::UniqueConnection);
  }

  UpdateSelectedArea();
}

void m2SpectrumView::UpdateSelectedArea()
{
  if (!m_SelectedArea[0] || !m_SelectedArea[1] || !m_SelectedArea[2])
    return;
  if (m_Chart->series().empty())
    return;

  const double yMin = m_yAxis->min();
  const double yMax = m_yAxis->max();
  const double xL = m_SelectedAreaX[0];
  const double xR = m_SelectedAreaX[1];

  // Baseline
  m_SelectedArea[1]->replace({QPointF{xL, yMin}, QPointF{xR, yMin}});

  // Left border
  m_SelectedArea[0]->replace({QPointF{xL, yMin}, QPointF{xL, yMax}});

  // Right border
  m_SelectedArea[2]->replace({QPointF{xR, yMin}, QPointF{xR, yMax}});

  UpdateSamplingPointOverlay();

  m_Controls.chartView->repaint();
}

// ============================================================================
// Sampling-point overlay
// ============================================================================

void m2SpectrumView::UpdateSamplingPointOverlay()
{
  const bool showSamplingPoints =
    m_M2aiaPreferences->GetBool("m2aia.view.spectrum.showSamplingPoints", false);
  if (!showSamplingPoints)
    return;

  for (auto &kv : m_DataProvider)
  {
    auto *group = m_NodeGraphicItems.at(kv.first);

    // Remove old overlay items.
    for (auto *item : group->childItems())
    {
      group->removeFromGroup(item);
      delete item;
    }

    if (!kv.first->IsVisible(nullptr))
      continue;

    try
    {
      const auto &xs = kv.second->xs();
      const auto &ys = kv.second->ys();

      // Find data points within the current selection range.
      const double selL = std::max(m_SelectedAreaX[0], m_xAxis->min());
      const double selR = std::min(m_SelectedAreaX[1], m_xAxis->max());

      auto itX0 = std::lower_bound(xs.begin(), xs.end(), selL);
      auto itX1 = std::upper_bound(itX0, xs.end(), selR);
      auto itY  = ys.begin() + std::distance(xs.begin(), itX0);

      double size = 5.0;
      if (auto *prop = kv.first->GetProperty("spectrum.marker.size"))
        if (auto *sp = dynamic_cast<mitk::IntProperty *>(prop))
          size = static_cast<double>(sp->GetValue());

      QColor markerColor;
      if (auto *prop = kv.first->GetProperty("spectrum.marker.color"))
        if (auto *cp = dynamic_cast<mitk::ColorProperty *>(prop))
        {
          auto mc = cp->GetColor();
          markerColor.setRgbF(mc.GetRed(), mc.GetGreen(), mc.GetBlue());
        }

      for (; itX0 != itX1; ++itX0, ++itY)
      {
        auto *rect = new QGraphicsRectItem();
        const auto scenePos = m_Chart->mapToPosition(QPointF(*itX0, *itY));
        rect->setRect(scenePos.x() - size / 2.0,
                      scenePos.y() - size / 2.0,
                      size, size);
        if (markerColor.isValid())
        {
          rect->setBrush(QBrush(markerColor));
          rect->setPen(QPen(markerColor));
        }
        rect->setZValue(20.0);
        group->addToGroup(rect);
      }
    }
    catch (const std::exception &e)
    {
      MITK_ERROR << "UpdateSamplingPointOverlay: " << e.what();
    }
  }
}

// ============================================================================
// Mouse interaction
// ============================================================================

void m2SpectrumView::OnMousePress(QPointF /*pos*/, qreal mz, qreal intValue,
                                  Qt::MouseButton button,
                                  Qt::KeyboardModifiers mod)
{
  // Alt + left drag → start a range selection.
  if ((mod & Qt::AltModifier) && !m_RangeSelectionActive)
  {
    m_Controls.chartView->setRubberBand(QChartView::HorizontalRubberBand);
    m_SelectedAreaX[0] = mz;
    m_RangeSelectionActive = true;
  }

  // Middle button → start panning.
  if (button == Qt::MiddleButton && m_xAxis && m_yAxis)
  {
    if (mz > m_xAxis->min() && mz < m_xAxis->max() &&
        intValue > m_yAxis->min() && intValue < m_yAxis->max())
    {
      m_DragPivotMz    = mz;
      m_DragLeftDelta  = mz - m_xAxis->min();
      m_DragRightDelta = m_xAxis->max() - mz;
      m_DraggingActive = true;
    }
  }
}

void m2SpectrumView::OnMouseMove(QPointF pos, qreal mz, qreal intValue,
                                 Qt::MouseButton /*button*/,
                                 Qt::KeyboardModifiers /*mod*/)
{
  const auto *chart = m_Controls.chartView->chart();

  // Middle-button pan: translate the x axis.
  if (m_DraggingActive && m_xAxis)
  {
    auto *mutableChart = m_Controls.chartView->chart();
    const double sceneNowX   = mutableChart->mapToPosition(QPointF(mz, 0.0)).x();
    const double scenePivotX = mutableChart->mapToPosition(QPointF(m_DragPivotMz, 0.0)).x();
    const double pixelDelta  = scenePivotX - sceneNowX;
    m_Controls.chartView->chart()->scroll(pixelDelta, 0.0);
  }

  // Crosshair label.
  if (!chart->series().empty() && m_xAxis && m_yAxis)
  {
    const bool inside = (mz       > m_xAxis->min()) && (mz       < m_xAxis->max()) &&
                        (intValue > m_yAxis->min()) && (intValue < m_yAxis->max());
    if (inside)
    {
      m_Crosshair->setText(QString("%1 @ %2")
                             .arg(intValue, 0, 'e', 3)
                             .arg(mz));
      m_Crosshair->setPos(pos + QPointF(10.0, -30.0));
      m_Crosshair->setZValue(11.0);
      m_Crosshair->show();
    }
    else
    {
      m_Crosshair->hide();
    }
    m_Controls.chartView->repaint();
  }
}

void m2SpectrumView::OnMouseRelease(QPointF /*pos*/, qreal mz,
                                    qreal /*intValue*/,
                                    Qt::MouseButton button,
                                    Qt::KeyboardModifiers mod)
{
  if (m_RangeSelectionActive)
  {
    if (mod & Qt::AltModifier)
      m_SelectedAreaX[1] = mz;

    m_Controls.chartView->setRubberBand(QChartView::NoRubberBand);

    // Emit an image-update signal with the center mz and half-width tolerance.
    const double center = (m_SelectedAreaX[0] + m_SelectedAreaX[1]) * 0.5;
    const double tol    = std::abs(m_SelectedAreaX[1] - m_SelectedAreaX[0]) * 0.5;
    emit m2::UIUtils::Instance()->UpdateImage(center, tol);

    m_RangeSelectionActive = false;
  }

  if (button == Qt::MiddleButton)
    m_DraggingActive = false;
}

void m2SpectrumView::OnMouseDoubleClick(QPointF /*pos*/, qreal xVal, qreal yVal,
                                        Qt::MouseButton /*button*/,
                                        Qt::KeyboardModifiers /*mod*/)
{
  if (m_DataProvider.empty() || !m_xAxis || !m_yAxis)
    return;

  if (yVal < m_yAxis->min())
  {
    // Double-click below the plot area → reset the x axis to global range.
    m_xAxis->setRange(m_GlobalMinX, m_GlobalMaxX);
  }
  else if (xVal < m_xAxis->min())
  {
    // Double-click left of the plot area → reset the y axis.
    m_yAxis->setRange(0.0, m_LocalMaxY * 1.1);
  }
  else if (xVal >= m_xAxis->min() && xVal <= m_xAxis->max() &&
           yVal >= m_yAxis->min() && yVal <= m_yAxis->max())
  {
    // Double-click inside the chart → request an image at the cursor mz.
    emit m2::UIUtils::Instance()->UpdateImage(xVal, -1);
  }

  DrawSelectedArea();
}

void m2SpectrumView::OnMouseWheel(QPointF /*pos*/, qreal x, qreal y,
                                  int angleDelta,
                                  Qt::KeyboardModifiers mod)
{
  if (m_DataProvider.empty() || !m_xAxis || !m_yAxis || angleDelta == 0)
    return;

  const double yMin  = m_yAxis->min();
  const double yMax  = m_yAxis->max();
  const bool inside  = (x > m_xAxis->min()) && (x < m_xAxis->max()) &&
                       (y > yMin)            && (y < yMax);
  const bool belowX  = y <= yMin; // wheel below the plot → zoom x
  const bool leftY   = x <= m_xAxis->min(); // wheel left → zoom y

  const int   dir    = (angleDelta > 0) ? 1 : -1;
  double zoomFactor  = (mod & Qt::ControlModifier) ? 0.35 : 0.10;

  if (belowX || inside)
  {
    const double d0 = std::abs(x - m_xAxis->min()) * zoomFactor * dir;
    const double d1 = std::abs(x - m_xAxis->max()) * zoomFactor * dir;
    m_xAxis->setRange(m_xAxis->min() + d0, m_xAxis->max() - d1);
  }

  if (leftY || (inside && (mod & Qt::ShiftModifier)))
  {
    const double newMax = m_yAxis->max() * (1.0 - dir * zoomFactor);
    m_yAxis->setMax(std::max(newMax, 0.0));
  }
}

// ============================================================================
// Axis range callbacks
// ============================================================================

void m2SpectrumView::OnRangeChangedAxisX(qreal xMin, qreal xMax)
{
  if (m_DataProvider.empty() || !m_xAxis)
    return;

  // Guard: if global extents are still at their sentinel values the data hasn't
  // arrived yet — do nothing until OnDataModified sets them properly.
  if (m_GlobalMinX >= m_GlobalMaxX)
    return;

  // Clamp to the global data range to prevent panning past the data edges.
  bool clamped = false;
  if (xMax > m_GlobalMaxX) { xMax = m_GlobalMaxX; clamped = true; }
  if (xMin < m_GlobalMinX) { xMin = m_GlobalMinX; clamped = true; }

  if (clamped)
  {
    m_xAxis->blockSignals(true);
    m_xAxis->setRange(xMin, xMax);
    m_xAxis->blockSignals(false);
  }

  // Regenerate all series for the new visible range.
  for (auto &kv : m_DataProvider)
    kv.second->GenerateSeriesDataWithinRange(xMin, xMax);

  UpdateCurrentMinMaxY();
  AutoZoomUseLocalExtremaY();
  UpdateSelectedArea();
}

void m2SpectrumView::OnRangeChangedAxisY(qreal min, qreal max)
{
  if (!m_yAxis)
    return;

  // Prevent the user from scrolling to unreasonable y values.
  bool clamped = false;
  if (max > m_GlobalMaxY * 1.1) { max = m_GlobalMaxY * 1.1; clamped = true; }
  if (min < m_GlobalMinY)       { min = m_GlobalMinY;        clamped = true; }

  if (clamped)
  {
    m_yAxis->blockSignals(true);
    m_yAxis->setRange(min, max);
    m_yAxis->blockSignals(false);
  }
}

// ============================================================================
// Slot: mass range changed from an external source (e.g. image slider)
// ============================================================================

void m2SpectrumView::OnMassRangeChanged(qreal centerMz, qreal tol)
{
  // Clamp the incoming range to the global data extents so that the selection
  // markers never fall outside the data region.
  const double clampedCenter = std::max(m_GlobalMinX, std::min(centerMz, m_GlobalMaxX));
  const double clampedL = std::max(m_GlobalMinX, clampedCenter - tol);
  const double clampedR = std::min(m_GlobalMaxX, clampedCenter + tol);

  SetSelectedAreaStartX(clampedL);
  SetSelectedAreaEndX(clampedR);

  // If the center is not within the currently visible x window, scroll the
  // axis to keep the current zoom width but center on the new peak.
  if (m_xAxis && (clampedCenter < m_xAxis->min() || clampedCenter > m_xAxis->max()))
  {
    const double halfWidth = (m_xAxis->max() - m_xAxis->min()) * 0.5;
    const double newMin    = std::max(m_GlobalMinX, clampedCenter - halfWidth);
    const double newMax    = std::min(m_GlobalMaxX, newMin + 2.0 * halfWidth);

    // Block signals while panning so that OnRangeChangedAxisX does not
    // regenerate series before we get to DrawSelectedArea below.
    m_xAxis->blockSignals(true);
    m_xAxis->setRange(newMin, newMax);
    m_xAxis->blockSignals(false);

    // Regenerate all series for the new x window and update local y extents.
    for (auto &kv : m_DataProvider)
      kv.second->GenerateSeriesDataWithinRange(m_xAxis->min(), m_xAxis->max());

    UpdateCurrentMinMaxY();
    AutoZoomUseLocalExtremaY();
  }

  DrawSelectedArea();
}

// ============================================================================
// ITK data/property callbacks
// ============================================================================

void m2SpectrumView::OnDataModified(const itk::Object *caller,
                                    const itk::EventObject &)
{
  const auto *node = dynamic_cast<const mitk::DataNode *>(caller);
  if (!node)
    return;

  auto *ivec = dynamic_cast<m2::IntervalVector *>(node->GetData());
  if (!ivec)
    return;

  // If this node was added before the spectrum view was opened it won't be in
  // m_DataProvider yet.  Initialise it now so the event is not silently lost.
  if (m_DataProvider.find(node) == m_DataProvider.end())
    NodeAdded(node);

  auto it = m_DataProvider.find(node);
  if (it == m_DataProvider.end())
    return;

  auto &provider = it->second;
  provider->SetData(ivec);
  provider->InitializeLoDData();

  UpdateGlobalMinMaxValues();

  if (!m_xAxis || !m_yAxis)
    return;

  // Block axis signals while we reset ranges so that OnRangeChangedAxisX is
  // not re-entered during this initialisation path.
  m_xAxis->blockSignals(true);
  m_yAxis->blockSignals(true);

  if (m_GlobalMinX < m_GlobalMaxX)
    m_xAxis->setRange(m_GlobalMinX, m_GlobalMaxX);

  // Regenerate all providers for the (potentially new) full range.
  for (auto &kv : m_DataProvider)
    kv.second->GenerateSeriesDataWithinRange(m_xAxis->min(), m_xAxis->max());

  UpdateCurrentMinMaxY();

  const double yHigh = m_LocalMaxY * 1.1;
  if (m_LocalMinY < yHigh)
    m_yAxis->setRange(m_LocalMinY, yHigh);

  m_xAxis->blockSignals(false);
  m_yAxis->blockSignals(false);

  DrawSelectedArea();
  m_Controls.chartView->repaint();
}

void m2SpectrumView::OnPropertyListChanged(const itk::Object *caller,
                                           const itk::EventObject &)
{
  const auto *node = dynamic_cast<const mitk::DataNode *>(caller);
  if (!node)
    return;

  if (!dynamic_cast<m2::IntervalVector *>(node->GetData()))
    return;

  auto it = m_DataProvider.find(node);
  if (it == m_DataProvider.end())
    return;

  auto &provider = it->second;

  // Visibility
  const bool visible = node->IsVisible(nullptr);
  provider->GetSeries()->setVisible(visible);

  // Color
  if (auto *prop = node->GetProperty("spectrum.plot.color"))
  {
    if (auto *cp = dynamic_cast<mitk::ColorProperty *>(prop))
    {
      float alpha = 1.0f;
      // node->GetOpacity(alpha, nullptr);
      const auto c = cp->GetColor();
      provider->SetColor(c.GetRed(), c.GetGreen(), c.GetBlue(), alpha);
    }
  }

  // Sampling-point marker color
  if (auto *prop = node->GetProperty("spectrum.marker.color"))
  {
    if (auto *cp = dynamic_cast<mitk::ColorProperty *>(prop))
    {
      const auto mc = cp->GetColor();
      QColor qc;
      qc.setRgbF(mc.GetRed(), mc.GetGreen(), mc.GetBlue());

      for (auto *item : m_NodeGraphicItems.at(node)->childItems())
      {
        if (auto *rect = dynamic_cast<QGraphicsRectItem *>(item))
        {
          rect->setBrush(QBrush(qc));
          rect->setPen(QPen(qc));
        }
      }
    }
  }

  // Sampling-point marker size
  if (auto *prop = node->GetProperty("spectrum.marker.size"))
  {
    if (auto *sp = dynamic_cast<mitk::IntProperty *>(prop))
    {
      const double size = static_cast<double>(sp->GetValue());
      for (auto *item : m_NodeGraphicItems.at(node)->childItems())
      {
        if (auto *rect = dynamic_cast<QGraphicsRectItem *>(item))
        {
          const auto center = rect->rect().center();
          rect->setRect(center.x() - size / 2.0,
                        center.y() - size / 2.0,
                        size, size);
        }
      }
    }
  }
}

// ============================================================================
// NodeAdded
// ============================================================================

void m2SpectrumView::NodeAdded(const mitk::DataNode *node)
{
  if (!node)
    return;

  m_Chart = m_Controls.chartView->chart();

  auto *ivec = dynamic_cast<m2::IntervalVector *>(node->GetData());
  if (!ivec)
    return;

  // Seed global x extents on the very first node (guard against empty data).
  const auto &xMean = ivec->GetXMean();
  if (m_DataProvider.empty() && !xMean.empty())
  {
    m_GlobalMinX = xMean.front();
    m_GlobalMaxX = xMean.back();
  }

  // Build the data provider.
  auto provider = std::make_shared<m2::SeriesDataProvider>();
  provider->SetData(ivec);
  provider->InitializeSeries();
  provider->InitializeLoDData();
  provider->GetSeries()->setName(node->GetName().c_str());
  const bool nodeVisible = node->IsVisible(nullptr);
  provider->GetSeries()->setVisible(nodeVisible);
  m_DataProvider[node] = provider;

  // Sampling-point overlay group.
  auto *group = new QGraphicsItemGroup();
  m_Chart->scene()->addItem(group);
  m_NodeGraphicItems[node] = group;

  // ---- (Re)bind axes ----
  // Disconnect BEFORE addSeries/createDefaultAxes to prevent spurious
  // OnRangeChangedAxisX callbacks while axes are being rebuilt.
  if (m_xAxis)
    QObject::disconnect(m_xAxis, SIGNAL(rangeChanged(qreal, qreal)),
                        this, SLOT(OnRangeChangedAxisX(qreal, qreal)));
  if (m_yAxis)
    QObject::disconnect(m_yAxis, SIGNAL(rangeChanged(qreal, qreal)),
                        this, SLOT(OnRangeChangedAxisY(qreal, qreal)));

  // Add series to chart and rebuild default axes.
  m_Chart->addSeries(provider->GetSeries());
  m_Chart->createDefaultAxes();

  m_xAxis = static_cast<QValueAxis *>(m_Chart->axes(Qt::Horizontal).front());
  m_yAxis = static_cast<QValueAxis *>(m_Chart->axes(Qt::Vertical).front());

  UpdateGlobalMinMaxValues();

  // Guard against degenerate ranges (empty/uninitialized data).
  // NOTE: set the x-axis range BEFORE calling UpdateCurrentMinMaxY so that
  // the local-Y computation operates over the correct visible x window, not
  // over the default [0,1] range left behind by createDefaultAxes().
  if (m_GlobalMinX < m_GlobalMaxX)
    m_xAxis->setRange(m_GlobalMinX, m_GlobalMaxX);

  // Populate series data for the initial visible range – regenerate ALL
  // providers because createDefaultAxes() above rebuilt the axes and reset
  // their range, so any previously added series must also be refreshed.
  for (auto &kv : m_DataProvider)
    kv.second->GenerateSeriesDataWithinRange(m_xAxis->min(), m_xAxis->max());

  // Now that both the x-axis and the series points are correct, compute
  // the local y extents and apply them to the y-axis.
  UpdateCurrentMinMaxY();

  const double yHigh = m_LocalMaxY * 1.1;
  if (m_LocalMinY < yHigh)
    m_yAxis->setRange(m_LocalMinY, yHigh);

  MITK_INFO << "NodeAdded [" << node->GetName() << "]"
            << " xs.size=" << provider->xs().size()
            << " ys.size=" << provider->ys().size()
            << " series.points=" << provider->GetSeries()->count()
            << " series.visible=" << provider->GetSeries()->isVisible()
            << " xAxis=[" << m_xAxis->min() << "," << m_xAxis->max() << "]"
            << " yAxis=[" << m_yAxis->min() << "," << m_yAxis->max() << "]"
            << " GlobalX=[" << m_GlobalMinX << "," << m_GlobalMaxX << "]"
            << " LocalMaxY=" << m_LocalMaxY;

  QObject::connect(m_xAxis, SIGNAL(rangeChanged(qreal, qreal)),
                   this, SLOT(OnRangeChangedAxisX(qreal, qreal)));
  QObject::connect(m_yAxis, SIGNAL(rangeChanged(qreal, qreal)),
                   this, SLOT(OnRangeChangedAxisY(qreal, qreal)));

  OnAxisXTicksChanged(static_cast<int>(m_AxisTicks[0]));
  OnAxisYTicksChanged(static_cast<int>(m_AxisTicks[1]));

  // ---- Register ITK observers ----
  // [0] Base property-list Modified → property/color/size sync
  {
    auto cmd = itk::NodeMemberCommand<m2SpectrumView>::New();
    cmd->SetNode(node);
    cmd->SetCallbackFunction(this, &m2SpectrumView::OnPropertyListChanged);
    const unsigned long tag =
      node->GetPropertyList()->AddObserver(itk::ModifiedEvent(), cmd);
    m_NodeObserverTags[node].push_back(tag);
  }
  // [1] IntervalVectorModified on the DataNode → data update
  {
    auto cmd = itk::NodeMemberCommand<m2SpectrumView>::New();
    cmd->SetNode(node);
    cmd->SetCallbackFunction(this, &m2SpectrumView::OnDataModified);
    const unsigned long tag =
      const_cast<mitk::DataNode *>(node)->AddObserver(m2::IntervalVectorModified(), cmd);
    m_NodeObserverTags[node].push_back(tag);
  }
  // [2] DataNode itk::ModifiedEvent → catches visibility changes made via a
  //     renderer-specific property list (e.g. Data Manager checkbox), which do
  //     NOT propagate to the base GetPropertyList() Modified().
  {
    auto cmd = itk::NodeMemberCommand<m2SpectrumView>::New();
    cmd->SetNode(node);
    cmd->SetCallbackFunction(this, &m2SpectrumView::OnPropertyListChanged);
    const unsigned long tag =
      const_cast<mitk::DataNode *>(node)->AddObserver(itk::ModifiedEvent(), cmd);
    m_NodeObserverTags[node].push_back(tag);
  }

  // Apply default node properties and trigger an initial property sync.
  m2::DefaultNodeProperties(node, false);
  node->GetPropertyList()->Modified();

  // Re-apply normalisation if it was already enabled before this node arrived.
  if (m_NormalizeSeries)
    ApplyNormalization();

  m_Controls.chartView->repaint();
}

// ============================================================================
// NodeRemoved
// ============================================================================

void m2SpectrumView::NodeRemoved(const mitk::DataNode *node)
{
  if (!dynamic_cast<m2::SpectrumImage *>(node->GetData()) &&
      !dynamic_cast<m2::IntervalVector *>(node->GetData()))
    return;

  if (m_DataProvider.find(node) == m_DataProvider.end())
    return;

  // Remove ITK observers.
  // Tag layout: [0] base property list Modified, [1] IntervalVectorModified,
  //             [2] DataNode itk::ModifiedEvent (visibility via any renderer).
  auto tagIt = m_NodeObserverTags.find(node);
  if (tagIt != m_NodeObserverTags.end())
  {
    const auto &tags = tagIt->second;
    if (tags.size() > 0)
      node->GetPropertyList()->RemoveObserver(tags[0]);
    if (tags.size() > 1)
      const_cast<mitk::DataNode *>(node)->RemoveObserver(tags[1]);
    if (tags.size() > 2)
      const_cast<mitk::DataNode *>(node)->RemoveObserver(tags[2]);
    m_NodeObserverTags.erase(tagIt);
  }

  // Remove the sampling-point scene item.
  auto groupIt = m_NodeGraphicItems.find(node);
  if (groupIt != m_NodeGraphicItems.end())
  {
    m_Chart->scene()->removeItem(groupIt->second);
    delete groupIt->second;
    m_NodeGraphicItems.erase(groupIt);
  }

  // Remove the chart series.
  auto provIt = m_DataProvider.find(node);
  if (provIt != m_DataProvider.end())
  {
    m_Controls.chartView->chart()->removeSeries(provIt->second->GetSeries());
    m_DataProvider.erase(provIt);
  }

  // When the last node is removed, reset axis ranges to a sane default.
  if (m_DataProvider.empty())
  {
    if (m_xAxis) m_xAxis->setRange(0.0, 1.0);
    if (m_yAxis)
    {
      m_yAxis->setRange(0.0, 1.0);
      m_yAxis->setLabelFormat("%.6f");
    }
  }
}

// ============================================================================
// Global / local min-max helpers
// ============================================================================

void m2SpectrumView::UpdateGlobalMinMaxValues()
{
  if (m_DataProvider.empty())
    return;

  m_GlobalMinX = std::numeric_limits<double>::max();
  m_GlobalMaxX = std::numeric_limits<double>::lowest();
  m_GlobalMinY = std::numeric_limits<double>::max();
  m_GlobalMaxY = std::numeric_limits<double>::lowest();

  // Use AllNodes (visible SpectrumImage nodes) for the global X range.
  auto visibleSpectrumNodes = m2::UIUtils::AllNodes(GetDataStorage());
  for (const auto &n : *visibleSpectrumNodes)
  {
    if (auto img = dynamic_cast<m2::SpectrumImage *>(n->GetData()))
    {
      m_GlobalMinX = std::min(m_GlobalMinX, img->GetPropertyValue<double>("m2aia.xs.min"));
      m_GlobalMaxX = std::max(m_GlobalMaxX, img->GetPropertyValue<double>("m2aia.xs.max"));
    }
  }

  // Fallback: if no SpectrumImage node contributed an X range (e.g. a standalone
  // IntervalVector was added), derive the global X extents directly from the
  // data providers.
  if (m_GlobalMinX >= m_GlobalMaxX)
  {
    for (const auto &kv : m_DataProvider)
    {
      if (auto *ivec = dynamic_cast<m2::IntervalVector *>(kv.first->GetData()))
      {
        const auto &xMean = ivec->GetXMean();
        if (!xMean.empty())
        {
          m_GlobalMinX = std::min(m_GlobalMinX, static_cast<double>(xMean.front()));
          m_GlobalMaxX = std::max(m_GlobalMaxX, static_cast<double>(xMean.back()));
        }
      }
    }
  }

  // Use m_DataProvider (visible series) for the global Y range.
  for (const auto &kv : m_DataProvider)
  {
    if (!kv.second->GetSeries()->isVisible())
      continue;

    const auto &ys = kv.second->ys();

    if (ys.empty())
      continue;

    const double scale = kv.second->GetYScale();
    const auto [yMinIt, yMaxIt] = std::minmax_element(ys.begin(), ys.end());
    m_GlobalMinY = std::min(m_GlobalMinY, std::max(0.0, *yMinIt * scale));
    m_GlobalMaxY = std::max(m_GlobalMaxY, *yMaxIt * scale);
  }

  const bool useMinIntensity =
    m_M2aiaPreferences->GetBool("m2aia.view.spectrum.useMinIntensity", true);
  if (!useMinIntensity)
    m_GlobalMinY = 0.0;
}

void m2SpectrumView::UpdateCurrentMinMaxY()
{
  if (m_DataProvider.empty() || !m_xAxis)
    return;

  m_LocalMinY = std::numeric_limits<double>::max();
  m_LocalMaxY = std::numeric_limits<double>::lowest();

  const double visXMin = m_xAxis->min();
  const double visXMax = m_xAxis->max();

  for (const auto &kv : m_DataProvider)
  {
    if (!kv.second->GetSeries()->isVisible())
      continue;

    const auto &xs = kv.second->xs();
    const auto &ys = kv.second->ys();

    if (xs.empty() || ys.empty())
      continue;

    const auto lb = std::lower_bound(xs.begin(), xs.end(), visXMin);
    const auto ub = std::upper_bound(xs.begin(), xs.end(), visXMax);
    const auto d0 = std::distance(xs.begin(), lb);
    const auto d1 = std::distance(xs.begin(), ub);

    if (d0 >= d1)
      continue;

    const double scale = kv.second->GetYScale();
    const auto [yMinIt, yMaxIt] =
      std::minmax_element(ys.begin() + d0, ys.begin() + d1);

    m_LocalMinY = std::min(m_LocalMinY, std::max(0.0, *yMinIt * scale));
    m_LocalMaxY = std::max(m_LocalMaxY, *yMaxIt * scale);
  }

  if (m_LocalMaxY == std::numeric_limits<double>::lowest())
    m_LocalMaxY = 1.0;
  if (m_LocalMinY == std::numeric_limits<double>::max())
    m_LocalMinY = 0.0;
}

void m2SpectrumView::UpdateAllSeries()
{
  if (!m_xAxis)
    return;
  for (auto &kv : m_DataProvider)
    kv.second->GenerateSeriesDataWithinRange(m_xAxis->min(), m_xAxis->max());
}

void m2SpectrumView::ApplyNormalization()
{
  for (auto &kv : m_DataProvider)
  {
    double scale = 1.0;
    if (m_NormalizeSeries)
    {
      const auto &ys = kv.second->ys();
      if (!ys.empty())
      {
        const double yMax = *std::max_element(ys.begin(), ys.end());
        if (yMax > 0.0)
          scale = 1.0 / yMax;
      }
    }
    kv.second->SetYScale(scale);
  }

  UpdateAllSeries();
  UpdateGlobalMinMaxValues();
  UpdateCurrentMinMaxY();
  AutoZoomUseLocalExtremaY();
  UpdateSelectedArea();

  // Keep the Y axis title in sync with the normalisation state.
  if (m_yAxis && m_yAxis->isTitleVisible())
    m_yAxis->setTitleText(m_NormalizeSeries ? "Relative Intensity (0\u20131)" : "Intensity (a.u.)");
  m_Controls.chartView->repaint();
}

void m2SpectrumView::AutoZoomUseLocalExtremaY()
{
  if (m_DataProvider.empty() || !m_yAxis)
    return;

  const bool useMax =
    m_M2aiaPreferences->GetBool("m2aia.view.spectrum.useMaxIntensity", true);
  const bool useMin =
    m_M2aiaPreferences->GetBool("m2aia.view.spectrum.useMinIntensity", true);

  if (useMax)
    m_yAxis->setMax(m_LocalMaxY * 1.1);

  if (useMin)
    m_yAxis->setMin(m_LocalMinY);
  else
    m_yAxis->setMin(0.0);
}

// ============================================================================
// Legend / series visibility helper
// ============================================================================

void m2SpectrumView::SetSeriesVisible(QAbstractSeries *series, bool visible)
{
  series->setVisible(visible);
  const double alpha = visible ? 1.0 : 0.5;

  for (auto *marker : m_Chart->legend()->markers())
  {
    if (marker->series() != series)
      continue;

    marker->setVisible(visible);

    auto applyAlpha = [alpha](QColor c) -> QColor
    {
      c.setAlphaF(alpha);
      return c;
    };

    QBrush lb = marker->labelBrush();
    lb.setColor(applyAlpha(lb.color()));
    marker->setLabelBrush(lb);

    QBrush b = marker->brush();
    b.setColor(applyAlpha(b.color()));
    marker->setBrush(b);

    QPen p = marker->pen();
    p.setColor(applyAlpha(p.color()));
    marker->setPen(p);

    break;
  }
}

void m2SpectrumView::OnLegendMarkerClicked()
{
  auto *marker = qobject_cast<QLegendMarker *>(sender());
  Q_ASSERT(marker);
  SetSeriesVisible(marker->series(), !marker->series()->isVisible());
}

// ============================================================================
// Axis tick slots
// ============================================================================

void m2SpectrumView::OnAxisXTicksChanged(int v)
{
  m_AxisTicks[0] = static_cast<unsigned int>(v);
  if (!m_xAxis)
    return;

  m_xAxis->setTickCount(v);
  m_xAxis->setMinorTickCount(1);
  m_xAxis->setLabelsAngle(0);

  // Use a fine semi-transparent grid for a clean, low-clutter look.
  QColor gc = m_xAxis->gridLineColor();
  gc.setAlphaF(0.20);
  m_xAxis->setGridLineColor(gc);
  QPen gridPen(gc);
  gridPen.setWidthF(0.5);
  m_xAxis->setGridLinePen(gridPen);
  QColor mgc = gc;
  mgc.setAlphaF(0.08);
  m_xAxis->setMinorGridLineColor(mgc);
  QPen minorGridPen(mgc);
  minorGridPen.setWidthF(0.5);
  m_xAxis->setMinorGridLinePen(minorGridPen);

  m_xAxis->setLabelFormat("%.1f");
}

void m2SpectrumView::OnAxisYTicksChanged(int v)
{
  m_AxisTicks[1] = static_cast<unsigned int>(v);
  if (!m_yAxis)
    return;

  m_yAxis->setTickCount(v);
  m_yAxis->setMinorTickCount(1);

  QColor gc = m_yAxis->gridLineColor();
  gc.setAlphaF(0.20);
  m_yAxis->setGridLineColor(gc);
  QPen gridPen(gc);
  gridPen.setWidthF(0.5);
  m_yAxis->setGridLinePen(gridPen);
  QColor mgc = gc;
  mgc.setAlphaF(0.08);
  m_yAxis->setMinorGridLineColor(mgc);
  QPen minorGridPen(mgc);
  minorGridPen.setWidthF(0.5);
  m_yAxis->setMinorGridLinePen(minorGridPen);
}

// ============================================================================
// Qt part control setup
// ============================================================================

void m2SpectrumView::CreateQtPartControl(QWidget *parent)
{
  m_Controls.setupUi(parent);

  // The MITK plugin container may add its own layout margins — strip them
  // so the chart view reaches the panel edges without any gap.
  parent->setContentsMargins(0, 0, 0, 0);
  if (parent->layout())
    parent->layout()->setContentsMargins(0, 0, 0, 0);

  // Keyboard shortcuts.
  auto *shortcutSHIFTLeft  = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Left),  parent);
  auto *shortcutSHIFTRight = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Right), parent);
  auto *shortcutSHIFTUp    = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Up),    parent);
  auto *shortcutSHIFTDown  = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Down),  parent);
  auto *shortcutCTRLLeft   = new QShortcut(QKeySequence(Qt::CTRL  | Qt::Key_Left),  parent);
  auto *shortcutCTRLRight  = new QShortcut(QKeySequence(Qt::CTRL  | Qt::Key_Right), parent);

  auto *UIUtils = m2::UIUtils::Instance();
  connect(UIUtils, SIGNAL(RangeChanged(qreal, qreal)),
          this,    SLOT(OnMassRangeChanged(qreal, qreal)));

  connect(shortcutSHIFTRight, SIGNAL(activated()), UIUtils, SIGNAL(NextImage()));
  connect(shortcutSHIFTLeft,  SIGNAL(activated()), UIUtils, SIGNAL(PreviousImage()));
  connect(shortcutSHIFTUp,    SIGNAL(activated()), UIUtils, SIGNAL(IncreaseTolerance()));
  connect(shortcutSHIFTDown,  SIGNAL(activated()), UIUtils, SIGNAL(DecreaseTolerance()));
  connect(shortcutCTRLRight,  SIGNAL(activated()), UIUtils, SIGNAL(NextPeakImage()));
  connect(shortcutCTRLLeft,   SIGNAL(activated()), UIUtils, SIGNAL(PreviousPeakImage()));

  // Preferences.
  auto *prefsService = mitk::CoreServices::GetPreferencesService();
  m_M2aiaPreferences = prefsService->GetSystemPreferences();

  CreateQChartView();
  CreateQChartViewMenu();

  m_Controls.chartView->chart()->legend()->setVisible(false);

  // Allow the chart view to expand and fill all available vertical space.
  m_Controls.chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  // Theme combo box.
  m_Controls.comboBox->addItem("BlueIcy",       QChart::ChartThemeBlueIcy);
  m_Controls.comboBox->addItem("BlueCerulean",  QChart::ChartThemeBlueCerulean);
  m_Controls.comboBox->addItem("BlueNcs",       QChart::ChartThemeBlueNcs);
  m_Controls.comboBox->addItem("BrownSand",     QChart::ChartThemeBrownSand);
  m_Controls.comboBox->addItem("Dark",          QChart::ChartThemeDark);
  m_Controls.comboBox->addItem("HighContrast",  QChart::ChartThemeHighContrast);
  m_Controls.comboBox->addItem("Light",         QChart::ChartThemeLight);
  m_Controls.comboBox->addItem("Qt",            QChart::ChartThemeQt);

  // Preview theme on hover; apply on selection.
  connect(m_Controls.comboBox, qOverload<int>(&QComboBox::highlighted), this,
          [this](int i)
          {
            const auto theme =
              static_cast<QChart::ChartTheme>(m_Controls.comboBox->itemData(i).toInt());
            m_Chart->setTheme(theme);
            // Restore the thin pen width that the theme resets to a thick default.
            for (auto *s : m_Chart->series())
              if (auto *xy = dynamic_cast<QXYSeries *>(s))
              {
                QPen p = xy->pen();
                p.setWidthF(0.75);
                xy->setPen(p);
              }
          });

  connect(m_Controls.comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
          [this](int i)
          {
            const auto theme =
              static_cast<QChart::ChartTheme>(m_Controls.comboBox->itemData(i).toInt());
            m_Chart->setTheme(theme);
          });

  m_Controls.comboBox->setCurrentIndex(4); // Dark by default.

  // Save button.
  connect(m_Controls.saveButton, &QPushButton::clicked, this,
          [this]()
          {
            const auto name = QFileDialog::getSaveFileName(
              m_Controls.chartView, tr("Save Chart Image"), "~",
              tr("Images (*.png *.xpm *.jpg)"));
            if (name.isEmpty())
              return;
            auto r = m_Controls.chartView->rect().adjusted(10, 10, -10, -10);
            m_Controls.chartView->grab(r).save(name);
          });

  // Process IntervalVector nodes that were already in the data storage before
  // this view was first opened (e.g. data loaded before the view was created).
  auto existingNodes = GetDataStorage()->GetSubset(
    mitk::TNodePredicateDataType<m2::IntervalVector>::New());
  for (const auto &n : *existingNodes)
    NodeAdded(n);
}

// ============================================================================
// Chart + context-menu factory helpers
// ============================================================================

void m2SpectrumView::CreateQChartView()
{
  auto *chart = new QChart();
  m_Controls.chartView->setChart(chart);
  m_Chart = chart;

  // Minimise internal whitespace so the plot area fills the widget.
  chart->setMargins(QMargins(0, 0, 0, 0));
  chart->setBackgroundRoundness(0);
  chart->setDropShadowEnabled(false);

  // Remove the QGraphicsView viewport padding that would otherwise add
  // extra space between the chart scene and the widget border.
  
  m_Controls.chartView->setContentsMargins(0, 0, 0, 0);
  if (m_Controls.chartView->viewport())
    m_Controls.chartView->viewport()->setContentsMargins(0, 0, 0, 0);

  chart->legend()->setAlignment(Qt::AlignRight);
  chart->legend()->setShowToolTips(true);
  chart->setAnimationOptions(QChart::NoAnimation);
  chart->setTheme(QChart::ChartThemeBlueIcy);

  m_Crosshair = new QGraphicsSimpleTextItem("", chart);
  auto brush   = m_Crosshair->brush();
  brush.setColor(Qt::white);
  m_Crosshair->setBrush(brush);
  m_Crosshair->hide();

  connect(m_Controls.chartView, &m2::ChartView::mousePress,
          this, &m2SpectrumView::OnMousePress);
  connect(m_Controls.chartView, &m2::ChartView::mouseMove,
          this, &m2SpectrumView::OnMouseMove);
  connect(m_Controls.chartView, &m2::ChartView::mouseRelease,
          this, &m2SpectrumView::OnMouseRelease);
  connect(m_Controls.chartView, &m2::ChartView::mouseDoubleClick,
          this, &m2SpectrumView::OnMouseDoubleClick);
  connect(m_Controls.chartView, &m2::ChartView::mouseWheel,
          this, &m2SpectrumView::OnMouseWheel);
}

void m2SpectrumView::CreateQChartViewMenu()
{
  m_Menu = new QMenu(m_Controls.chartView);

  // ---- Normalize series ----
  m_NormalizeAction = new QAction("Normalize series (0\u20131)", m_Controls.chartView);
  m_NormalizeAction->setCheckable(true);
  m_NormalizeAction->setChecked(false);
  m_NormalizeAction->setToolTip(
    "Divide each series by its own maximum so all spectra share the 0\u20131 range.\n"
    "Useful when spectra have very different absolute intensity scales.");
  m_Menu->addAction(m_NormalizeAction);
  connect(m_NormalizeAction, &QAction::toggled, this,
          [this](bool on)
          {
            m_NormalizeSeries = on;
            ApplyNormalization();
          });

  m_Menu->addSeparator();

  // ---- Scale All ----
  m_ScaleAll = new QAction("Scale All", m_Controls.chartView);
  m_ScaleAll->setToolTip("Reset both axes to show the full data range");
  m_Menu->addAction(m_ScaleAll);
  connect(m_ScaleAll, &QAction::triggered, this,
          [this]()
          {
            if (!m_xAxis || !m_yAxis)
              return;
            m_xAxis->blockSignals(true);
            m_xAxis->setRange(m_GlobalMinX, m_GlobalMaxX);
            m_xAxis->blockSignals(false);
            for (auto &kv : m_DataProvider)
              kv.second->GenerateSeriesDataWithinRange(m_GlobalMinX, m_GlobalMaxX);
            UpdateCurrentMinMaxY();
            const double yHigh = m_LocalMaxY * 1.1;
            m_yAxis->setRange(m_LocalMinY > 0.0 ? m_LocalMinY : 0.0, yHigh);
            UpdateSelectedArea();
          });

  m_Menu->addSeparator();

  // ---- Show/hide axis titles ----
  m_ShowAxesTitles = new QAction("Show axes titles", m_Controls.chartView);
  m_ShowAxesTitles->setCheckable(true);
  m_ShowAxesTitles->setChecked(false);
  m_Menu->addAction(m_ShowAxesTitles);
  connect(m_ShowAxesTitles, &QAction::toggled, this,
          [this](bool show)
          {
            if (m_xAxis)
            {
              m_xAxis->setTitleText("m/z");
              m_xAxis->setTitleVisible(show);
            }
            if (m_yAxis)
            {
              m_yAxis->setTitleText(m_NormalizeSeries ? "Relative Intensity (0\u20131)" : "Intensity (a.u.)");
              m_yAxis->setTitleVisible(show);
            }
          });

  // ---- Show/hide legend ----
  m_ShowLegend = new QAction("Show legend", m_Controls.chartView);
  m_ShowLegend->setCheckable(true);
  m_ShowLegend->setChecked(false);
  m_Menu->addAction(m_ShowLegend);
  connect(m_ShowLegend, &QAction::toggled, this,
          [this](bool show)
          {
            m_Chart->legend()->setVisible(show);
          });

  m_Menu->addSeparator();

  // ---- X axis tick count ----
  auto *labelX   = new QLabel("X axis ticks", m_Controls.chartView);
  m_TickCountX   = new QSlider(Qt::Horizontal, m_Controls.chartView);
  m_TickCountX->setRange(3, 50);
  m_TickCountX->setValue(static_cast<int>(m_AxisTicks[0]));

  auto *actionLabelX = new QWidgetAction(m_Controls.chartView);
  actionLabelX->setDefaultWidget(labelX);
  auto *actionSliderX = new QWidgetAction(m_Controls.chartView);
  actionSliderX->setDefaultWidget(m_TickCountX);
  m_Menu->addAction(actionLabelX);
  m_Menu->addAction(actionSliderX);
  connect(m_TickCountX, &QSlider::valueChanged,
          this, &m2SpectrumView::OnAxisXTicksChanged);

  // ---- Y axis tick count ----
  auto *labelY   = new QLabel("Y axis ticks", m_Controls.chartView);
  m_TickCountY   = new QSlider(Qt::Horizontal, m_Controls.chartView);
  m_TickCountY->setRange(3, 10);
  m_TickCountY->setValue(static_cast<int>(m_AxisTicks[1]));

  auto *actionLabelY = new QWidgetAction(m_Controls.chartView);
  actionLabelY->setDefaultWidget(labelY);
  auto *actionSliderY = new QWidgetAction(m_Controls.chartView);
  actionSliderY->setDefaultWidget(m_TickCountY);
  m_Menu->addAction(actionLabelY);
  m_Menu->addAction(actionSliderY);
  connect(m_TickCountY, &QSlider::valueChanged,
          this, &m2SpectrumView::OnAxisYTicksChanged);

  // ---- Help ----
  m_Menu->addSeparator();
  auto *helpAction = new QAction("Help...", m_Controls.chartView);
  m_Menu->addAction(helpAction);
  connect(helpAction, &QAction::triggered, this,
          [this]()
          {
            auto *dlg = new QDialog(m_Controls.chartView);
            dlg->setWindowTitle("Spectrum View – Controls");
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->resize(520, 400);

            auto *browser = new QTextBrowser(dlg);
            browser->setOpenExternalLinks(true);
            browser->setHtml(
              "<h3>Spectrum View Controls</h3>"
              "<table cellspacing='4' cellpadding='2'>"
              "<tr><th align='left'>Mouse / Key</th><th align='left'>Action</th></tr>"
              "<tr><td><b>Double-click</b> inside chart</td><td>Request ion image at cursor m/z</td></tr>"
              "<tr><td><b>Double-click</b> below x-axis</td><td>Reset x-axis to full data range</td></tr>"
              "<tr><td><b>Double-click</b> left of y-axis</td><td>Reset y-axis to local range</td></tr>"
              "<tr><td><b>Scroll wheel</b> inside / below chart</td><td>Zoom x-axis around cursor</td></tr>"
              "<tr><td><b>Scroll wheel</b> + Ctrl</td><td>Zoom x-axis faster</td></tr>"
              "<tr><td><b>Scroll wheel</b> + Shift</td><td>Zoom y-axis</td></tr>"
              "<tr><td><b>Scroll wheel</b> left of y-axis</td><td>Zoom y-axis</td></tr>"
              "<tr><td><b>Middle button drag</b></td><td>Pan x-axis</td></tr>"
              "<tr><td><b>Alt + drag</b></td><td>Select m/z range (updates ion image)</td></tr>"
              "<tr><td><b>Click legend marker</b></td><td>Show / hide that series</td></tr>"
              "<tr><td><b>Shift &#8592; / &#8594;</b></td><td>Previous / next image</td></tr>"
              "<tr><td><b>Shift &#8593; / &#8595;</b></td><td>Increase / decrease tolerance</td></tr>"
              "<tr><td><b>Ctrl &#8592; / &#8594;</b></td><td>Previous / next peak image</td></tr>"
              "</table>"
              "<p><i>Right-click for the context menu (Normalize series, Scale All, axis options, help).</i></p>"
            );

            auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dlg);
            connect(buttons, &QDialogButtonBox::rejected, dlg, &QDialog::reject);

            auto *layout = new QVBoxLayout(dlg);
            layout->addWidget(browser);
            layout->addWidget(buttons);

            dlg->exec();
          });

  // Context-menu trigger.
  m_Controls.chartView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_Controls.chartView, &QChartView::customContextMenuRequested, this,
          [this](const QPoint &pos)
          {
            m_Menu->exec(m_Controls.chartView->viewport()->mapToGlobal(pos));
          });
}
