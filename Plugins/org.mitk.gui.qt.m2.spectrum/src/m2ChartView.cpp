/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include "m2ChartView.h"

#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

m2::ChartView::ChartView(QWidget *parent)
  : QChartView(parent)
{
  setRubberBand(QChartView::NoRubberBand);
  setCursor(Qt::CrossCursor);
}

m2::ChartView::ChartView(QChart *chart, QWidget *parent)
  : QChartView(chart, parent)
{
  setRubberBand(QChartView::NoRubberBand);
  setCursor(Qt::CrossCursor);
}

// ---------------------------------------------------------------------------
// Protected event overrides
// ---------------------------------------------------------------------------

void m2::ChartView::resizeEvent(QResizeEvent *event)
{
  QChartView::resizeEvent(event);
}

void m2::ChartView::mousePressEvent(QMouseEvent *event)
{
  const auto p = chart()->mapToValue(event->position());
  emit mousePress(event->position(), p.x(), p.y(),
                  event->button(),
                  QGuiApplication::keyboardModifiers());
  QChartView::mousePressEvent(event);
}

void m2::ChartView::mouseMoveEvent(QMouseEvent *event)
{
  const auto p = chart()->mapToValue(event->position());
  emit mouseMove(event->position(), p.x(), p.y(),
                 event->button(),
                 QGuiApplication::keyboardModifiers());
  QChartView::mouseMoveEvent(event);
  event->accept();
}

void m2::ChartView::mouseReleaseEvent(QMouseEvent *event)
{
  const auto p = chart()->mapToValue(event->position());
  emit mouseRelease(event->position(), p.x(), p.y(),
                    event->button(),
                    QGuiApplication::keyboardModifiers());
  QChartView::mouseReleaseEvent(event);
  event->accept();
}

void m2::ChartView::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (event->button() != Qt::LeftButton)
    return;

  const auto p = chart()->mapToValue(event->position());
  emit mouseDoubleClick(event->position(), p.x(), p.y(),
                        event->button(),
                        QGuiApplication::keyboardModifiers());
  event->accept();
}

void m2::ChartView::wheelEvent(QWheelEvent *event)
{
  const auto p = chart()->mapToValue(event->position());
  emit mouseWheel(event->position(), p.x(), p.y(),
                  event->angleDelta().y(),
                  QGuiApplication::keyboardModifiers());
  QChartView::wheelEvent(event);
  event->accept();
}
