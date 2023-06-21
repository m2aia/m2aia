/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "Qm2SpectrumChartDataProvider.h"

#include <QRandomGenerator>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QXYSeries>

Qm2SpectrumChartDataProvider::Qm2SpectrumChartDataProvider(const mitk::DataNode *node) : m_DataNode(node)
{
  auto rnd = QRandomGenerator::global()->generateDouble();
  m_Color = QColor::fromHsv(rnd * 255, 255, 255);
}

void Qm2SpectrumChartDataProvider::UpdateGroup(const std::string &groupName, double xMin, double xMax)
{
  if (auto image = dynamic_cast<m2::SpectrumImageBase *>(m_DataNode->GetData()))
  {
    // no update for non spectra

    if (xMin < 0 && xMax < 0)
    {
      return;
    }

    if (groupName == "Peaks")
    {
      if(image->GetIntervals().empty()){
        return;
      }

      using namespace std;
      m_Groups.at(groupName).series->setVisible(true);
      const auto &peaks = image->GetIntervals(); //use for location
      auto lower = lower_bound(begin(peaks), end(peaks), m2::Interval{xMin, 0});
      const auto upper = upper_bound(lower, end(peaks), m2::Interval{xMax, 0});

      QVector<QPointF> seriesData;
      seriesData.reserve(distance(lower,upper));
      auto insert = back_inserter(seriesData);
      auto type = m_Groups.at(groupName).type;
      
      // position peak indicators at the bottom of the view (at y=0)
      if(m_Groups.find("Overview") == m_Groups.end()){
        for (; lower != upper; ++lower)
          insert = QPointF{lower->x.mean(),0};
      }else{ // use overview spectrum for y position
        const auto & overviewGroup = m_Groups.at("Overview");
        const auto & v = overviewGroup.data.at(type).at(0);
        for (; lower != upper; ++lower){
          auto lb = lower_bound(begin(v),end(v),QPointF{lower->x.mean(),0},[](const QPointF &a,const QPointF &b){return a.x() < b.x();});
          insert = QPointF{lower->x.mean(),lb->y()};
        }
      }
      m_Groups.at(groupName).series->replace(seriesData);

    }else{
      if (xMin < 0 && xMax < 0)
      {
        const auto &data = this->m_Groups[groupName].GetRawData();
        xMin = data.front().x();
        xMax = data.back().y();
      }
      m_Groups[groupName].currentLoD = FindGroupLoD(groupName, xMin, xMax);

      bool isCentroid = image->GetSpectrumType().Format == m2::SpectrumFormat::ContinuousCentroid;
      isCentroid |= image->GetSpectrumType().Format == m2::SpectrumFormat::ProcessedCentroid;

      const auto currentData = m_Groups[groupName].GetCurrentData();
      auto lower = std::lower_bound(std::begin(currentData), std::end(currentData), QPointF{xMin, 0}, lessXQPointF);
      const auto upper = std::upper_bound(lower, std::end(currentData), QPointF{xMax, 0}, lessXQPointF);

      auto currentSeries = m_Groups.at(groupName).series;

      QVector<QPointF> seriesData;
      seriesData.reserve(std::distance(lower, upper));
      auto insert = std::back_inserter(seriesData);
      for (; lower != upper; ++lower)
      {
        if (isCentroid)
          insert = QPointF{lower->x(), -0.001};
        insert = QPointF{lower->x(), lower->y()};
        if (isCentroid)
          insert = QPointF{lower->x(), 0};
      }
      currentSeries->replace(seriesData);
    }
  }
}

int Qm2SpectrumChartDataProvider::FindGroupLoD(const std::string &groupName, double xMin, double xMax) const
{
  const auto &type = m_Groups.at(groupName).type;

  std::function<double(int)> WantedPointsInViewQuotient = [&](int lod) -> double {
    if (lod < 0 || lod >= (int)m_BinSizes.size())
      return std::numeric_limits<double>::max();

    auto const &points = m_Groups.at(groupName).data.at(type).at(lod);
    const auto lower = std::lower_bound(std::cbegin(points), std::cend(points), QPointF{xMin, 0}, lessXQPointF);
    const auto upper = std::upper_bound(lower, std::cend(points), QPointF{xMax, 0}, lessXQPointF);
    auto qCurrent = std::distance(lower, upper) / (double)m_WantedPointInView;

    return qCurrent;
  };

  const int lod = m_Groups.at(groupName).currentLoD;
  const auto qCurrent = WantedPointsInViewQuotient(lod);
  int lodLower = lod;
  int lodUpper = lod;
  double qLowerPrev = qCurrent;
  double qUpperPrev = qCurrent;

  {
    double qLower = WantedPointsInViewQuotient(--lodLower);
    while (std::abs(qLower - 1) < std::abs(qLowerPrev - 1))
    {
      qLowerPrev = qLower;
      qLower = WantedPointsInViewQuotient(--lodLower);
    }
    ++lodLower;
  }

  {
    double qUpper = WantedPointsInViewQuotient(++lodUpper);
    while (std::abs(qUpper - 1) < std::abs(qUpperPrev - 1))
    {
      qUpperPrev = qUpper;
      qUpper = WantedPointsInViewQuotient(++lodUpper);
    }
    --lodUpper;
  }

  if ((lodUpper - lod + lodLower - lod) != 0)
  {
    if (std::abs(qLowerPrev - 1) < std::abs(qUpperPrev - 1))
      return lodLower;
    else
      return lodUpper;
  }
  return lod;
}

void Qm2SpectrumChartDataProvider::UpdateAllSeries(double , double )
{
  // for (auto &kv1 : m_Groups) 
      // kv1.second->UpdateBoundaries(xMin, xMax);
}

void Qm2SpectrumChartDataProvider::SetGroupVisiblity(const std::string &groupName, bool v)
{
  m_Groups.at(groupName).series->setVisible(v);
}

void Qm2SpectrumChartDataProvider::SetGroupSpectrumType(const std::string &groupName, m2::SpectrumType type)
{
  m_Groups.at(groupName).SetCurrentSpectrumType(type);
}

bool Qm2SpectrumChartDataProvider::Exists(const std::string &groupName)
{
  return m_Groups.find(groupName) != m_Groups.end();
}

void Qm2SpectrumChartDataProvider::SetGroupSeries(const std::string &groupName, QtCharts::QXYSeries *series)
{
  m_Groups[groupName].series = series;
  series->setColor(m_Color);

  if (dynamic_cast<QtCharts::QScatterSeries *>(series))
    SetCentroidSpectrumDefaultMarkerStyle(series);
  else
    SetProfileSpectrumDefaultStyle(series);
}

void Qm2SpectrumChartDataProvider::SetProfileSpectrumDefaultStyle(QtCharts::QXYSeries *series)
{
  auto p = series->pen();
  p.setWidthF(.75);
  p.setCapStyle(Qt::FlatCap);
  series->setPen(p);
}

void Qm2SpectrumChartDataProvider::SetCentroidSpectrumDefaultMarkerStyle(QtCharts::QXYSeries *series)
{
  if (auto scatterSeries = dynamic_cast<QtCharts::QScatterSeries *>(series))
  {
    scatterSeries->setMarkerShape(QtCharts::QScatterSeries::MarkerShape::MarkerShapeRectangle);
    scatterSeries->setMarkerSize(3);
  }
}

QtCharts::QXYSeries *Qm2SpectrumChartDataProvider::GetGroupSeries(const std::string &groupName) const
{
  try
  {
    return m_Groups.at(groupName).series;
  }
  catch (const std::exception &e)
  {
    MITK_INFO << "Series with name " + groupName + " does not exist!";
    return nullptr;
  }
}
