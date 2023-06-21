/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/m2aia/m2aia for details.

===================================================================*/

#pragma once

#include <QPointF>
#include <QVector>
#include <QXYSeries>
#include <array>

namespace m2
{

  /**
   * @brief Data Provider class for providing multiple Level of Detail (LoD) data sets.
   */
  class SeriesDataProvider
  {
  public:
    using PointsVector = QVector<QPointF>;

    enum Format{
        centorid,
        profile
    };

    SeriesDataProvider();
    virtual ~SeriesDataProvider(){}
    void Initialize(const std::vector<double> & xs, const std::vector<double> & ys, Format format);
    void UpdateBoundaries(double x1 = -1, double x2 = -1);
    
    const std::vector<double> & xs() const {return m_xs;}
    const std::vector<double> & ys() const {return m_ys;}
    
    void SetColor(qreal r, qreal g, qreal b);
    void SetColor(QColor c);
    
    QtCharts::QXYSeries * GetSeries() const{return m_Series;}
  private:
    /**
     * Qt Series for providing XY data for the QtChartsView
     */
    QtCharts::QXYSeries *m_Series;

    /**
     * This series is used to add line information to scatter plots
    */
    QtCharts::QXYSeries *m_SeriesHelper;

    /**
     * Data degeneration levels.
     * */
    std::array<unsigned int, 7> m_Levels = {1, 2, 4, 8, 16, 32, 64};

    /**
     * Stores untouched raw data x/y values.
     */
    std::vector<double> m_xs;
    std::vector<double> m_ys;

    /**
     * Stores for each level a degenerated datasets.
     */
    std::vector<PointsVector> m_DataLoD;

    /**
     * Indicate if it is a centorid or profile dataset.
     */
    Format m_Format;

    /**
     * 
    */
    QColor m_DefaultColor;

    /**
     *  @brief Create and set default values for a series (color of data node, style of marker (centroid or profile))
     *  @param seriesName 
     *  @param series 
    */
    void InitializeSeries();

    /**
     *  @brief Degenerate rawdata for LoD data.
     *  @param seriesName 
     *  @param series 
    */
    void InitializeData();


    static void SetProfileSpectrumDefaultStyle(QtCharts::QXYSeries *series);
    static void SetCentroidSpectrumDefaultMarkerStyle(QtCharts::QXYSeries *series);
    static void SetMarkerSpectrumDefaultMarkerStyle(QtCharts::QXYSeries *series);
    static PointsVector GenerateLoDData(std::vector<double> & xs, std::vector<double> & ys, unsigned int value);

    /**
     *  @brief Convert std::Vector of float for x and y values to a QList of PointF.
    */
    static PointsVector ConvertToQVector(const std::vector<double> & xs, const std::vector<double> & ys);

    int FindLoD(double xMin, double xMax) const;

    unsigned int m_WantedPointsInView = 1500;

  };
} // namespace m2