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
#pragma once

#include <QColor>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QVector>
#include <QWidget>

/**
 * @brief Widget that displays a thumbnail pixmap centered in its area and
 *        lets the user drag multiple rectangular selection regions on it.
 *
 * All selection coordinates are in pixmap-local space (0,0 = top-left of
 * the pixmap, regardless of the widget size or position).
 *
 * Interactions:
 *   - Drag on empty area  → add a new selection with a unique colour.
 *   - Click on a selection → remove that selection.
 *   - Double-click        → clear all selections.
 */
class Qm2ThumbnailWidget : public QWidget
{
  Q_OBJECT

public:
  struct SelectionEntry
  {
    QRect rect;   ///< pixmap-local coordinates
    QColor color;
  };

  explicit Qm2ThumbnailWidget(QWidget *parent = nullptr);

  /** Replace the displayed pixmap. Existing selections are preserved. */
  void setPixmap(const QPixmap &pixmap);
  const QPixmap &pixmap() const { return m_Pixmap; }

  /** Clear the pixmap and all selections (no signal emitted). */
  void clearPixmap();

  int selectionCount() const { return m_Selections.size(); }
  const SelectionEntry &selectionAt(int index) const { return m_Selections.at(index); }
  bool hasAnySelection() const { return !m_Selections.isEmpty(); }

  /** Replace all selections at once; colours are assigned by index. Emits selectionsChanged(). */
  void setSelections(const QVector<QRect> &rects);

  /** Discard all selections and repaint. Emits selectionsChanged(). */
  void clearAllSelections();

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

signals:
  void selectionsChanged();

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
  /** Rectangle (in widget coordinates) where the pixmap is drawn. */
  QRect displayedPixmapRect() const;

  /** Clamp start/end to pixmap bounds and return normalised pixmap-local QRect. */
  QRect clampToPixmap(const QPoint &start, const QPoint &end) const;

  /** Return index of topmost selection containing pixmapPos, or -1. */
  int findSelectionAt(const QPoint &pixmapPos) const;

  /** Deterministic colour pool for selections. */
  static QColor colorForIndex(int index);

  QPixmap m_Pixmap;
  QVector<SelectionEntry> m_Selections;
  QRect m_ActiveDrag;    ///< rect currently being drawn (not yet committed)
  QPoint m_DragStart;    ///< pixmap-local drag-start
  bool m_IsDragging = false;
  int m_PressedOnIndex = -1; ///< index of existing selection we pressed on (-1 = none)

  static constexpr int kMinExtent = 3;
};
