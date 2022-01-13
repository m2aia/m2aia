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

#include <QXYSeries>
#include <QScatterSeries>
#include <QPointF>
#include <QVector>
#include <limits>
#include <m2CoreCommon.h>
#include <map>
#include <mitkDataNode.h>
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

  template<class SeriesType>
  inline QtCharts::QXYSeries * MakeSeries(const std::string &groupName){
    auto series = new SeriesType();
    SetGroupSeries(groupName, series);
    return series;
    }
  
  void Register(const std::string &groupName, m2::SpectrumType type);
  bool Exists(const std::string &groupName);
  
  void SetGroupSeries(const std::string &groupName, QtCharts::QXYSeries * series);
  void SetGroupVisiblity(const std::string &groupName, bool v);
  void SetGroupSpectrumType(const std::string &groupName, m2::SpectrumType type);

  void UpdateAllGroups(double xMin = -1, double xMax = -1);
  void UpdateGroup(const std::string &groupName, double xMin = -1, double xMax = -1);
  
  static void SetProfileSpectrumDefaultStyle(QtCharts::QXYSeries *series);
  static void SetCentroidSpectrumDefaultMarkerStyle(QtCharts::QXYSeries *series);

  QtCharts::QXYSeries *GetGroupSeries(const std::string &groupName) const;
    
  struct GroupData{
    m2::SpectrumFormat format;
    QtCharts::QXYSeries * series;
    m2::SpectrumType type;
    int currentLoD = 0;
    std::map<m2::SpectrumType, std::vector<QVector<QPointF>>> data;
       
    void SetCurrentSpectrumType(m2::SpectrumType type){this->type = type;}
    void SetCurrentLoD(unsigned int lod ){currentLoD = lod;}

    QVector<QPointF> & GetRawData() {return data[type][0];}
    const QVector<QPointF> & GetRawData() const {return data.at(type).at(0);}

    QVector<QPointF> & GetCurrentData() {return data[type][currentLoD];}
    const QVector<QPointF> & GetCurrentData() const {return data.at(type).at(currentLoD);}
  };

  std::map<std::string, GroupData> m_Groups;
  const mitk::DataNode * m_DataNode;

private:
  const std::function<bool(const QPointF &, const QPointF &)> lessXQPointF = [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); };
  int FindGroupLoD(const std::string &groupName, double xMin = -1, double xMax = -1) const;
  unsigned int m_WantedPointInView = 1500; // ~ 1p
  std::array<unsigned int, 7> m_BinSizes = {1,2,4,8,16,32,64};
  QColor m_Color;
  

  
};
