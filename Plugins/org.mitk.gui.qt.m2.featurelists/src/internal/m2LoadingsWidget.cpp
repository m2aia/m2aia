/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "m2LoadingsWidget.h"

#include <QHelpEvent>
#include <QPainter>
#include <QToolTip>

#include <algorithm>
#include <cmath>

namespace
{
  /** Width one bar asks for; the width of the column follows from it. */
  constexpr int PREFERRED_BAR_WIDTH = 7;
  constexpr int MINIMUM_BAR_WIDTH = 2;
  /** Height the widget asks for, whatever the number of components. */
  constexpr int PREFERRED_HEIGHT = 20;
  constexpr int MINIMUM_HEIGHT = 10;
  /** Kept free above and below, so that a full length bar does not touch the cell border. */
  constexpr double VERTICAL_MARGIN = 1.0;
  /** Gap between neighbouring bars. */
  constexpr double BAR_GAP = 1.0;

  /** A hash that does not change between sessions, unlike qHash on a QString, which Qt is free to
      seed randomly. An unknown method has to keep its colours across restarts to be recognisable. */
  unsigned int StableHash(const QString &text)
  {
    // FNV-1a over the utf-8 bytes
    unsigned int hash = 2166136261u;
    for (const auto byte : text.toUtf8())
    {
      hash ^= static_cast<unsigned char>(byte);
      hash *= 16777619u;
    }

    return hash;
  }
} // namespace

m2LoadingsWidget::m2LoadingsWidget(QWidget *parent) : QWidget(parent)
{
  // the height is fixed on purpose: more components must not make the row of the table taller
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void m2LoadingsWidget::SetEntries(std::vector<Entry> entries)
{
  m_Entries = std::move(entries);
  updateGeometry();
  update();
}

void m2LoadingsWidget::SetColorTable(const ColorTable &table)
{
  m_ColorTable = table;
  update();
}

m2LoadingsWidget::ColorTable m2LoadingsWidget::ColorTableForMethod(const QString &methodName)
{
  // one fixed table per method of the Data Compression view, so that a column is recognisable by
  // its colours alone. The hues are kept apart from each other rather than chosen for beauty.
  const auto method = methodName.trimmed().toUpper();

  if (method == QLatin1String("PCA"))
    return {QColor(180, 4, 38), QColor(59, 76, 192), true}; // red over blue

  if (method == QLatin1String("ICA"))
    return {QColor(230, 111, 81), QColor(106, 61, 154), true}; // orange over purple

  if (method == QLatin1String("PLSDA"))
    return {QColor(198, 36, 139), QColor(15, 123, 108), true}; // magenta over teal

  if (method == QLatin1String("NMF"))
  {
    // a non-negative factorization never produces a negative loading, so it gets one colour and
    // the full height of the cell
    return {QColor(42, 157, 143), QColor(42, 157, 143), false};
  }

  // anything else: two opposite hues derived from the name, stable across sessions
  const int hue = static_cast<int>(StableHash(method) % 360u);
  ColorTable table;
  table.positive = QColor::fromHsv(hue, 200, 175);
  table.negative = QColor::fromHsv((hue + 180) % 360, 190, 195);
  table.signedValues = true;

  return table;
}

void m2LoadingsWidget::Paint(QPainter &painter,
                             const QRect &area,
                             const std::vector<Entry> &entries,
                             const ColorTable &colors)
{
  const auto count = static_cast<int>(entries.size());
  if (count == 0 || area.width() <= 0 || area.height() <= 0)
    return;

  // the bars share the width; the height is what it is, which is what keeps the row from growing
  const double slotWidth = static_cast<double>(area.width()) / count;

  // a method that cannot go negative puts its baseline at the bottom and uses the whole height;
  // one that can keeps it in the middle so that both directions have room
  const double baseline =
    colors.signedValues ? area.top() + area.height() * 0.5 : area.bottom() - VERTICAL_MARGIN;
  const double fullLength = std::max(
    1.0, colors.signedValues ? area.height() * 0.5 - VERTICAL_MARGIN : area.height() - 2.0 * VERTICAL_MARGIN);

  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, false);

  // the zero line, so that the sign of a short bar is still readable
  painter.setPen(QPen(QColor(128, 128, 128, 110)));
  painter.drawLine(QPointF(area.left(), baseline), QPointF(area.right(), baseline));

  const double barWidth = std::max(1.0, slotWidth - BAR_GAP);

  for (int i = 0; i < count; ++i)
  {
    const auto &entry = entries[static_cast<size_t>(i)];
    const double left = area.left() + i * slotWidth;

    if (!entry.valid || entry.scale <= 0.0)
    {
      // a centroid that does not carry this component keeps its slot empty; drawing a zero would
      // claim a measurement that was never made
      continue;
    }

    const double normalized = std::max(-1.0, std::min(1.0, entry.value / entry.scale));
    const double length = std::abs(normalized) * fullLength;

    // a loading that is not exactly zero stays visible as a one pixel stub
    const double drawn = normalized == 0.0 ? 0.0 : std::max(1.0, length);

    // an unsigned method draws every bar upwards, whatever rounding left of the value
    const bool upwards = !colors.signedValues || normalized >= 0.0;
    const QRectF bar = upwards ? QRectF(left, baseline - drawn, barWidth, drawn)
                               : QRectF(left, baseline, barWidth, drawn);

    painter.setPen(Qt::NoPen);
    painter.setBrush(upwards ? colors.positive : colors.negative);
    painter.drawRect(bar);
  }

  painter.restore();
}

void m2LoadingsWidget::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  Paint(painter, rect(), m_Entries, m_ColorTable);
}

QSize m2LoadingsWidget::sizeHint() const
{
  // the width follows the number of components, the height never does
  const auto count = std::max<int>(1, static_cast<int>(m_Entries.size()));
  return QSize(count * PREFERRED_BAR_WIDTH, PREFERRED_HEIGHT);
}

QSize m2LoadingsWidget::minimumSizeHint() const
{
  const auto count = std::max<int>(1, static_cast<int>(m_Entries.size()));
  return QSize(count * MINIMUM_BAR_WIDTH, MINIMUM_HEIGHT);
}

int m2LoadingsWidget::IndexAt(const QPoint &position) const
{
  const auto count = static_cast<int>(m_Entries.size());
  if (count == 0 || width() <= 0)
    return -1;

  const int index = position.x() * count / width();

  return index >= 0 && index < count ? index : -1;
}

bool m2LoadingsWidget::event(QEvent *event)
{
  if (event->type() == QEvent::ToolTip)
  {
    auto *helpEvent = static_cast<QHelpEvent *>(event);
    const int index = IndexAt(helpEvent->pos());

    if (index < 0)
      QToolTip::hideText();
    else
    {
      const auto &entry = m_Entries[static_cast<size_t>(index)];
      QToolTip::showText(helpEvent->globalPos(),
                         entry.valid ? QString("%1\n%2").arg(entry.name).arg(entry.value, 0, 'g', 6)
                                     : QString("%1\nnot available for this centroid").arg(entry.name));
    }

    event->accept();
    return true;
  }

  return QWidget::event(event);
}
