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

#include <QColor>
#include <QPointF>
#include <QVector>
#include <QXYSeries>
#include <array>
#include <m2IntervalVector.h>

namespace m2
{
  /**
   * @brief Provides Level-of-Detail (LoD) spectrum data for a Qt chart series.
   *
   * Holds the raw x/y vectors from an IntervalVector and pre-computes
   * decimated copies at several levels. On each view-range change the
   * appropriate LoD is selected and only the visible points are pushed
   * into the owning QXYSeries.
   */
  class SeriesDataProvider
  {
  public:
    using PointsVector = QVector<QPointF>;

    SeriesDataProvider();
    ~SeriesDataProvider() = default;

    // Non-copyable – owns a QXYSeries pointer.
    SeriesDataProvider(const SeriesDataProvider &) = delete;
    SeriesDataProvider &operator=(const SeriesDataProvider &) = delete;

    // ---------------------------------------------------------------
    // Setup
    // ---------------------------------------------------------------

    /**
     * @brief Bind this provider to an IntervalVector data object.
     *
     * An optional @p format override can be supplied; otherwise the
     * format is taken from the data object itself.
     */
    void SetData(const m2::IntervalVector *data,
                 m2::SpectrumFormat format = m2::SpectrumFormat::None);

    /**
     * @brief Allocate the QXYSeries and apply default visual style.
     *
     * Must be called once after @ref SetData and before the series is
     * added to a QChart.
     */
    void InitializeSeries();

    /**
     * @brief Cache raw data and build all LoD levels.
     *
     * Must be called (again) whenever the underlying IntervalVector
     * data has changed.
     */
    void InitializeLoDData();

    // ---------------------------------------------------------------
    // Per-frame update
    // ---------------------------------------------------------------

    /**
     * @brief Replace the series points with the visible subset of data.
     *
     * Selects the most appropriate LoD for the given x range, finds
     * the data points that fall inside [@p x1, @p x2] (plus one guard
     * point on each side so profile lines are never clipped at the
     * axis edge), and replaces the series contents.
     *
     * Pass x1 < 0 && x2 < 0 to use the full data range.
     */
    void GenerateSeriesDataWithinRange(double x1 = -1.0, double x2 = -1.0);

    // ---------------------------------------------------------------
    // Accessors
    // ---------------------------------------------------------------

    /// Full raw x values (mean of each interval).
    const std::vector<double> &xs() const { return m_xs; }

    /// Full raw y values (max or mean depending on format).
    const std::vector<double> &ys() const { return m_ys; }

    m2::SpectrumFormat GetFormat() const { return m_Format; }

    QXYSeries *GetSeries() const { return m_Series; }

    // ---------------------------------------------------------------
    // Visual style
    // ---------------------------------------------------------------

    void SetColor(qreal r, qreal g, qreal b, qreal a = 1.0);
    void SetColor(QColor c);

    /**
     * @brief Set a multiplicative scale applied to every y value when the
     *        series data is pushed to the chart (default = 1.0).
     *
     * Used by the spectrum view to normalise series with very different
     * absolute intensity ranges so they can all share a single Y axis.
     */
    void   SetYScale(double scale) { m_YScale = scale; }
    double GetYScale() const       { return m_YScale; }

  private:
    // ---------------------------------------------------------------
    // Internal helpers
    // ---------------------------------------------------------------

    static void ApplyProfileStyle(QXYSeries *series);

    /**
     * @brief Build a decimated PointsVector from raw data.
     *
     * Every @p stride-th bin is kept; within each stride window the
     * maximum y is selected so that peaks are preserved.
     */
    static PointsVector BuildLoDLevel(const std::vector<double> &xs,
                                      const std::vector<double> &ys,
                                      unsigned int stride);

    /**
     * @brief Return the LoD level index that best matches a desired
     *        point density for the visible x range.
     *
     * Always returns 0 for centroid data.
     */
    int SelectLoD(double xMin, double xMax) const;

    // ---------------------------------------------------------------
    // Data members
    // ---------------------------------------------------------------

    /// The Qt series owned by this provider and displayed in the chart.
    QXYSeries *m_Series = nullptr;

    /// Source data.
    m2::IntervalVector::ConstPointer m_IntervalVector;

    /// Decimation stride values used to build LoD levels.
    static inline constexpr std::array<unsigned int, 7> k_Levels = {1, 2, 4, 8, 16, 32, 64};

    /// Full-resolution raw x (m/z mean) and y values.
    std::vector<double> m_xs;
    std::vector<double> m_ys;

    /// Pre-computed LoD data sets (index 0 = full resolution).
    std::vector<PointsVector> m_DataLoD;

    m2::SpectrumFormat m_Format = m2::SpectrumFormat::None;
    QColor m_DefaultColor;

    /// Multiplicative scale applied to each y coordinate on output (default 1.0).
    double m_YScale = 1.0;
  };

} // namespace m2