/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/m2aia/m2aia for details.

===================================================================*/

#include "m2SeriesDataProvider.h"

#include <QLineSeries>
#include <QRandomGenerator>
#include <algorithm>
#include <iterator>
#include <limits>
#include <mitkCoreServices.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>
#include <signal/m2Binning.h>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

m2::SeriesDataProvider::SeriesDataProvider()
  : m_DefaultColor(QColor::fromHsv(
      static_cast<int>(QRandomGenerator::global()->generateDouble() * 359.0), 220, 220))
{
}

// ---------------------------------------------------------------------------
// SetData
// ---------------------------------------------------------------------------

void m2::SeriesDataProvider::SetData(const m2::IntervalVector *data,
                                     m2::SpectrumFormat format)
{
  m_IntervalVector = data;
  m_Format = (format == m2::SpectrumFormat::None) ? data->GetType() : format;
}

// ---------------------------------------------------------------------------
// InitializeSeries
// ---------------------------------------------------------------------------

void m2::SeriesDataProvider::InitializeSeries()
{
  delete m_Series;

  m_Series = new QLineSeries();
  ApplyProfileStyle(m_Series);

  // For centroid data each peak is displayed as a vertical needle, so we
  // make individual points visible to aid identification at low zoom.
  const bool isCentroid =
    to_underlying(m_Format) & to_underlying(m2::SpectrumFormat::Centroid);
  m_Series->setPointsVisible(isCentroid);

  m_Series->setColor(m_DefaultColor);
}

// ---------------------------------------------------------------------------
// InitializeLoDData
// ---------------------------------------------------------------------------

void m2::SeriesDataProvider::InitializeLoDData()
{
  m_DataLoD.clear();

  if (to_underlying(m_Format) & to_underlying(m2::SpectrumFormat::Centroid))
  {
    // Centroid spectra: store a single full-resolution level.
    // The number of peaks is typically small enough that no decimation
    // is required.
    m_xs = m_IntervalVector->GetXMean();
    // Prefer the max-intensity channel; fall back to the mean channel so that
    // intervals whose y was only accumulated via add() also have a display value.
    m_ys = m_IntervalVector->GetYMax();
    if (m_ys.empty())
      m_ys = m_IntervalVector->GetYMean();
    if (m_xs.empty() || m_ys.empty())
      return;

    PointsVector full;
    full.reserve(static_cast<int>(m_xs.size()));
    for (std::size_t i = 0; i < m_xs.size(); ++i)
      full.append(QPointF(m_xs[i], m_ys[i]));
    m_DataLoD.push_back(std::move(full));
  }
  else
  {
    // Profile spectra: build one LoD level per stride.
    m_xs = m_IntervalVector->GetXMean();
    m_ys = m_IntervalVector->GetYMean();
    if (m_xs.empty() || m_ys.empty())
      return;

    m_DataLoD.reserve(k_Levels.size());
    for (unsigned int stride : k_Levels)
      m_DataLoD.push_back(BuildLoDLevel(m_xs, m_ys, stride));
  }
}

// ---------------------------------------------------------------------------
// GenerateSeriesDataWithinRange
// ---------------------------------------------------------------------------

void m2::SeriesDataProvider::GenerateSeriesDataWithinRange(double x1, double x2)
{
  if (!m_Series || m_xs.empty() || m_DataLoD.empty())
    return;

  // Default: show the full data range.
  if (x1 < 0.0 && x2 < 0.0)
  {
    x1 = m_xs.front();
    x2 = m_xs.back();
  }

  const int lodIndex = SelectLoD(x1, x2);
  const PointsVector &lod = m_DataLoD[lodIndex];

  if (lod.isEmpty())
  {
    m_Series->replace(PointsVector{});
    return;
  }

  // Binary search within the LoD vector, comparing on x only.
  const auto cmpX = [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); };

  auto itLow =
    std::lower_bound(lod.constBegin(), lod.constEnd(), QPointF{x1, 0.0}, cmpX);
  auto itHigh =
    std::upper_bound(lod.constBegin(), lod.constEnd(), QPointF{x2, 0.0}, cmpX);

  // Extend by one guard point on each side so that profile lines are not
  // clipped abruptly at the visible axis boundary.
  if (itLow != lod.constBegin())
    --itLow;
  if (itHigh != lod.constEnd())
    ++itHigh;

  const bool isCentroid =
    to_underlying(m_Format) & to_underlying(m2::SpectrumFormat::Centroid);

  QVector<QPointF> seriesData;
  const int rawCount = static_cast<int>(std::distance(itLow, itHigh));
  seriesData.reserve(isCentroid ? rawCount * 3 : rawCount);

  for (auto it = itLow; it != itHigh; ++it)
  {
    const double scaledY = it->y() * m_YScale;
    if (isCentroid)
    {
      // Vertical needle: floor → peak → floor
      seriesData.append(QPointF{it->x(), 0.0});
      seriesData.append(QPointF{it->x(), scaledY});
      seriesData.append(QPointF{it->x(), 0.0});
    }
    else
    {
      seriesData.append(QPointF{it->x(), scaledY});
    }
  }

  m_Series->replace(seriesData);
}

// ---------------------------------------------------------------------------
// SetColor
// ---------------------------------------------------------------------------

void m2::SeriesDataProvider::SetColor(QColor c)
{
  m_DefaultColor = c;
  if (m_Series)
    m_Series->setColor(c);
}

void m2::SeriesDataProvider::SetColor(qreal r, qreal g, qreal b, qreal a)
{
  QColor c;
  c.setRgbF(r, g, b, a);
  SetColor(c);
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void m2::SeriesDataProvider::ApplyProfileStyle(QXYSeries *series)
{
  QPen p = series->pen();
  p.setWidthF(0.75);
  p.setCapStyle(Qt::FlatCap);
  p.setJoinStyle(Qt::MiterJoin);
  series->setPen(p);
}

m2::SeriesDataProvider::PointsVector
m2::SeriesDataProvider::BuildLoDLevel(const std::vector<double> &xs,
                                      const std::vector<double> &ys,
                                      unsigned int stride)
{
  if (xs.size() != ys.size())
    mitkThrow() << "SeriesDataProvider::BuildLoDLevel: xs.size() != ys.size()";

  if (stride <= 1u)
  {
    // Stride 1 = full resolution, just copy.
    PointsVector full;
    full.reserve(static_cast<int>(xs.size()));
    for (std::size_t i = 0; i < xs.size(); ++i)
      full.append(QPointF(xs[i], ys[i]));
    return full;
  }

  const std::size_t numBins = std::max<std::size_t>(1u, xs.size() / stride);

  std::vector<m2::Interval> bins;
  bins.reserve(numBins);
  m2::Signal::equidistantBinning(
    xs.begin(), xs.end(), ys.begin(), std::back_inserter(bins), numBins);

  PointsVector result;
  result.reserve(static_cast<int>(bins.size()));
  for (const auto &b : bins)
    result.append(QPointF{b.x.mean(), b.y.max()});
  return result;
}

int m2::SeriesDataProvider::SelectLoD(double xMin, double xMax) const
{
  // Centroid spectra always use the single full-resolution level (index 0).
  if (to_underlying(m_Format) & to_underlying(m2::SpectrumFormat::Centroid))
    return 0;

  if (m_DataLoD.empty())
    return 0;

  auto *prefsService = mitk::CoreServices::GetPreferencesService();
  const int pointsWanted =
    prefsService->GetSystemPreferences()->GetInt("m2aia.view.spectrum.bins", 15000);

  const double visibleRange = xMax - xMin;
  if (visibleRange <= 0.0)
    return 0;

  // Target spatial density: desired number of points per x unit.
  const double targetDensity = pointsWanted / visibleRange;

  int bestLevel = 0;
  double bestDelta = std::numeric_limits<double>::max();

  for (int i = 0; i < static_cast<int>(m_DataLoD.size()); ++i)
  {
    const auto &lod = m_DataLoD[i];
    if (lod.size() < 2)
      continue;

    const double lodRange = lod.last().x() - lod.first().x();
    if (lodRange <= 0.0)
      continue;

    const double lodDensity = lod.size() / lodRange;
    const double delta = std::abs(targetDensity - lodDensity);
    if (delta < bestDelta)
    {
      bestDelta = delta;
      bestLevel = i;
    }
  }
  return bestLevel;
}
