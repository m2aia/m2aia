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

#include <QPointF>
#include <QScatterSeries>
#include <QVector>
#include <QXYSeries>
#include <array>
#include <limits>
#include <m2CoreCommon.h>
#include <m2SpectrumImageBase.h>
#include <map>
#include <mitkDataNode.h>
#include <signal/m2Binning.h>
#include <string>

/**
 * @brief This helper class provides level spectrum data and utilities to receive subspectra.
 *
 */
class Qm2SpectrumChartDataProvider : public QObject
{
  Q_OBJECT

public:
  using PointModifierPushBackFunction = std::function<void(QVector<QPointF> &s, QPointF &&p)>;

  Qm2SpectrumChartDataProvider(const mitk::DataNode *node);

  template <class SeriesType>
  inline QtCharts::QXYSeries *MakeSeries(const std::string &groupName)
  {
    auto series = new SeriesType();
    SetGroupSeries(groupName, series);
    return series;
  }

  bool Exists(const std::string &groupName);

  void SetGroupSeries(const std::string &groupName, QtCharts::QXYSeries *series);
  void SetGroupVisiblity(const std::string &groupName, bool v);
  void SetGroupSpectrumType(const std::string &groupName, m2::SpectrumType type);

  void UpdateAllSeries(double xMin = -1, double xMax = -1);
  void UpdateGroup(const std::string &groupName, double xMin = -1, double xMax = -1);

  static void SetProfileSpectrumDefaultStyle(QtCharts::QXYSeries *series);
  static void SetCentroidSpectrumDefaultMarkerStyle(QtCharts::QXYSeries *series);

  QtCharts::QXYSeries *GetGroupSeries(const std::string &groupName) const;

  struct GroupData
  {
    m2::SpectrumFormat format;
    QtCharts::QXYSeries *series;
    m2::SpectrumType type;
    int currentLoD = 0;
    std::map<m2::SpectrumType, std::vector<QVector<QPointF>>> data;

    void SetCurrentSpectrumType(m2::SpectrumType type) { this->type = type; }
    void SetCurrentLoD(unsigned int lod) { currentLoD = lod; }

    QVector<QPointF> &GetRawData() { return data[type][0]; }
    const QVector<QPointF> &GetRawData() const { return data.at(type).at(0); }

    QVector<QPointF> &GetCurrentData() { return data[type][currentLoD]; }
    const QVector<QPointF> &GetCurrentData() const { return data.at(type).at(currentLoD); }
  };

  std::map<std::string, GroupData> m_Groups;
  const mitk::DataNode *m_DataNode;


  template <class XType, class YType>
  void Register(const std::string &groupName,
                m2::SpectrumType type,
                const std::vector<XType> &xs,
                const std::vector<YType> &ys)
  {
    
    m_Groups[groupName].type = type;
    if (type == m2::SpectrumType::None)
      return;

    if (xs.empty() || ys.empty()) // no raw data provided
    {
      MITK_INFO << "Added Empty";
      return;
    }

    if (ys.size() != xs.size())
      mitkThrow() << "Spectrum size isn't equal to x axis size!";

    // everything seems to be valid so start providing some level data for the given spectrum type
    auto &lodData = m_Groups[groupName].data[type];
    lodData.resize(m_BinSizes.size());

    auto lodDataIt = std::begin(lodData);
    for (unsigned int binSize : m_BinSizes)
    {
      std::vector<m2::Interval> binnedData;
      const auto numBins = xs.size() / binSize;
      m2::Signal::equidistantBinning(
        std::begin(xs), std::end(xs), std::begin(ys), std::back_inserter(binnedData), numBins);

      lodDataIt->clear();
      std::transform(
        std::begin(binnedData), std::end(binnedData), std::back_inserter(*lodDataIt), [](const m2::Interval &p) {
          return QPointF{p.x.mean(), p.y.max()};
        });
      ++lodDataIt;

      const auto lod = FindGroupLoD(groupName);
      m_Groups[groupName].series->replace(lodData[lod]);
    }
  }


  void Register(const std::string &groupName, m2::SpectrumType type) {
    std::vector<float> empty;
    Register(groupName, type, empty, empty);
  }
private:
  const std::function<bool(const QPointF &, const QPointF &)> lessXQPointF = [](const QPointF &a, const QPointF &b) {
    return a.x() < b.x();
  };
  int FindGroupLoD(const std::string &groupName, double xMin = -1, double xMax = -1) const;
  unsigned int m_WantedPointInView = 1500; // ~ 1p
  std::array<unsigned int, 7> m_BinSizes = {1, 3, 6, 12, 24, 48, 96};
  QColor m_Color;
};
