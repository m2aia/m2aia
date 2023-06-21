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

#include <iostream>
#include <memory>

// Qt includes
// #include <QApplication>
#include <QColor>
#include <QGraphicsSimpleTextItem>
#include <QLabel>
#include <QMenu>
#include <QRandomGenerator>
#include <QShortcut>
#include <QValueAxis>
#include <QWidgetAction>
#include <QXYSeries>

// MITK includes
#include <mitkColorProperty.h>
#include <mitkCoreServices.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>
#include <mitkLookupTableProperty.h>
#include <mitkStatusBar.h>

// M2aia includes
#include <m2ImzMLSpectrumImage.h>
#include <m2UIUtils.h>
#include <signal/m2PeakDetection.h>

// internal includes
#include "m2SeriesDataProvider.h"

const std::string m2Spectrum::VIEW_ID = "org.mitk.views.m2.Spectrum";

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

    m_NodeRealtedGraphicItems.clear();

    using namespace std;
    double size = 5;
    for (auto kv1 : m_DataProvider)
    {
      try
      {
        auto p = kv1.second->GetActiveProvider().lock();
        auto lowerX = lower_bound(begin(p->xs()), end(p->xs()), std::max(m_SelectedAreaStartX, m_xAxis->min()));
        auto upperX = upper_bound(lowerX, end(p->xs()), std::min(m_SelectedAreaEndX, m_xAxis->max()));
        auto lowerY = begin(p->ys()) + distance(begin(p->xs()), lowerX);

        for (; lowerX < upperX; ++lowerX, ++lowerY)
        {
          auto rect = std::make_shared<QGraphicsRectItem>();
          if (auto prop = kv1.first->GetProperty("spectrum.marker.size"))
          {
            if (auto sizeProp = dynamic_cast<mitk::IntProperty *>(prop))
            {
              size = sizeProp->GetValue();
            }
          }
          auto itemPos = m_chart->mapToPosition(QPointF(*lowerX, *lowerY));
          rect->setRect(itemPos.x() - size / 2.0, itemPos.y() - size / 2.0, size, size);

          if (auto prop = kv1.first->GetProperty("spectrum.marker.color"))
          {
            if (auto colorProp = dynamic_cast<mitk::ColorProperty *>(prop))
            {
              auto mc = colorProp->GetColor();
              QColor c;
              c.setRgbF(mc.GetRed(), mc.GetGreen(), mc.GetBlue());
              rect->setBrush(QBrush(c));
              rect->setPen(QPen(c));
            }
          }

          rect->setZValue(20);

          m_NodeRealtedGraphicItems[kv1.first].push_back(rect);
        }
      }
      catch (std::exception &e)
      {
        MITK_ERROR << e.what();
      }
    }
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

  UpdateSelectedArea();
  // MITK_INFO("m2Spectrum::OnMousePress") << "Mouse Pressed";
}

void m2Spectrum::OnMassRangeChanged(qreal x, qreal tol)
{
  SetSelectedAreaStartX(x - tol);
  SetSelectedAreaEndX(x + tol);
  DrawSelectedArea();
}

// void m2Spectrum::OnSpectrumArtifactChanged(const mitk::DataNode * node, m2::SpectrumType type){

//   if(dynamic_cast<m2::SpectrumImageBase *>(node->GetData())){
//     if(type == m2::SpectrumType::None){
//       if(m_DataProvider[node]->Exists("Peaks")){
//         m_DataProvider[node]->UpdateGroup(pea)
//       }
//     }
//   }

// }

void m2Spectrum::OnPeakListChanged(const itk::Object *, const itk::EventObject &)
{
  UpdateAllSeries();
  UpdateCurrentMinMaxY();
}

void m2Spectrum::OnPropertyListChanged(const itk::Object *caller, const itk::EventObject &)
{
  if (auto node = dynamic_cast<const mitk::DataNode *>(caller))
  {
    if (auto genericProperty = node->GetProperty("spectrum.plot.color"))
    {
      if (auto colorProperty = dynamic_cast<mitk::ColorProperty *>(genericProperty))
      {
        auto c = colorProperty->GetColor();

        for (auto kv : m_DataProvider[node]->GetProviders())
        {
          kv.second->SetColor(c.GetRed(), c.GetGreen(), c.GetBlue());
        }
      }
    }

    if (auto genericProperty = node->GetProperty("spectrum.marker.color"))
    {
      if (auto colorProperty = dynamic_cast<mitk::ColorProperty *>(genericProperty))
      {
        auto mc = colorProperty->GetColor();

        for (auto item : m_NodeRealtedGraphicItems[node])
        {
          QColor c;
          c.setRgbF(mc.GetRed(), mc.GetGreen(), mc.GetBlue());
          auto rect = dynamic_cast<QGraphicsRectItem *>(item.get());
          rect->setBrush(QBrush(c));
          rect->setPen(QPen(c));
        }
      }
    }

    if (auto prop = node->GetProperty("spectrum.marker.size"))
    {
      if (auto sizeProp = dynamic_cast<mitk::IntProperty *>(prop))
      {
        auto size = sizeProp->GetValue();
        for (auto item : m_NodeRealtedGraphicItems[node])
        {
          auto rect = dynamic_cast<QGraphicsRectItem *>(item.get());
          auto itemPos = rect->rect().center();
          rect->setRect(itemPos.x() - size / 2.0, itemPos.y() - size / 2.0, size, size);
        }
      }
    }

    // std::string idString;
    //   if (node->GetStringProperty("m2.singlespectrum.id", idString))
    //   {
    //     MITK_INFO << "Called " << idString;
    //     if (auto specImage = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
    //     {
    //       if (specImage->GetSpectrumType().Format == m2::SpectrumFormat::ContinuousProfile)
    //       {
    //         auto &provider = m_DataProvider[node];
    //         QtCharts::QXYSeries *series;
    //         series = provider->GetGroupSeries("SingleSpectrum");
    //         if (idString.empty())
    //         {
    //           provider->SetGroupVisiblity("SingleSpectrum", false);
    //           series->clear();
    //           return;
    //         }
    //         auto id = std::stoul(idString);
    //         std::vector<double> ys;
    //         specImage->GetYValues(id, ys);
    //         const auto &xs = specImage->GetXAxis();

    //         MITK_INFO << "Provide Data " << xs.size() << " " << ys.size();
    //         QVector<QPointF> seriesData;
    //         auto insert = std::back_inserter(seriesData);
    //         auto xIt = std::cbegin(xs);
    //         for (auto yIt = std::cbegin(ys); yIt != std::cend(ys); ++yIt, ++xIt)
    //           insert = QPointF{*xIt, *yIt};

    //         series->replace(seriesData);
    //         provider->SetGroupVisiblity("SingleSpectrum", true);
    //       }
    //     }
    //   }
  }
}

void m2Spectrum::OnInitializationFinished(const itk::Object * /*caller*/, const itk::EventObject & /*event*/)
{
  // if (auto node = dynamic_cast<const mitk::DataNode *>(caller))
  // {

  // }
}

void m2Spectrum::OnDataNodeReceived(const mitk::DataNode *node)
{
  if (!node)
    return;

  m_chart = m_Controls.chartView->chart();
  if (auto intervals = dynamic_cast<m2::IntervalVector *>(node->GetData()))
  {
    if (m_DataProvider.empty())
    {
      m_GlobalMinimumX = m_LocalMinimumX = intervals->GetXMean().front();
      m_GlobalMaximumX = m_LocalMaximumX = intervals->GetXMean().back();
    }

    // Overview spectra
    auto provider = std::make_shared<m2::SeriesDataProvider>();
    provider->Initialize(intervals->GetXMean(), intervals->GetYMean(), m2::SeriesDataProvider::Format::centorid);
    m_chart->addSeries(provider->GetSeries());
    provider->GetSeries()->setName((node->GetName() + " Mean_peaks").c_str());
    provider->GetSeries()->setVisible(m_CurrentOverviewSpectrumType == m2::SpectrumType::Mean);
    m_DataProvider[node]->AddProvider("Mean", provider);

    provider = std::make_shared<m2::SeriesDataProvider>();
    provider->Initialize(intervals->GetXMean(), intervals->GetYMax(), m2::SeriesDataProvider::Format::centorid);
    m_chart->addSeries(provider->GetSeries());
    provider->GetSeries()->setName((node->GetName() + " Max_peaks").c_str());
    provider->GetSeries()->setVisible(m_CurrentOverviewSpectrumType == m2::SpectrumType::Maximum);
    m_DataProvider[node]->AddProvider("Max", provider);
  }

  if (auto imageBase = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    QColor color = QColor::fromHsv(QRandomGenerator::global()->generateDouble() * 255, 255, 255);
    node->GetPropertyList()->SetProperty("spectrum.plot.color",
                                         mitk::ColorProperty::New(color.redF(), color.greenF(), color.blueF()));
    node->GetPropertyList()->SetProperty("spectrum.marker.color",
                                         mitk::ColorProperty::New(color.redF(), color.greenF(), color.blueF()));
    node->GetPropertyList()->SetProperty("spectrum.marker.size", mitk::IntProperty::New(2));

    auto command = itk::NodeMemberCommand<m2Spectrum>::New();
    command->SetNode(node);
    command->SetCallbackFunction(this, &m2Spectrum::OnPropertyListChanged);
    m_NodeObserverTags[node].push_back(node->AddObserver(itk::ModifiedEvent(), command));

    // command = itk::NodeMemberCommand<m2Spectrum>::New();
    // command->SetNode(node);
    // command->SetCallbackFunction(this, &m2Spectrum::OnInitializationFinished);
    // imageBase->AddObserver(m2::InitializationFinishedEvent(), command);

    // node->GetPropertyList()->SetStringProperty("m2.singlespectrum.id", "");
    // auto prop = node->GetPropertyList()->GetProperty("m2.singlespectrum.id");
    // auto propertyModifiedCommand = itk::NodeMemberCommand<m2Spectrum>::New();
    // propertyModifiedCommand->SetNode(node);
    // propertyModifiedCommand->SetCallbackFunction(this, &m2Spectrum::OnPropertyChanged);
    // prop->AddObserver(itk::ModifiedEvent(), propertyModifiedCommand);

    // auto provider = std::make_shared<Qm2SpectrumChartDataProvider>(node);
    // m_DataProvider.emplace(node, provider);

    if (m_DataProvider.empty())
    {
      m_GlobalMinimumX = m_LocalMinimumX = imageBase->GetXAxis().front();
      m_GlobalMaximumX = m_LocalMaximumX = imageBase->GetXAxis().back();
    }

    auto helper = std::make_shared<DataProviderHelper>();

    m2::SeriesDataProvider::Format format;
    if (imageBase->GetSpectrumType().Format == m2::SpectrumFormat::ContinuousCentroid ||
        imageBase->GetSpectrumType().Format == m2::SpectrumFormat::ContinuousCentroid)
    {
      format = m2::SeriesDataProvider::Format::centorid;
    }
    else
    {
      format = m2::SeriesDataProvider::Format::profile;
    }
    // Overview spectra
    auto provider = std::make_shared<m2::SeriesDataProvider>();
    provider->Initialize(imageBase->GetXAxis(), imageBase->MeanSpectrum(), format);
    provider->SetColor(color);
    provider->GetSeries()->setName((node->GetName() + " Mean").c_str());
    provider->GetSeries()->setVisible(m_CurrentOverviewSpectrumType == m2::SpectrumType::Mean);

    helper->AddProvider("Mean", provider);
    m_chart->addSeries(provider->GetSeries());

    provider = std::make_shared<m2::SeriesDataProvider>();
    provider->Initialize(imageBase->GetXAxis(), imageBase->SkylineSpectrum(), format);
    provider->SetColor(color);
    provider->GetSeries()->setName((node->GetName() + " Max").c_str());
    provider->GetSeries()->setVisible(m_CurrentOverviewSpectrumType == m2::SpectrumType::Maximum);

    helper->AddProvider("Max", provider);
    m_chart->addSeries(provider->GetSeries());

    helper->SetActiveProviderTo("Mean");
    m_DataProvider.emplace(node, helper);
    // {
    //   auto series = provider->MakeSeries<QtCharts::QLineSeries>("SingleSpectrum");
    //   provider->Register("SingleSpectrum", m2::SpectrumType::None);
    //   series->setName((node->GetName() + " Single").c_str());
    //   chart->addSeries(series);
    //   provider->SetGroupVisiblity("SingleSpectrum", false);
    // }

    // { // Peak indicators
    //   auto scatterSeries = provider->MakeSeries<QtCharts::QScatterSeries>("Peaks");
    //   provider->Register("Peaks", m2::SpectrumType::Maximum);
    //   provider->Register("Peaks", m2::SpectrumType::Mean);
    //   scatterSeries->setName((node->GetName() + " Peaks").c_str());
    //   provider->UpdateGroup("Peaks");
    //   provider->SetGroupVisiblity("Peaks", true);
    //   chart->addSeries(scatterSeries);
    // }
  }

  m_chart->createDefaultAxes();
  m_xAxis = static_cast<QtCharts::QValueAxis *>(m_chart->axes(Qt::Horizontal).front());
  m_yAxis = static_cast<QtCharts::QValueAxis *>(m_chart->axes(Qt::Vertical).front());
  QObject::connect(m_xAxis, SIGNAL(rangeChanged(qreal, qreal)), this, SLOT(OnRangeChangedAxisX(qreal, qreal)));
  QObject::connect(m_yAxis, SIGNAL(rangeChanged(qreal, qreal)), this, SLOT(OnRangeChangedAxisY(qreal, qreal)));

  // const auto markers = chart->legend()->markers();
  // for (auto *marker : markers)
  //   QObject::connect(marker, SIGNAL(clicked()), this, SLOT(OnLegnedHandleMarkerClicked()), Qt::UniqueConnection);

  OnAxisXTicksChanged(m_xAxisTicks);
  OnAxisYTicksChanged(m_yAxisTicks);

  UpdateAllSeries();
  OnResetView();
  AutoZoomUseLocalExtremaY();

  //   bool isCentroidSpectrum = false;

  //   isCentroidSpectrum = any(imageBase->GetSpectrumType().Format &
  //                            (m2::SpectrumFormat::ContinuousCentroid | m2::SpectrumFormat::ProcessedCentroid));

  //   const unsigned numSeries = m_LineSeries.size() + m_ScatterSeries.size() + m_PeakSeries.size();

  //   QColor color;
  //   QtCharts::QLineSeries *seriesLine;
  //   const auto &type = m_CurrentOverviewSpectrumType;
  //   if (isCentroidSpectrum)
  //   {
  //     seriesLine = m_PeakSeries[node] = new QtCharts::QLineSeries();
  //     color = seriesLine->color();
  //     CreatePeakData(node);
  //     seriesLine->replace(m_PeakTypeData[node][type]);
  //   }
  //   else
  //   {
  //     // multi map insertion order dependent, add two line series for each node
  //     // the following ordering will not change (since c++11)
  //     seriesLine = m_LineSeries.emplace(node, new QtCharts::QLineSeries())->second; // overview spectrum
  //     color = seriesLine->color();
  //     auto singleSpectrum =
  //       m_LineSeries.emplace(node, new QtCharts::QLineSeries())->second; // single spectrum selection

  //     CreateLevelData(node);
  //     const auto &coarseDataLevel = m_LineTypeLevelData[node][type].back();
  //     seriesLine->replace(coarseDataLevel);

  //     singleSpectrum->setName("Single Spectrum");
  //     singleSpectrum->setColor(color);
  //     auto b = singleSpectrum->pen();
  //     b.setStyle(Qt::PenStyle::DashLine);
  //     singleSpectrum->hide();
  //     chart->addSeries(singleSpectrum);
  //   }

  //   // seriesLine->setName((node->GetName()).c_str());
  //   // chart->addSeries(seriesLine);
  //   // SetDefaultLineSeriesStyle(seriesLine);

  //   // initialize sereis

  //   auto seriesScatter = m_ScatterSeries[node] = new QtCharts::QScatterSeries();
  //   chart->addSeries(seriesScatter);
  //   seriesScatter->setName("Peaks");
  //   seriesScatter->setColor(color);
  //   seriesScatter->hide();
  //   SetDefaultScatterSeriesStyle(seriesScatter);

  //   bool xTitleVisible = false;
  //   bool yTitleVisible = false;

  //   if (numSeries > 0)
  //   {
  //     xTitleVisible = m_xAxis->isTitleVisible();
  //     yTitleVisible = m_yAxis->isTitleVisible();
  //   }

  //   chart->createDefaultAxes();
  //   m_xAxis = static_cast<QtCharts::QValueAxis *>(chart->axes(Qt::Horizontal).front());
  //   m_yAxis = static_cast<QtCharts::QValueAxis *>(chart->axes(Qt::Vertical).front());
  //   QObject::connect(m_xAxis, SIGNAL(rangeChanged(qreal, qreal)), this, SLOT(OnRangeChangedAxisX(qreal, qreal)));
  //   QObject::connect(m_yAxis, SIGNAL(rangeChanged(qreal, qreal)), this, SLOT(OnRangeChangedAxisY(qreal, qreal)));

  //   if (numSeries == 0)
  //   {
  //     m_xAxis->setTitleVisible(false);
  //     m_yAxis->setTitleVisible(false);
  //   }
  //   else
  //   {
  //     m_xAxis->setTitleVisible(xTitleVisible);
  //     m_yAxis->setTitleVisible(yTitleVisible);
  //   }

  //   UpdateAxisLabels(node);
  //   UpdateSeriesMinMaxValues();
  //   OnResetView();

  // }
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
      m_Crosshair->setPos(pos + QPoint(10, -30));

      m_Crosshair->setZValue(11);
      // m_Crosshair->updateGeometry();
      m_Crosshair->show();
    }
    else
    {
      m_Crosshair->hide();
      m_Controls.chartView->repaint();
    }
  }

  UpdateSelectedArea();

  // MITK_INFO("m2Spectrum::OnMouseMove") << "Mouse Pressed";
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
  QPoint pos, qreal xVal, qreal yVal, Qt::MouseButton /*button*/, Qt::KeyboardModifiers /*mod*/)
{
  Q_UNUSED(pos)

  if (m_DataProvider.empty())
    return;

  // reset axis by double click below/left of the chart plot area
  if (yVal < m_yAxis->min()) // rest x
    m_xAxis->setRange(m_GlobalMinimumX, m_GlobalMaximumX);
  else if (xVal < m_xAxis->min())
    m_yAxis->setRange(0, m_GlobalMaximumY * 1.1); // reset y
  else if (xVal > m_xAxis->max())
  {
  } // do nothing
  else
    emit m2::UIUtils::Instance()->UpdateImage(xVal, -1);
}

void m2Spectrum::OnMouseWheel(QPoint pos, qreal x, qreal y, int angle, Qt::KeyboardModifiers mod)
{
  Q_UNUSED(pos)

  if (m_DataProvider.empty())
  {
    return;
  }

  const auto modifiedMin = m_yAxis->min() * 0.9;
  const auto modifiedMax = m_yAxis->max() * 1.1;
  bool bothAxes = ((y > modifiedMin) && (y < modifiedMax) && (x > m_xAxis->min()) && (x < m_xAxis->max()));

  if (angle != 0)
  {
    double zoomFactor = 0.1;
    if (mod & Qt::KeyboardModifier::ControlModifier)
    {
      zoomFactor = 0.35;
    }

    if ((y < modifiedMin) || bothAxes)
    {
      const int zoomDirection = angle / std::abs(angle);
      const auto d0 = std::abs(x - m_xAxis->min()) * zoomFactor * zoomDirection;
      const auto d1 = std::abs(x - m_xAxis->max()) * zoomFactor * zoomDirection;
      m_xAxis->setRange(m_xAxis->min() + d0, m_xAxis->max() - d1);
    }

    if ((x < m_xAxis->min()) || (bothAxes && (mod == Qt::ShiftModifier)))
    {
      const int zoomDirection = angle / std::abs(angle);
      const auto d1 = m_yAxis->max() * (1 - zoomDirection * zoomFactor);
      m_yAxis->setMax(d1);
    }
  }

  UpdateSelectedArea();
}

void m2Spectrum::UpdateCurrentMinMaxY()
{
  m_LocalMinimumY = std::numeric_limits<double>::max();
  m_LocalMaximumY = std::numeric_limits<double>::min();

  for (auto &kv1 : m_DataProvider)
  {
    for (auto &kv2 : kv1.second->GetProviders())
    {
      if (!kv2.second->GetSeries()->isVisible() || kv2.second->xs().empty())
        continue;

      auto points = kv2.second->GetSeries()->points();
      auto minmax =
        std::minmax_element(points.begin(), points.end(), [](const auto &a, const auto &b) { return a.y() < b.y(); });

      m_LocalMinimumY = std::min(m_LocalMinimumY, minmax.first->y());
      m_LocalMaximumY = std::max(m_LocalMaximumY, minmax.second->y());
    }
  }

  // MITK_INFO("UpdateLocalMinMaxValues") << minX << " " << maxX;
  // MITK_INFO("UpdateLocalMinMaxValues") << "New local minmax Y: " << m_LocalMinimumY << " " << m_LocalMaximumY;
}

void m2Spectrum::UpdateGlobalMinMaxValues()
{
  if (m_DataProvider.empty())
    return;

  m_GlobalMinimumY = std::numeric_limits<double>::max();
  m_GlobalMaximumY = std::numeric_limits<double>::min();

  for (const auto &kv1 : m_DataProvider)
  {
    for (const auto &kv2 : kv1.second->GetProviders())
    {
      if (!kv2.second->GetSeries()->isVisible() && kv2.second->xs().empty())
        continue;

      m_GlobalMinimumX = std::min(m_GlobalMinimumX, kv2.second->xs().front());
      m_GlobalMaximumX = std::max(m_GlobalMaximumX, kv2.second->xs().back());
      const auto points = kv2.second->GetSeries()->points();
      const auto cmp = [](const auto &a, const auto &b) { return a.y() < b.y(); };
      const auto minmaxY = std::minmax_element(std::begin(points), std::end(points), cmp);
      m_GlobalMinimumY = std::min(m_GlobalMinimumY, minmaxY.first->y());
      m_GlobalMaximumY = std::max(m_GlobalMaximumY, minmaxY.second->y());
    }
  }

  auto useMinIntensity = m_M2aiaPreferences->GetBool("useMinIntensity", true);
  if (!useMinIntensity)
    m_GlobalMinimumY = 0;

  // m_LocalMinimumY = m_GlobalMinimumY;
  // m_LocalMaximumY = m_GlobalMaximumY;

  // MITK_INFO("UpdateGlobalMinMaxValues") << "New global minmax X: " << m_GlobalMinimumX << " " << m_GlobalMaximumX;
  // MITK_INFO("UpdateGlobalMinMaxValues") << "New global minmax Y: " << m_GlobalMinimumY << " " << m_GlobalMaximumY;
  // MITK_INFO("UpdateGlobalMinMaxValues") << "New local minmax Y: " << m_LocalMinimumY << " " << m_LocalMaximumY;
}

void m2Spectrum::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);

  QShortcut *shortcutLeft = new QShortcut(QKeySequence(Qt::Key_Left), parent);
  QShortcut *shortcutRight = new QShortcut(QKeySequence(Qt::Key_Right), parent);
  QShortcut *shortcutUp = new QShortcut(QKeySequence(Qt::Key_Up), parent);
  QShortcut *shortcutDown = new QShortcut(QKeySequence(Qt::Key_Down), parent);

  auto UIUtilsObject = m2::UIUtils::Instance();

  connect(shortcutRight, SIGNAL(activated()), UIUtilsObject, SIGNAL(NextImage()));
  connect(shortcutLeft, SIGNAL(activated()), UIUtilsObject, SIGNAL(PreviousImage()));
  connect(shortcutUp, SIGNAL(activated()), UIUtilsObject, SIGNAL(IncreaseTolerance()));
  connect(shortcutDown, SIGNAL(activated()), UIUtilsObject, SIGNAL(DecreaseTolerance()));

  connect(UIUtilsObject, SIGNAL(RangeChanged(qreal, qreal)), this, SLOT(OnMassRangeChanged(qreal, qreal)));
  connect(UIUtilsObject,
          SIGNAL(SpectrumImageNodeAdded(const mitk::DataNode *)),
          this,
          SLOT(OnDataNodeReceived(const mitk::DataNode *)));
  // connect(serviceRef,
  //         SIGNAL(OverviewSpectrumChanged(const mitk::DataNode *, m2::SpectrumType)),
  //         this,
  //         SLOT(OnSpectrumArtifactChanged(const mitk::DataNode *, m2::SpectrumType)));

  CreateQChartView();
  CreateQChartViewMenu();

  m_M2aiaPreferences =
    mitk::CoreServices::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");

  m_Controls.chartView->chart()->legend()->setVisible(false);
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

  m_ShowLegend = new QAction("Show legend", m_Controls.chartView);
  m_ShowLegend->setCheckable(false);

  m_ShowAxesTitles = new QAction("Show axes title", m_Controls.chartView);
  m_ShowAxesTitles->setCheckable(true);

  m_SpectrumSelectionGroup = new QActionGroup(m_Controls.chartView);
  m_SpectrumSelectionGroup->setExclusive(true);
  m_SpectrumSelectionGroup->addAction(m_SpectrumSkyline);
  m_SpectrumSelectionGroup->addAction(m_SpectrumMean);

  connect(m_SpectrumSkyline,
          &QAction::toggled,
          this,
          [&](bool v)
          {
            if (!v)
              return;

            for (auto &kv : this->m_DataProvider)
              kv.second->SetActiveProviderTo("Max");

            UpdateGlobalMinMaxValues();
            UpdateCurrentMinMaxY();
            emit m_xAxis->rangeChanged(m_xAxis->min(), m_xAxis->max());
          });

  connect(m_SpectrumMean,
          &QAction::toggled,
          this,
          [&](bool v)
          {
            if (!v)
              return;

            for (auto &kv : this->m_DataProvider)
              kv.second->SetActiveProviderTo("Mean");

            UpdateGlobalMinMaxValues();
            UpdateCurrentMinMaxY();
            emit m_xAxis->rangeChanged(m_xAxis->min(), m_xAxis->max());
          });

  m_Menu->addAction(m_ShowLegend);
  m_Menu->addAction(m_ShowAxesTitles);

  connect(m_ShowAxesTitles,
          &QAction::toggled,
          this,
          [this](bool s)
          {
            MITK_INFO(GetStaticClassName()) << m_yAxis << " " << m_xAxis;
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
            for (auto &kv : this->m_DataProvider)
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

  m_Crosshair = new QGraphicsSimpleTextItem("", chart);
  auto b = m_Crosshair->brush();
  b.setColor(QColor::fromRgbF(1, 1, 1));
  m_Crosshair->setBrush(b);
  m_Crosshair->hide();

  connect(m_Controls.chartView, &m2::ChartView::mouseDoubleClick, this, &m2Spectrum::OnMouseDoubleClick);
  connect(m_Controls.chartView, &m2::ChartView::mousePress, this, &m2Spectrum::OnMousePress);
  connect(m_Controls.chartView, &m2::ChartView::mouseMove, this, &m2Spectrum::OnMouseMove);
  connect(m_Controls.chartView, &m2::ChartView::mouseRelease, this, &m2Spectrum::OnMouseRelease);
  connect(m_Controls.chartView, &m2::ChartView::mouseWheel, this, &m2Spectrum::OnMouseWheel);
}

void m2Spectrum::AutoZoomUseLocalExtremaY()
{
  if (m_M2aiaPreferences->GetBool("useMaxIntensity", true) || m_M2aiaPreferences->GetBool("useMinIntensity", true))
  {
    if (m_M2aiaPreferences->GetBool("useMaxIntensity", true))
    {
      m_yAxis->setMax(m_LocalMaximumY * 1.1);
    } // else use just current zoom, no adaptions.

    if (m_M2aiaPreferences->GetBool("useMinIntensity", true))
    {
      m_yAxis->setMin(m_LocalMinimumY * 0.9);
    }
    else
    {
      m_GlobalMinimumY = 0;
      m_LocalMinimumY = 0;
      m_yAxis->setMin(0);
    }
  }
}

void m2Spectrum::OnSeriesFocused(const mitk::DataNode *node)
{
  double xMin = m_GlobalMaximumX;
  double xMax = m_GlobalMinimumX;

  for (auto &kv2 : m_DataProvider[node]->GetProviders())
  {
    xMin = std::min(xMin, kv2.second->xs().front());
    xMax = std::max(xMax, kv2.second->xs().back());
  }
  m_xAxis->setRange(xMin, xMax);
  AutoZoomUseLocalExtremaY();
}

void m2Spectrum::NodeRemoved(const mitk::DataNode *node)
{
  if (dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    if (m_DataProvider.find(node) == m_DataProvider.end())
      return;

    const_cast<mitk::DataNode*>(node)->RemoveAllObservers();
    for(auto kv: m_NodeRealtedGraphicItems){
      kv.second.clear();
    }

    for (auto &kv : m_DataProvider[node]->GetProviders())
      m_Controls.chartView->chart()->removeSeries(kv.second->GetSeries());

    m_DataProvider.erase(node);
    UpdateAxisLabels(node, true);
  }

  OnResetView();
}

void m2Spectrum::SetSeriesVisible(QtCharts::QAbstractSeries *series, bool visibility)
{
  auto markers = m_chart->legend()->markers();
  std::vector<QtCharts::QLegendMarker *> series_marker;
  for (auto marker : markers)
  {
    if (marker->series() == series)
    {
      // switch (marker->type())
      marker->series()->setVisible(visibility);
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
      return;
    }
  }
}

void m2Spectrum::OnLegnedHandleMarkerClicked()
{
  auto *marker = qobject_cast<QtCharts::QLegendMarker *>(sender());
  Q_ASSERT(marker);
  SetSeriesVisible(marker->series(), !marker->series()->isVisible());
}

void m2Spectrum::OnResetView()
{
  if (m_DataProvider.empty())
    return;

  UpdateGlobalMinMaxValues();
  UpdateCurrentMinMaxY();
  m_xAxis->setRange(m_GlobalMinimumX, m_GlobalMaximumX);
  m_yAxis->setRange(m_GlobalMinimumY * 0.9, m_GlobalMaximumY * 1.1);
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

void m2Spectrum::OnRangeChangedAxisX(qreal xMin, qreal xMax)
{
  if (m_DataProvider.empty())
    return;

  m_LocalMaximumX = xMax;
  m_LocalMinimumX = xMin;

  auto axis = m_xAxis;
  if (xMax > m_GlobalMaximumX)
  {
    MITK_INFO("OnRangeChangedAxisX") << xMax << " > " << m_GlobalMaximumX;
    this->m_MouseDragUpperDelta = std::abs(m_GlobalMaximumX - this->m_MouseDragCenterPos);
    axis->blockSignals(true);
    axis->setMax(m_GlobalMaximumX);
    axis->blockSignals(false);
  }

  if (xMin < m_GlobalMinimumX)
  {
    MITK_INFO("OnRangeChangedAxisX") << xMin << " > " << m_GlobalMinimumX;
    this->m_MouseDragLowerDelta = std::abs(m_GlobalMinimumX - this->m_MouseDragCenterPos);
    axis->blockSignals(true);
    axis->setMin(m_GlobalMinimumX);
    axis->blockSignals(false);
  }

  UpdateCurrentMinMaxY();
  UpdateAllSeries();
  AutoZoomUseLocalExtremaY();
}

void m2Spectrum::UpdateAllSeries()
{
  for (auto &kv1 : m_DataProvider)
    for (auto &kv2 : kv1.second->GetProviders())
      kv2.second->UpdateBoundaries(m_LocalMinimumX, m_LocalMaximumX);
}

void m2Spectrum::OnRangeChangedAxisY(qreal min, qreal max)
{
  if (max > m_GlobalMaximumY * 1.1)
  {
    m_yAxis->blockSignals(true);
    m_yAxis->setMax(m_GlobalMaximumY * 1.1);
    m_yAxis->blockSignals(false);
  }

  if (min < m_GlobalMinimumY * 0.9)
  {
    m_yAxis->blockSignals(true);
    m_yAxis->setMin(m_GlobalMinimumY);
    m_yAxis->blockSignals(false);
  }
  UpdateSelectedArea();
}

void m2Spectrum::UpdateAxisLabels(const mitk::DataNode *node, bool remove)
{
  MITK_INFO(GetStaticClassName()) << "Update axis";

  if (auto image = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    auto label = image->GetSpectrumType().XAxisLabel;
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
    case m2::SpectrumType::Maximum:
      m_yAxis->setTitleText("Intensity (maximum)");
      break;
    case m2::SpectrumType::Sum:
      m_yAxis->setTitleText("Intensity (sum)");
      break;
    case m2::SpectrumType::Mean:
      m_yAxis->setTitleText("Intensity (mean)");
      break;
    default:
      break;
  }
}
