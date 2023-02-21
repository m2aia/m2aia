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
#include <m2SpectrumImageBase.h>
#include <signal/m2Binning.h>

Qm2SpectrumChartDataProvider::Qm2SpectrumChartDataProvider(const mitk::DataNode *node) : m_DataNode(node)
{
  auto rnd = QRandomGenerator::global()->generateDouble();
  m_Color = QColor::fromHsv(rnd * 255, 255, 255);
}

void Qm2SpectrumChartDataProvider::Register(const std::string &groupName, m2::SpectrumType type)
{
  if (!m_DataNode)
    mitkThrow() << "Data node is null!";

  if (!Exists(groupName))
    mitkThrow() << "No group found with the given name!";


  if (auto image = dynamic_cast<m2::SpectrumImageBase *>(m_DataNode->GetData()))
  {
    
    if (type == m2::SpectrumType::None){
      m_Groups.at(groupName).series->setVisible(!image->GetPeaks().empty());
      return;
    }

    auto &artifacts = image->GetSpectraArtifacts();

    // check if image artifacts contain the corresponding spectrum type
    if (artifacts.find(type) != artifacts.end())
    {
      // get spectrum type data
      auto ys = artifacts.at(type);
      if (ys.empty())
        mitkThrow() << "Entry " << static_cast<int>(type) << " found but it is empty!";

      // check size
      auto xs = image->GetXAxis();
      if (ys.size() != xs.size())
        mitkThrow() << "Spectrum size isn't equal to x axis size!";

      // everything seems to be valid so start providing some level data for the given spectrum type
      auto &lodData = m_Groups[groupName].data[type];
      lodData.resize(m_BinSizes.size());
      m_Groups[groupName].type = type;

      auto lodDataIt = std::begin(lodData);
      for (unsigned int binSize : m_BinSizes)
      {
        std::vector<m2::Interval> binnedData;
        const auto numBins = xs.size() / binSize;
        m2::Signal::equidistantBinning(std::begin(xs), std::end(xs), std::begin(ys), std::back_inserter(binnedData), numBins);

        lodDataIt->clear();
        std::transform(std::begin(binnedData),
                       std::end(binnedData),
                       std::back_inserter(*lodDataIt),
                       [](const m2::Interval &p) {
                         return QPointF{p.x.mean(), p.y.max()};
                       });
        ++lodDataIt;
      }

      const auto lod = FindGroupLoD(groupName);
      m_Groups[groupName].series->replace(lodData[lod]);
    }
  }
}

void Qm2SpectrumChartDataProvider::UpdateGroup(const std::string &groupName, double xMin, double xMax)
{
  if (auto image = dynamic_cast<m2::SpectrumImageBase *>(m_DataNode->GetData()))
  {
    // no update for non spectra
    if (m_Groups[groupName].type == m2::SpectrumType::None)
    {
      if (xMin < 0 && xMax < 0)
      {
        return;
      }
      if (!image->GetPeaks().empty())
      {
        m_Groups.at(groupName).series->setVisible(true);
        const auto &peaks = image->GetPeaks();
        auto lower = std::lower_bound(std::begin(peaks), std::end(peaks), m2::Interval{0, xMin, 0});
        const auto upper = std::upper_bound(lower, std::end(peaks), m2::Interval{0, xMax, 0});
        
        QVector<QPointF> seriesData;
        auto insert = std::back_inserter(seriesData);
        for (;lower != upper; ++lower)
          insert = QPointF{lower->x.mean(), lower->y.mean()};
        
        m_Groups.at(groupName).series->replace(seriesData);
      }else{
        m_Groups.at(groupName).series->setVisible(false);
      }
    }
    else
    {
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
      auto lower =
        std::lower_bound(std::begin(currentData), std::end(currentData), QPointF{xMin, 0}, lessXQPointF);
      const auto upper = std::upper_bound(lower, std::end(currentData), QPointF{xMax, 0}, lessXQPointF);

      auto currentSeries = m_Groups.at(groupName).series;
      
      QVector<QPointF> seriesData;
      seriesData.reserve(std::distance(lower, upper));
      auto insert = std::back_inserter(seriesData);
      for (;lower != upper; ++lower)
      {
        if(isCentroid) insert = QPointF{lower->x(), -0.001};
        insert = QPointF{lower->x(), lower->y()};
        if(isCentroid) insert = QPointF{lower->x(), 0};
      }
      currentSeries->replace(seriesData);
    }
  }
}

int Qm2SpectrumChartDataProvider::FindGroupLoD(const std::string &groupName, double xMin, double xMax) const
{
  const auto &type = m_Groups.at(groupName).type;

  std::function<double(int)> WantedPointsInViewQuotient = [&](int lod) -> double
  {
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

void Qm2SpectrumChartDataProvider::UpdateAllGroups(double xMin, double xMax)
{
  for (auto &kv : m_Groups)
    UpdateGroup(kv.first, xMin, xMax);
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
