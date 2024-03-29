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

#include <QValueAxis>
#include <QtGui/QMouseEvent>
#include <iostream>
#include <qapplication.h>

m2::ChartView::ChartView(QtCharts::QChart *chart, QWidget *parent)
  : QtCharts::QChartView(chart, parent), m_isTouching(false)
{
  setRubberBand(QtCharts::QChartView::NoRubberBand);
  setCursor(Qt::CrossCursor);
}

m2::ChartView::ChartView(QWidget *parent) : QtCharts::QChartView(parent), m_isTouching(false)
{
  setRubberBand(QtCharts::QChartView::NoRubberBand);
  setCursor(Qt::CrossCursor);
}

void m2::ChartView::resizeEvent(QResizeEvent *event)
{
  QtCharts::QChartView::resizeEvent(event);
  //OnResize();
}

void m2::ChartView::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (event->buttons() != Qt::LeftButton)
    return;

  auto mods = QGuiApplication::keyboardModifiers();

  auto p = this->chart()->mapToValue(event->pos());
  emit(mouseDoubleClick(event->pos(), p.x(), p.y(), event->button(), mods));
  event->accept();
}

void m2::ChartView::wheelEvent(QWheelEvent *event)
{
  auto mods = QGuiApplication::keyboardModifiers();

  auto p = this->chart()->mapToValue(event->pos());
  emit(mouseWheel(event->pos(), p.x(), p.y(), event->angleDelta().y(), mods));
  QChartView::wheelEvent(event);
  event->accept();
}

void m2::ChartView::mouseMoveEvent(QMouseEvent *event)
{
  
  auto mods = QGuiApplication::keyboardModifiers();

  auto p = this->chart()->mapToValue(event->pos());
  emit(mouseMove(event->pos(), p.x(), p.y(), event->button(), mods));

  QChartView::mouseMoveEvent(event);
  event->accept();
}

void m2::ChartView::mousePressEvent(QMouseEvent *event)
{

  auto mods = QGuiApplication::keyboardModifiers();
  auto p = this->chart()->mapToValue(event->pos());
  emit(mousePress(event->pos(), p.x(), p.y(), event->button(), mods));
  QChartView::mousePressEvent(event);
  // event->accept();
}

void m2::ChartView::mouseReleaseEvent(QMouseEvent *event)
{
  auto mods = QGuiApplication::keyboardModifiers();

  auto p = this->chart()->mapToValue(event->pos());
  emit(mouseRelease(event->pos(), p.x(), p.y(), event->button(), mods));

  QChartView::mouseReleaseEvent(event);
  event->accept();

}
