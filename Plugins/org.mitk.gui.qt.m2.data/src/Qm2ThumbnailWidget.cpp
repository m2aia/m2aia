/*===================================================================

Mass Spectrometry Imaging applications for interactive
analysis in MITK (M2aia)

Copyright (c) Jonas Cordes, Hochschule Mannheim.
Division of Medical Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#include "Qm2ThumbnailWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <algorithm>

namespace
{
const QColor kSelectionColors[] = {
  QColor(237, 51, 59),   // red
  QColor(51, 142, 237),  // blue
  QColor(37, 193, 84),   // green
  QColor(237, 187, 51),  // amber
  QColor(149, 51, 237),  // purple
  QColor(237, 128, 51),  // orange
  QColor(51, 222, 213),  // teal
  QColor(237, 51, 180),  // pink
};
} // namespace

QColor Qm2ThumbnailWidget::colorForIndex(int index)
{
  return kSelectionColors[static_cast<unsigned>(index) %
                          (sizeof(kSelectionColors) / sizeof(kSelectionColors[0]))];
}

Qm2ThumbnailWidget::Qm2ThumbnailWidget(QWidget *parent)
  : QWidget(parent)
{
  setCursor(Qt::CrossCursor);
  setToolTip(tr("Drag to add a region. Click a region to remove it. Double-click to clear all."));
}

void Qm2ThumbnailWidget::setPixmap(const QPixmap &pixmap)
{
  m_Pixmap = pixmap;
  updateGeometry();
  update();
}

void Qm2ThumbnailWidget::clearPixmap()
{
  m_Pixmap = {};
  m_Selections.clear();
  m_ActiveDrag = {};
  m_IsDragging = false;
  updateGeometry();
  update();
}

void Qm2ThumbnailWidget::setSelections(const QVector<QRect> &rects)
{
  m_Selections.clear();
  for (int i = 0; i < rects.size(); ++i)
    m_Selections.append({rects[i], colorForIndex(i)});
  m_ActiveDrag = {};
  m_IsDragging = false;
  update();
  emit selectionsChanged();
}

void Qm2ThumbnailWidget::clearAllSelections()
{
  m_Selections.clear();
  m_ActiveDrag = {};
  m_IsDragging = false;
  update();
  emit selectionsChanged();
}

QSize Qm2ThumbnailWidget::sizeHint() const
{
  return m_Pixmap.isNull() ? QSize{512, 256} : m_Pixmap.size();
}

QSize Qm2ThumbnailWidget::minimumSizeHint() const
{
  return {128, 64};
}

QRect Qm2ThumbnailWidget::displayedPixmapRect() const
{
  if (m_Pixmap.isNull())
    return {};
  const QRect wr = rect();
  const QSize ps = m_Pixmap.size();
  const int x = std::max(0, (wr.width() - ps.width()) / 2);
  const int y = std::max(0, (wr.height() - ps.height()) / 2);
  return {QPoint(x, y), ps};
}

QRect Qm2ThumbnailWidget::clampToPixmap(const QPoint &start, const QPoint &end) const
{
  if (m_Pixmap.isNull())
    return {};
  const int w = m_Pixmap.width();
  const int h = m_Pixmap.height();
  const auto clamp = [w, h](const QPoint &p) {
    return QPoint(std::clamp(p.x(), 0, w), std::clamp(p.y(), 0, h));
  };
  return QRect(clamp(start), clamp(end)).normalized();
}

int Qm2ThumbnailWidget::findSelectionAt(const QPoint &pixmapPos) const
{
  // Search from top (last drawn) to bottom so topmost rect is hit first
  for (int i = m_Selections.size() - 1; i >= 0; --i)
  {
    if (m_Selections[i].rect.contains(pixmapPos))
      return i;
  }
  return -1;
}

void Qm2ThumbnailWidget::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  if (m_Pixmap.isNull())
  {
    painter.drawText(rect(), Qt::AlignCenter, tr("Image"));
    return;
  }

  const QRect pr = displayedPixmapRect();
  painter.drawPixmap(pr.topLeft(), m_Pixmap);

  painter.setRenderHint(QPainter::Antialiasing, false);

  for (const auto &entry : m_Selections)
  {
    const QRect overlayRect = entry.rect.translated(pr.topLeft());
    QColor fill = entry.color;
    fill.setAlpha(60);
    painter.fillRect(overlayRect, fill);
    painter.setPen(QPen(entry.color, 2));
    painter.drawRect(overlayRect);
  }

  if (m_IsDragging && m_ActiveDrag.isValid())
  {
    const QRect activeRect = m_ActiveDrag.translated(pr.topLeft());
    painter.fillRect(activeRect, QColor(180, 180, 180, 60));
    painter.setPen(QPen(QColor(160, 160, 160), 1, Qt::DashLine));
    painter.drawRect(activeRect);
  }
}

void Qm2ThumbnailWidget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() != Qt::LeftButton)
    return;

  const QRect pr = displayedPixmapRect();
  if (!pr.contains(event->pos()))
    return;

  const QPoint pixmapPos = event->pos() - pr.topLeft();
  m_IsDragging = true;
  m_DragStart = pixmapPos;
  m_ActiveDrag = {};
  m_PressedOnIndex = findSelectionAt(pixmapPos);
  update();
}

void Qm2ThumbnailWidget::mouseMoveEvent(QMouseEvent *event)
{
  if (!m_IsDragging)
    return;

  const QPoint pos = event->pos() - displayedPixmapRect().topLeft();
  m_ActiveDrag = clampToPixmap(m_DragStart, pos);
  update();
}

void Qm2ThumbnailWidget::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() != Qt::LeftButton || !m_IsDragging)
    return;

  m_IsDragging = false;
  const QPoint pos = event->pos() - displayedPixmapRect().topLeft();
  const QRect finalRect = clampToPixmap(m_DragStart, pos);
  m_ActiveDrag = {};

  const bool isTrivial = finalRect.width() < kMinExtent || finalRect.height() < kMinExtent;

  if (isTrivial)
  {
    // Treat as a click: remove the rect we pressed on, if any
    if (m_PressedOnIndex >= 0 && m_PressedOnIndex < m_Selections.size())
    {
      m_Selections.removeAt(m_PressedOnIndex);
      emit selectionsChanged();
    }
  }
  else
  {
    m_Selections.append({finalRect, colorForIndex(m_Selections.size())});
    emit selectionsChanged();
  }

  m_PressedOnIndex = -1;
  update();
}

void Qm2ThumbnailWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (event->button() != Qt::LeftButton)
    return;

  m_IsDragging = false;
  m_ActiveDrag = {};
  if (!m_Selections.isEmpty())
  {
    m_Selections.clear();
    update();
    emit selectionsChanged();
  }
}
