/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include "m2Spectrum.h"

#include <QApplication>
#include <QValueAxis>
#include <berryIPreferences.h>
#include <berryPlatform.h>
#include <berryPlatformUI.h>
#include <iostream>
#include <m2ImzMLSpectrumImage.h>
#include <m2UIUtils.h>
#include <mitkLookupTableProperty.h>
#include <mitkStatusBar.h>
#include <qlabel.h>
#include <qmenu.h>
#include <qvalueaxis.h>
#include <qwidgetaction.h>
#include <signal/m2PeakDetection.h>

const std::string m2Spectrum::VIEW_ID = "org.mitk.views.m2.Spectrum";

void m2Spectrum::OnOverviewSpectrumChanged(const mitk::DataNode *node, m2::OverviewSpectrumType specType)
{
  if (specType == m2::OverviewSpectrumType::PeakIndicators)
  {
    OnUpdateScatterSeries(node);
  }
}

void m2Spectrum::DrawSelectedArea()
{
  if (!m_xAxis)
    return;
  if (!m_yAxis)
    return;

  auto chart = m_Controls.chartView->chart();
  if (m_SelectedAreaLower == nullptr)
  {
    m_SelectedAreaLower = new QtCharts::QLineSeries();
    chart->addSeries(m_SelectedAreaLower);
    m_SelectedAreaLower->attachAxis(m_xAxis);
    m_SelectedAreaLower->attachAxis(m_yAxis);

    const auto markers = chart->legend()->markers();
    for (auto *marker : markers)
    {
      QObject::connect(marker, SIGNAL(clicked()), this, SLOT(OnLegnedHandleMarkerClicked()), Qt::UniqueConnection);
    }

    QPen pen(QColor(0, 155, 0, 120));
    pen.setWidthF(2);
    pen.setMiterLimit(0);
    m_SelectedAreaLower->setPen(pen);
    m_SelectedAreaLower->setName("Selection");

    m_SelectedAreaRight = new QtCharts::QLineSeries();
    chart->addSeries(m_SelectedAreaRight);
    m_SelectedAreaRight->attachAxis(m_xAxis);
    m_SelectedAreaRight->attachAxis(m_yAxis);

    QPen borderpen(QColor(0, 155, 0, 255));
    borderpen.setWidthF(0.75);
    m_SelectedAreaRight->setPen(borderpen);

    chart->legend()->markers(m_SelectedAreaRight)[0]->setVisible(false);

    m_SelectedAreaLeft = new QtCharts::QLineSeries();
    chart->addSeries(m_SelectedAreaLeft);
    m_SelectedAreaLeft->attachAxis(m_xAxis);
    m_SelectedAreaLeft->attachAxis(m_yAxis);

    m_SelectedAreaLeft->setPen(borderpen);
    chart->legend()->markers(m_SelectedAreaLeft)[0]->setVisible(false);
  }

  UpdateSelectedArea();
}

void m2Spectrum::UpdateSelectedArea()
{
  if (m_SelectedAreaLower)
  {
    m_SelectedAreaLower->clear();
    *m_SelectedAreaLower << QPointF(m_SelectedAreaStartX, m_yAxis->min())
                         << QPointF(m_SelectedAreaEndX, m_yAxis->min());

    m_SelectedAreaLeft->clear();
    *m_SelectedAreaLeft << QPointF(m_SelectedAreaStartX, m_yAxis->min())
                        << QPointF(m_SelectedAreaStartX, m_yAxis->max());

    m_SelectedAreaRight->clear();
    *m_SelectedAreaRight << QPointF(m_SelectedAreaEndX, m_yAxis->min()) << QPointF(m_SelectedAreaEndX, m_yAxis->max());

    m_Controls.chartView->repaint();
  }
}

void m2Spectrum::OnMousePress(QPoint pos, qreal mz, qreal intValue, Qt::MouseButton button, Qt::KeyboardModifiers mod)
{
  Q_UNUSED(pos)
  if (mod & Qt::AltModifier && !m_RangeSelectionStarted)
  {
    m_Controls.chartView->setRubberBand(QtCharts::QChartView::RubberBand::HorizontalRubberBand);
    m_SelectedAreaStartX = mz;
    m_RangeSelectionStarted = true;
  }

  if (button == Qt::MouseButton::MidButton)
  {
    auto c = m_Controls.chartView->chart();
    const auto *xAx = dynamic_cast<QtCharts::QValueAxis *>(c->axes(Qt::Horizontal).front());
    const auto *yAx = dynamic_cast<QtCharts::QValueAxis *>(c->axes(Qt::Vertical).front());
    if (mz > xAx->min() && mz < xAx->max() && intValue > yAx->min() && intValue < yAx->max())
    {
      m_MouseDragCenterPos = mz;
      m_MouseDragLowerDelta = mz - xAx->min();
      m_MouseDragUpperDelta = xAx->max() - mz;
      m_DraggingActive = true;
    }
  }
}

void m2Spectrum::SetDefaultLineSeriesStyle(QtCharts::QLineSeries *series)
{
  auto p = series->pen();

  // p.setStyle(Qt::SolidLine);
  // p.setWidth(2);
  p.setWidthF(.75);
  p.setCapStyle(Qt::FlatCap);
  series->setPen(p);
}

void m2Spectrum::SetDefaultScatterSeriesStyle(QtCharts::QScatterSeries *series)
{
  series->setMarkerShape(QtCharts::QScatterSeries::MarkerShape::MarkerShapeRectangle);
  series->setMarkerSize(3);
}

void m2Spectrum::CreateLevelData(const mitk::DataNode *node)
{
  if (auto image = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    // determine level of details
    const auto &mzs = image->GetXAxis();
    const std::vector<double> scales = {1.0, 2.0, 4.0, 8.0, 16.0};

    const auto &artifacts = image->GetSpectraArtifacts();
    std::function<void(QVector<QPointF> & s, QPointF && p)> PushBackFormat = [](auto &s, auto &&p) -> void
    { s.push_back(p); };

    if (any(image->GetImportMode() &
            (m2::SpectrumFormatType::ContinuousCentroid | m2::SpectrumFormatType::ProcessedCentroid)))
    {
      PushBackFormat = [](auto &s, auto &&p) -> void
      {
        s.push_back({p.x(), -0.3});
        s.push_back(p);
        s.push_back({p.x(), -0.3});
      };
    }

    for (const auto &kv : artifacts)
    {
      const auto type = kv.first;
      const auto &data = kv.second;

      m_LineTypeLevelData[node][type].resize(scales.size());
      // std::vector<m2::MassValue> peaks;
      // const auto mad = m2::Noise::mad(data);
      // m2::Peaks::localMaxima(std::begin(data), std::end(data), std::begin(xs), std::back_inserter(peaks), 10, 2 *
      // mad); auto p = std::cbegin(peaks);
      auto &level = m_LineTypeLevelData[node][type];

      // level 0
      std::transform(std::begin(mzs),
                     std::end(mzs),
                     std::begin(data),
                     std::back_inserter(level[0]),
                     [](auto &m, auto &i) -> QPointF {
                       return {m, i};
                     });

      // level 1 - N
      for (unsigned int k = 1; k < scales.size(); ++k)
      {
        level[k].clear();

        PushBackFormat(level[k], {mzs.front(), data.front()});
        auto dataIt = std::begin(data);
        auto mzsIt = std::begin(mzs);
        const auto s = scales[k];

        while (dataIt != std::end(data))
        {
          double mzSum = 0;
          double intMax = 0;
          unsigned count;
          for (count = 0; count < s && dataIt != std::end(data); ++count, ++dataIt, ++mzsIt)
          {
            mzSum += *mzsIt;
            intMax = std::max(intMax, *dataIt);
          }
          if (count == 0)
            break;
          PushBackFormat(level[k], {mzSum / double(count), intMax});
        }
        PushBackFormat(level[k], {mzs.back(), data.back()});
      }
    }
  }
}

void m2Spectrum::CreatePeakData(const mitk::DataNode *node)
{
  if (auto image = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    // determine level of details
    const auto &mzs = image->GetXAxis();
    const auto &artifacts = image->GetSpectraArtifacts();
    std::function<void(QVector<QPointF> & s, QPointF && p)> PushBackFormat = [](auto &s, auto &&p) -> void
    {
      s.push_back({p.x(), -0.3});
      s.push_back(p);
      s.push_back({p.x(), -0.3});
    };

    for (const auto &kv : artifacts)
    {
      // const auto type = kv.first;
      const auto &data = kv.second;

      auto mzIt = std::begin(mzs);
      auto intIt = std::begin(data);
      while (mzIt != std::end(mzs))
        PushBackFormat(m_PeakTypeData[node][kv.first], {*mzIt++, *intIt++});
    }
  }
}

void m2Spectrum::OnMassRangeChanged(qreal x, qreal tol)
{
  SetSelectedAreaStartX(x - tol);
  SetSelectedAreaEndX(x + tol);
  DrawSelectedArea();
}

void m2Spectrum::OnDataNodeReceived(const mitk::DataNode *node)
{
  if (!node)
    return;
  if (auto baseImage = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    auto chart = m_Controls.chartView->chart();
    bool isCentroidSpectrum = false;

    isCentroidSpectrum = any(baseImage->GetImportMode() &
                             (m2::SpectrumFormatType::ContinuousCentroid | m2::SpectrumFormatType::ProcessedCentroid));

    const unsigned numSeries = m_LineSeries.size() + m_ScatterSeries.size() + m_PeakSeries.size();

    QColor color;
    QtCharts::QLineSeries *seriesLine;
    const auto &type = m_CurrentOverviewSpectrumType;
    if (isCentroidSpectrum)
    {
      seriesLine = m_PeakSeries[node] = new QtCharts::QLineSeries();
      CreatePeakData(node);
      seriesLine->replace(m_PeakTypeData[node][type]);
    }
    else
    {
      seriesLine = m_LineSeries[node] = new QtCharts::QLineSeries();
      CreateLevelData(node);
      const auto &coarseDataLevel = m_LineTypeLevelData[node][type].back();
      seriesLine->replace(coarseDataLevel);
    }

    seriesLine->setName((node->GetName()).c_str());
    chart->addSeries(seriesLine);
    color = seriesLine->color();
    SetDefaultLineSeriesStyle(seriesLine);

    // initialize sereis

    auto seriesScatter = m_ScatterSeries[node] = new QtCharts::QScatterSeries();
    chart->addSeries(seriesScatter);
    seriesScatter->setName("Peaks");
    seriesScatter->setColor(color);
    seriesScatter->hide();
    SetDefaultScatterSeriesStyle(seriesScatter);

    bool xTitleVisible = false;
    bool yTitleVisible = false;

    if (numSeries > 0)
    {
      xTitleVisible = m_xAxis->isTitleVisible();
      yTitleVisible = m_yAxis->isTitleVisible();
    }

    chart->createDefaultAxes();
    m_xAxis = static_cast<QtCharts::QValueAxis *>(chart->axes(Qt::Horizontal).front());
    m_yAxis = static_cast<QtCharts::QValueAxis *>(chart->axes(Qt::Vertical).front());
    QObject::connect(m_xAxis, SIGNAL(rangeChanged(qreal, qreal)), this, SLOT(OnRangeChangedAxisX(qreal, qreal)));
    QObject::connect(m_yAxis, SIGNAL(rangeChanged(qreal, qreal)), this, SLOT(OnRangeChangedAxisY(qreal, qreal)));

    if (numSeries == 0)
    {
      m_xAxis->setTitleVisible(false);
      m_yAxis->setTitleVisible(false);
    }
    else
    {
      m_xAxis->setTitleVisible(xTitleVisible);
      m_yAxis->setTitleVisible(yTitleVisible);
    }

    UpdateAxisLabels(node);
    UpdateSeriesMinMaxValues();
    OnResetView();

    const auto markers = chart->legend()->markers();
    for (auto *marker : markers)
    {
      QObject::connect(marker, SIGNAL(clicked()), this, SLOT(OnLegnedHandleMarkerClicked()), Qt::UniqueConnection);
    }

    OnAxisXTicksChanged(m_xAxisTicks);
    OnAxisYTicksChanged(m_yAxisTicks);
  }
}

void m2Spectrum::OnMouseMove(
  QPoint pos, qreal mz, qreal intValue, Qt::MouseButton /*button*/, Qt::KeyboardModifiers /*mod*/)
{
  const auto chart = m_Controls.chartView->chart();
  const auto chartPos = chart->mapToPosition(QPoint(mz, intValue));

  if (m_DraggingActive)
  {
    const auto xPos = chartPos.x();
    const auto xLastPos = chart->mapToPosition(QPoint(m_MouseDragCenterPos, 0)).x();
    const auto delta = xLastPos - xPos;
    chart->scroll(delta, 0);
  }

  // inside draw area
  if (!chart->series().empty())
  {
    if (((intValue > m_yAxis->min()) && (intValue < m_yAxis->max()) && (mz > m_xAxis->min()) && (mz < m_xAxis->max())))
    {
      m_Controls.chartView->repaint();
      m_Crosshair->setText(QString("%2 @ %1").arg(mz).arg(intValue, 0, 'e', 3));
      m_Crosshair->setAnchor(pos);

      m_Crosshair->setZValue(11);
      m_Crosshair->updateGeometry();
      m_Crosshair->show();
    }
    else
    {
      m_Crosshair->hide();
      m_Controls.chartView->repaint();
    }
  }
}

void m2Spectrum::OnMouseRelease(QPoint pos, qreal mz, qreal intValue, Qt::MouseButton button, Qt::KeyboardModifiers mod)
{
  Q_UNUSED(pos)
  Q_UNUSED(intValue)
  if (mod & Qt::AltModifier && m_RangeSelectionStarted)
  {
    m_SelectedAreaEndX = mz;
  }
  if (m_RangeSelectionStarted)
  {
    m_Controls.chartView->setRubberBand(QtCharts::QChartView::RubberBand::NoRubberBand);
    DrawSelectedArea();
    const auto mz = (m_SelectedAreaEndX + m_SelectedAreaStartX) * 0.5;
    const auto tol = std::abs(m_SelectedAreaEndX - m_SelectedAreaStartX) * 0.5;

    emit m2::UIUtils::Instance()->UpdateImage(mz, tol);
  }
  m_RangeSelectionStarted = false;
  m_Controls.chartView->setRubberBand(QtCharts::QChartView::RubberBand::NoRubberBand);

  if (button == Qt::MouseButton::MidButton)
  {
    m_DraggingActive = false;
  }
}

void m2Spectrum::OnMouseDoubleClick(
  QPoint pos, qreal mz, qreal intValue, Qt::MouseButton /*button*/, Qt::KeyboardModifiers /*mod*/)
{
  Q_UNUSED(pos)
  const unsigned numSeries = m_LineSeries.size() + m_ScatterSeries.size() + m_PeakSeries.size();
  if (!numSeries)
    return;

  if (intValue < m_yAxis->min())
  {
    // Double click below plot area (x-axis)
    m_xAxis->setRange(m_CurrentMinMZ, m_CurrentMaxMZ);
  }
  else if (mz < m_xAxis->min())
  {
    // Double click left to plot area (y-axis)
    m_yAxis->setRange(0, m_CurrentMaxIntensity);
  }
  else
  {
    emit m2::UIUtils::Instance()->UpdateImage(mz, -1);
  }
}

void m2Spectrum::OnMouseWheel(QPoint pos, qreal mz, qreal intValue, int angle, Qt::KeyboardModifiers mod)
{
  Q_UNUSED(pos)

  bool bothAxes =
    ((intValue > m_yAxis->min()) && (intValue < m_yAxis->max()) && (mz > m_xAxis->min()) && (mz < m_xAxis->max()));

  if (angle != 0)
  {
    double zoomFactor = 0.1;
    if (mod & Qt::KeyboardModifier::ControlModifier)
    {
      zoomFactor = 0.35;
    }

    if ((intValue < m_yAxis->min()) || bothAxes)
    {
      const int zoomDirection = angle / std::abs(angle);
      const auto d0 = std::abs(mz - m_xAxis->min()) * zoomFactor * zoomDirection;
      const auto d1 = std::abs(mz - m_xAxis->max()) * zoomFactor * zoomDirection;
      m_xAxis->setRange(m_xAxis->min() + d0, m_xAxis->max() - d1);
    }

    if ((mz < m_xAxis->min()) || (bothAxes && (mod == Qt::ShiftModifier)))
    {
      const int zoomDirection = angle / std::abs(angle);
      const auto d1 = m_yAxis->max() * (1 - zoomDirection * zoomFactor);
      m_yAxis->setMax(d1);
    }
  }
}

void m2Spectrum::UpdateSeriesMinMaxValues()
{
  m_CurrentMaxIntensity = std::numeric_limits<double>::min();
  m_CurrentMaxMZ = std::numeric_limits<double>::min();
  m_CurrentMinMZ = std::numeric_limits<double>::max();

  for (auto &kv : m_LineTypeLevelData)
  {
    const auto &ppp = kv.second[m_CurrentOverviewSpectrumType].front();
    auto m = std::max_element(ppp.begin(), ppp.end(), [](auto largest, auto p) { return p.y() > largest.y(); });
    if (m->y() > m_CurrentMaxIntensity)
      m_CurrentMaxIntensity = m->y();

    if (ppp.first().x() < m_CurrentMinMZ)
      m_CurrentMinMZ = ppp.first().x();
    if (ppp.back().x() > m_CurrentMaxMZ)
      m_CurrentMaxMZ = ppp.back().x();
  }

  for (auto &kv : m_PeakSeries)
  {
    auto ppp = kv.second->points();

    auto m = std::max_element(ppp.begin(), ppp.end(), [](auto largest, auto p) { return p.y() > largest.y(); });
    if (m->y() > m_CurrentMaxIntensity)
      m_CurrentMaxIntensity = m->y();

    if (ppp.first().x() < m_CurrentMinMZ)
      m_CurrentMinMZ = ppp.first().x();
    if (ppp.back().x() > m_CurrentMaxMZ)
      m_CurrentMaxMZ = ppp.back().x();
  }
}

void m2Spectrum::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);

  // 20201023: use communciation service
  auto serviceRef = m2::UIUtils::Instance();
  connect(serviceRef, SIGNAL(RangeChanged(qreal, qreal)), this, SLOT(OnMassRangeChanged(qreal, qreal)));
  connect(serviceRef,
          SIGNAL(SpectrumImageNodeAdded(const mitk::DataNode *)),
          this,
          SLOT(OnDataNodeReceived(const mitk::DataNode *)));
  connect(serviceRef,
          SIGNAL(OverviewSpectrumChanged(const mitk::DataNode *, m2::OverviewSpectrumType)),
          this,
          SLOT(OnOverviewSpectrumChanged(const mitk::DataNode *, m2::OverviewSpectrumType)));

  CreateQChartView();
  CreateQChartViewMenu();
}

void m2Spectrum::CreateQChartViewMenu()
{
  m_Menu = new QMenu(m_Controls.chartView);

  m_FocusMenu = new QMenu(m_Menu);
  m_FocusMenu->setTitle("Focus on ...");
  m_Menu->addMenu(m_FocusMenu);

  m_SpectrumSkyline = new QAction("Skyline spectrum", m_Controls.chartView);
  m_SpectrumSkyline->setCheckable(true);

  m_SpectrumMean = new QAction("Mean spectrum", m_Controls.chartView);
  m_SpectrumMean->setCheckable(true);
  m_SpectrumMean->setChecked(true);

  m_SpectrumSum = new QAction("Sum spectrum", m_Controls.chartView);
  m_SpectrumSum->setCheckable(true);

  m_ShowLegend = new QAction("Show legend", m_Controls.chartView);
  m_ShowLegend->setCheckable(true);

  m_ShowAxesTitles = new QAction("Show axes title", m_Controls.chartView);
  m_ShowAxesTitles->setCheckable(true);

  m_SpectrumSelectionGroup = new QActionGroup(m_Controls.chartView);
  m_SpectrumSelectionGroup->setExclusive(true);
  m_SpectrumSelectionGroup->addAction(m_SpectrumSkyline);
  m_SpectrumSelectionGroup->addAction(m_SpectrumSum);
  m_SpectrumSelectionGroup->addAction(m_SpectrumMean);

  connect(m_SpectrumSkyline,
          &QAction::toggled,
          this,
          [&](bool v)
          {
            if (v)
            {
              m_CurrentOverviewSpectrumType = m2::OverviewSpectrumType::Maximum;
              for (auto &kv : m_LineSeries)
                UpdateLineSeriesWindow(kv.first);
              for (auto &kv : m_ScatterSeries)
                OnUpdateScatterSeries(kv.first);
              m_yAxis->setRange(0, std::numeric_limits<qreal>::max());
            }
          });
  connect(m_SpectrumSum,
          &QAction::toggled,
          this,
          [&](bool v)
          {
            if (v)
            {
              m_CurrentOverviewSpectrumType = m2::OverviewSpectrumType::Sum;
              for (auto &kv : m_LineSeries)
                UpdateLineSeriesWindow(kv.first);
              for (auto &kv : m_ScatterSeries)
                OnUpdateScatterSeries(kv.first);
              m_yAxis->setRange(0, std::numeric_limits<qreal>::max());
            }
          });
  connect(m_SpectrumMean,
          &QAction::toggled,
          this,
          [&](bool v)
          {
            if (v)
            {
              m_CurrentOverviewSpectrumType = m2::OverviewSpectrumType::Mean;
              for (auto &kv : m_LineSeries)
                UpdateLineSeriesWindow(kv.first);
              for (auto &kv : m_ScatterSeries)
                OnUpdateScatterSeries(kv.first);
              m_yAxis->setRange(0, std::numeric_limits<qreal>::max());
            }
          });

  m_Menu->addAction(m_ShowLegend);
  m_Menu->addAction(m_ShowAxesTitles);

  connect(m_ShowAxesTitles,
          &QAction::toggled,
          this,
          [this](bool s)
          {
            MITK_INFO << m_yAxis << " " << m_xAxis;
            if (m_yAxis)
              m_yAxis->setTitleVisible(s);
            if (m_xAxis)
              m_xAxis->setTitleVisible(s);
          });

  m_ShowAxesTitles->setChecked(false);

  connect(
    m_ShowLegend, &QAction::toggled, this, [this](bool s) { m_Controls.chartView->chart()->legend()->setVisible(s); });

  m_ShowLegend->setChecked(true);

  m_Menu->addSection("Overview spectrum type");
  m_Menu->addAction(m_SpectrumSkyline);
  m_Menu->addAction(m_SpectrumSum);
  m_Menu->addAction(m_SpectrumMean);

  // auto sec = m_Menu->addSection("Tick control");
  m_TickCountX = new QSlider(m_Controls.chartView);
  m_TickCountX->setMinimum(3);
  m_TickCountX->setMaximum(50);
  m_TickCountX->setValue(9);
  m_TickCountX->setOrientation(Qt::Horizontal);

  m_TickCountY = new QSlider(m_Controls.chartView);
  m_TickCountY->setMinimum(3);
  m_TickCountY->setMaximum(10);
  m_TickCountY->setValue(4);
  m_TickCountY->setOrientation(Qt::Horizontal);

  auto wActionX = new QWidgetAction(m_Controls.chartView);
  wActionX->setDefaultWidget(m_TickCountX);
  connect(m_TickCountX, &QSlider::valueChanged, this, &m2Spectrum::OnAxisXTicksChanged);

  auto wActionXLabel = new QWidgetAction(m_Controls.chartView);
  wActionXLabel->setDefaultWidget(new QLabel("X axis ticks", m_Controls.chartView));

  auto wActionY = new QWidgetAction(m_Controls.chartView);
  wActionY->setDefaultWidget(m_TickCountY);
  connect(m_TickCountY, &QSlider::valueChanged, this, &m2Spectrum::OnAxisYTicksChanged);
  auto wActionYLabel = new QWidgetAction(m_Controls.chartView);
  wActionYLabel->setDefaultWidget(new QLabel("Y axis ticks", m_Controls.chartView));

  wActionY->setDefaultWidget(m_TickCountY);

  m_Menu->addAction(wActionXLabel);
  m_Menu->addAction(wActionX);

  m_Menu->addAction(wActionYLabel);
  m_Menu->addAction(wActionY);

  m_Controls.chartView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_Controls.chartView,
          &QtCharts::QChartView::customContextMenuRequested,
          this,
          [&](const QPoint &pos)
          {
            m_FocusMenu->clear();
            for (auto &kv : this->m_LineSeries)
            {
              auto action = new QAction(m_FocusMenu);
              m_FocusMenu->addAction(action);
              action->setText(kv.first->GetName().c_str());
              auto node = kv.first;
              connect(action, &QAction::triggered, this, [node, this]() { OnSeriesFocused(node); });
            }

            for (auto &kv : this->m_PeakSeries)
            {
              auto action = new QAction(m_FocusMenu);
              m_FocusMenu->addAction(action);
              action->setText(kv.first->GetName().c_str());
              auto node = kv.first;
              connect(action, &QAction::triggered, this, [node, this]() { OnSeriesFocused(node); });
            }

            m_Menu->exec(m_Controls.chartView->viewport()->mapToGlobal(pos));
          });
}

void m2Spectrum::CreateQChartView()
{
  m_SelectedAreaLeft = nullptr;
  m_SelectedAreaRight = nullptr;
  m_SelectedAreaLower = nullptr;
  m_SelectedAreaStartX = 0;
  m_SelectedAreaEndX = 0;

  auto chart = new QtCharts::QChart();
  m_Controls.chartView->setChart(chart);
  m_chart = chart;

  chart->legend()->setAlignment(Qt::AlignRight);
  chart->legend()->setShowToolTips(true);
  chart->setAnimationOptions(QtCharts::QChart::NoAnimation);
  chart->setTheme(QtCharts::QChart::ChartThemeDark);

  m_Crosshair = new m2Crosshair(chart);
  m_Crosshair->hide();

  connect(m_Controls.chartView, &m2::ChartView::mouseDoubleClick, this, &m2Spectrum::OnMouseDoubleClick);
  connect(m_Controls.chartView, &m2::ChartView::mousePress, this, &m2Spectrum::OnMousePress);
  connect(m_Controls.chartView, &m2::ChartView::mouseMove, this, &m2Spectrum::OnMouseMove);
  connect(m_Controls.chartView, &m2::ChartView::mouseRelease, this, &m2Spectrum::OnMouseRelease);
  connect(m_Controls.chartView, &m2::ChartView::mouseWheel, this, &m2Spectrum::OnMouseWheel);
}

void m2Spectrum::OnSeriesFocused(const mitk::DataNode *node)
{
  UpdateSeriesMinMaxValues();
  if (auto image = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    if (m_LineTypeLevelData.find(node) != std::end(m_LineTypeLevelData))
    {
      const auto points = m_LineTypeLevelData[node][m_CurrentOverviewSpectrumType].front();

      const auto max =
        std::max_element(
          std::begin(points), std::end(points), [](const auto &a, const auto &b) { return a.y() < b.y(); })
          ->y();
      if (m_yAxis)
        m_yAxis->setRange(0, 1.1 * max);
      if (m_xAxis)
        m_xAxis->setRange(image->GetXAxis().front(), image->GetXAxis().back());
    }
    else if (m_PeakSeries.find(node) != std::end(m_PeakSeries))
    {
      const auto points = m_PeakSeries[node]->pointsVector();

      const auto max =
        std::max_element(
          std::begin(points), std::end(points), [](const auto &a, const auto &b) { return a.y() < b.y(); })
          ->y();
      if (m_yAxis)
        m_yAxis->setRange(0, 1.1 * max);
      if (m_xAxis)
        m_xAxis->setRange(image->GetXAxis().front(), image->GetXAxis().back());
    }
  }
}

void m2Spectrum::NodeRemoved(const mitk::DataNode *node)
{
  if (dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    if (m_LineSeries.find(node) != std::end(m_LineSeries))
    {
      m_Controls.chartView->chart()->removeSeries(m_LineSeries[node]);
      m_LineSeries.erase(node);
    }

    if (m_LineTypeLevelData.find(node) != std::end(m_LineTypeLevelData))
    {
      m_LineTypeLevelData.erase(node);
    }

    if (m_PeakSeries.find(node) != std::end(m_PeakSeries))
    {
      m_Controls.chartView->chart()->removeSeries(m_PeakSeries[node]);
      m_PeakSeries.erase(node);
    }

    if (m_ScatterSeries.find(node) != std::end(m_ScatterSeries))
    {
      m_Controls.chartView->chart()->removeSeries(m_ScatterSeries[node]);
      m_ScatterSeries.erase(node);
    }

    UpdateAxisLabels(node, true);
  }
}

void m2Spectrum::OnLegnedHandleMarkerClicked()
{
  auto *marker = qobject_cast<QtCharts::QLegendMarker *>(sender());
  Q_ASSERT(marker);

  // switch (marker->type())
  marker->series()->setVisible(!marker->series()->isVisible());
  marker->setVisible(true);

  // Dim the marker, if series is not visible
  qreal alpha = 1.0;

  if (!marker->series()->isVisible())
    alpha = 0.5;

  QColor color;
  QBrush brush = marker->labelBrush();
  color = brush.color();
  color.setAlphaF(alpha);
  brush.setColor(color);
  marker->setLabelBrush(brush);

  brush = marker->brush();
  color = brush.color();
  color.setAlphaF(alpha);
  brush.setColor(color);
  marker->setBrush(brush);

  QPen pen = marker->pen();
  color = pen.color();
  color.setAlphaF(alpha);
  pen.setColor(color);
  marker->setPen(pen);
}

void m2Spectrum::OnResetView()
{
  m_xAxis->setMax(m_CurrentMaxMZ);
  m_xAxis->setMin(m_CurrentMinMZ);
  m_yAxis->setMax(m_CurrentMaxIntensity * 1.1);
  m_yAxis->setMin(0);
  m_Controls.chartView->repaint();
}

void m2Spectrum::OnAxisXTicksChanged(int v)
{
  m_xAxisTicks = v;
  if (m_xAxis)
  {
    m_xAxis->setTickCount(m_xAxisTicks);
    m_xAxis->setMinorTickCount(1);
    m_xAxis->setLabelsAngle(0);
    auto col = m_xAxis->gridLineColor();
    col.setAlphaF(0.3);
    m_xAxis->setGridLineColor(col);
    col.setAlphaF(0.15);
    m_xAxis->setMinorGridLineColor(col);
  }
}

void m2Spectrum::OnAxisYTicksChanged(int v)
{
  m_yAxisTicks = v;
  if (m_yAxis)
  {
    m_yAxis->setTickCount(m_yAxisTicks);
    m_yAxis->setMinorTickCount(1);
    auto col = m_yAxis->gridLineColor();
    col.setAlphaF(0.3);
    m_yAxis->setGridLineColor(col);
    col.setAlphaF(0.15);
    m_yAxis->setMinorGridLineColor(col);
    // yAxis->setTitleText("Intensity");
  }
}

void m2Spectrum::OnRangeChangedAxisX(qreal min, qreal max)
{
  auto axis = m_xAxis;
  axis->blockSignals(true);
  if (max > m_CurrentMaxMZ)
  {
    this->m_MouseDragUpperDelta = std::abs(m_CurrentMaxMZ - this->m_MouseDragCenterPos);
    axis->setMax(m_CurrentMaxMZ);
  }

  if (min < m_CurrentMinMZ)
  {
    this->m_MouseDragLowerDelta = std::abs(m_CurrentMinMZ - this->m_MouseDragCenterPos);
    axis->setMin(m_CurrentMinMZ);
  }

  // check for each line series if a finer resolution should be reloaded
  for (auto &kv : m_LineSeries)
  {
    UpdateZoomLevel(kv.first);
    UpdateLineSeriesWindow(kv.first);
  }

  axis->blockSignals(false);

  // #25 auto zoom y axis on zoom x axis

  berry::IPreferences::Pointer preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");
  if (preferences->GetBool("useMaxIntensity", false) || preferences->GetBool("useMinIntensity", false))
  {
    double maxY = 0;
    double minY = std::numeric_limits<double>::max();
    for (auto &kv : m_LineSeries)
    {
      auto points = kv.second->points();
      if (points.empty())
        continue;
      auto p = points.begin();
      // start of window
      while (p != points.end() && p->x() < min)
        ++p;
      while (p != points.end() && p->x() <= max)
      {
        maxY = std::max(maxY, p->y());
        minY = std::min(minY, p->y());
        ++p;
      }
    }

    for (auto &kv : m_PeakSeries)
    {
      auto points = kv.second->points();
      if (points.empty())
        continue;
      auto p = points.begin();
      // start of window
      while (p != points.end() && p->x() < min)
        ++p;
      while (p != points.end() && p->x() <= max)
      {
        maxY = std::max(maxY, p->y());
        minY = std::max(minY, p->y());
        ++p;
      }
    }
    if (preferences->GetBool("useMaxIntensity", false))
    {
      m_yAxis->setMax(maxY * 1.1);
    }
    if (preferences->GetBool("useMinIntensity", false))
    {
      m_yAxis->setMin(minY * 0.9);
    }
  }
}

void m2Spectrum::OnRangeChangedAxisY(qreal min, qreal max)
{
  auto axis = m_yAxis;
  axis->blockSignals(true);
  if (max > m_CurrentMaxIntensity)
  { // calculated in ExclusiveShow(type)
    axis->setMax(m_CurrentMaxIntensity * 1.1);
  }

  if (min < 0)
  {
    axis->setMin(0);
  }
  UpdateSelectedArea();

  axis->blockSignals(false);
}

void m2Spectrum::OnUpdateScatterSeries(const mitk::DataNode *node)
{
  auto img = dynamic_cast<m2::SpectrumImageBase *>(node->GetData());
  const auto &peaks = img->GetPeaks();
  const auto &data = m_LineTypeLevelData[node][m_CurrentOverviewSpectrumType][0];

  QVector<QPointF> qPeaks;
  std::transform(std::begin(peaks),
                 std::end(peaks),
                 std::back_inserter(qPeaks),
                 [&data](const auto &a) -> QPointF {
                   return {a.GetX(), data[a.GetIndex()].y()};
                 });

  m_ScatterSeries[node]->clear();
  m_ScatterSeries[node]->replace(qPeaks);
  if (peaks.empty())
    m_ScatterSeries[node]->hide();
  else
    m_ScatterSeries[node]->show();
}

void m2Spectrum::UpdateLineSeriesWindow(const mitk::DataNode *node)
{
  if (node)
  {
    const auto &typeLevelData = m_LineTypeLevelData[node][m_CurrentOverviewSpectrumType];
    const auto &d = typeLevelData.at(m_Level[node]);

    const auto &rawData = typeLevelData[0];
    const auto windowXMin = std::max(m_xAxis->min(), rawData.front().x());
    const auto windowXMax = std::min(m_xAxis->max(), rawData.back().x());

    auto lb = std::lower_bound(
      std::begin(d), std::end(d), windowXMin, [](const auto &p, const auto &val) { return p.x() < val; });

    auto ub = std::upper_bound(
      std::begin(d), std::end(d), windowXMax, [](const auto &p, const auto &val) { return val.x() > p; });

    QVector<QPointF> points;
    std::copy(lb, ub, std::back_inserter(points));

    m_LineSeries[node]->replace(points);

    // update max intensity
    this->m_CurrentMaxIntensity =
      std::max_element(points.begin(), points.end(), [](const auto &a, const auto &b) { return a.y() < b.y(); })->y();
  }
}

void m2Spectrum::UpdateAxisLabels(const mitk::DataNode *node, bool remove)
{
  MITK_INFO << "Update axis";

  if (auto image = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    auto label = image->GetPropertyValue<std::string>("x_label");
    if (!m_xAxisTitels.contains(label.c_str()))
      m_xAxisTitels.push_back(label.c_str());
    else if (remove)
    {
      m_xAxisTitels.remove(m_xAxisTitels.indexOf(label.c_str()));
    }
  }

  QString label("");
  for (const auto &l : m_xAxisTitels)
  {
    label.append(l + "; ");
  }

  m_xAxis->setTitleText(label);
  switch (m_CurrentOverviewSpectrumType)
  {
    case m2::OverviewSpectrumType::Maximum:
      m_yAxis->setTitleText("Intensity (maximum)");
      break;
    case m2::OverviewSpectrumType::Sum:
      m_yAxis->setTitleText("Intensity (sum)");
      break;
    case m2::OverviewSpectrumType::Mean:
      m_yAxis->setTitleText("Intensity (mean)");
      break;
    default:
      break;
  }
}

void m2Spectrum::UpdateZoomLevel(const mitk::DataNode *node)
{
  auto image = dynamic_cast<m2::SpectrumImageBase *>(node->GetData());
  const auto &xs = image->GetXAxis();
  // determine how many points of the original data are visible
  const auto axesMin = m_xAxis->min();
  const auto axesMax = m_xAxis->max();
  using namespace std;
  auto s = std::upper_bound(begin(xs), end(xs), axesMin);
  auto e = std::upper_bound(begin(xs), end(xs), axesMax);
  auto p = std::distance(s, e);

  if (p < 5000)
    m_Level[node] = 0;
  else if (p < 8000)
    m_Level[node] = 1;
  else if (p < 16000)
    m_Level[node] = 2;
  else if (p < 40000)
    m_Level[node] = 3;
  else
    m_Level[node] = 4;

  // MITK_INFO << "Level " << m_Level[node] << " " << m_CurrentVisibleDataPoints;
}
