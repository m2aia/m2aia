/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#pragma once

#include <QChart>
#include <QChartView>

namespace m2
{
  /**
   * @brief Custom QChartView that translates Qt mouse/wheel events into
   *        chart-data-space signals.
   *
   * The @p mz and @p intValue parameters delivered by each signal are the
   * chart-value-space coordinates obtained via QChart::mapToValue().
   */
  class ChartView : public QChartView
  {
    Q_OBJECT

  public:
    explicit ChartView(QWidget *parent = nullptr);
    explicit ChartView(QChart *chart, QWidget *parent = nullptr);

  signals:
    /** Emitted on button press. @p mz / @p intValue carry chart-space coords. */
    void mousePress(QPointF pos, qreal mz, qreal intValue,
                    Qt::MouseButton button, Qt::KeyboardModifiers mods);

    /** Emitted on every mouse movement. */
    void mouseMove(QPointF pos, qreal mz, qreal intValue,
                   Qt::MouseButton button, Qt::KeyboardModifiers mods);

    /** Emitted on button release. */
    void mouseRelease(QPointF pos, qreal mz, qreal intValue,
                      Qt::MouseButton button, Qt::KeyboardModifiers mods);

    /** Emitted on double-click. */
    void mouseDoubleClick(QPointF pos, qreal mz, qreal intValue,
                          Qt::MouseButton button, Qt::KeyboardModifiers mods);

    /** Emitted on wheel rotation.  @p angleDelta is the y-component of
     *  QWheelEvent::angleDelta() (positive = forward/up). */
    void mouseWheel(QPointF pos, qreal mz, qreal intValue,
                    int angleDelta, Qt::KeyboardModifiers mods);

  protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
  };

} // namespace m2
