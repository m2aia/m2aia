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

//void m2::ChartView::OnResize()
//{
//  if (chart())
//  {
////    unsigned tickY = 8;
//    // auto ySubHeight = chart()->size().height() / m_;
//    // auto xTickCount = chart()->size().width() / ySubHeight;
//    auto axes = chart()->axes(Qt::Vertical);
//    if (!axes.empty())
//    {
//      if (auto yAxis = dynamic_cast<QtCharts::QValueAxis *>(axes.back()))
//      {
//        yAxis->setTickCount(m_AxisYTicks);
//        yAxis->setMinorTickCount(1);
//        auto col = yAxis->gridLineColor();
//        col.setAlphaF(0.3);
//        yAxis->setGridLineColor(col);
//        col.setAlphaF(0.15);
//        yAxis->setMinorGridLineColor(col);
//        // yAxis->setTitleText("Intensity");
//      }
//    }
//    axes = chart()->axes(Qt::Horizontal);
//    if (!axes.empty())
//    {
//      if (auto xAxis = dynamic_cast<QtCharts::QValueAxis *>(axes.back()))
//      {
//        xAxis->setTickCount(m_AxisXTicks);
//        xAxis->setMinorTickCount(1);
//        xAxis->setLabelsAngle(0);
//        auto col = xAxis->gridLineColor();
//        col.setAlphaF(0.3);
//        xAxis->setGridLineColor(col);
//        col.setAlphaF(0.15);
//        xAxis->setMinorGridLineColor(col);
//        // xAxis->setTitleText("m/z");
//      }
//    }
//    repaint();
//  }
//}

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
  /*auto mods = QGuiApplication::keyboardModifiers();

  QRectF plotArea = chart()->plotArea();
  if ((mods.testFlag(Qt::AltModifier) || mods.testFlag(Qt::ShiftModifier))
    && plotArea.contains(event->pos())) {
    emit(RangeSelectionUpdate(p.x()));
  }
  else {
    setRubberBand(QChartView::RubberBand::RectangleRubberBand);
  }
  QChartView::mouseMoveEvent(event);*/
  auto mods = QGuiApplication::keyboardModifiers();
//  QRectF plotArea = chart()->plotArea();

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

  // auto mods = QGuiApplication::keyboardModifiers();
  /*auto mods = event->modifiers();
  if (mods.testFlag(Qt::AltModifier) || mods.testFlag(Qt::ShiftModifier)) {*/

  // m_origin.setY(chart()->rect().height() - 25.0);

  /*if (selectedArea)
  {
    this->chart()->removeSeries(selectedArea);
    if (selectedArea->upperSeries())
      delete selectedArea->upperSeries();
    if (selectedArea->lowerSeries())
      delete selectedArea->lowerSeries();
    delete selectedArea;
    selectedArea = nullptr;
  }

  auto upper = new QLineSeries();
  auto lower = new QLineSeries();


  std::cout << p0.x() << " " << p1.x() << " " << chart()->maximumHeight();

  *upper<< QPointF(p0.x(), chart()->maximumHeight()) << QPointF(p1.x(), chart()->maximumHeight());
  *lower<< QPointF(p0.x(), 0) << QPointF(p1.x(), 0);


  selectedArea = new QAreaSeries(upper, lower);
  this->chart()->addSeries(selectedArea);
  selectedArea->attachAxis(chart()->axes(Qt::Horizontal).front());
  selectedArea->attachAxis(chart()->axes(Qt::Vertical).front());

  QPen pen;
  pen.setWidth(0);
  selectedArea->setPen(pen);
  selectedArea->setName(QString("Selected range"));
  OnResize();*/

  //

  /*auto p0 = this->chart()->mapToValue(m_origin, chart()->series().back());
  auto p1 = this->chart()->mapToValue(event->pos(), chart()->series().back());
  auto x1 = p0.x();
  auto x2 = p1.x();
  auto tol = std::abs(x1 - x2)*0.5;
  auto mz = (x1 + x2) * 0.5;





  auto mods = QGuiApplication::queryKeyboardModifiers();
  if (mods.testFlag(Qt::ShiftModifier))
    emit(AlignSpectra(mz, tol));
  else if (mods.testFlag(Qt::AltModifier)) {
    emit(RangeSelectionUpdate(p.x()));
    emit(UpdateImage(mz, tol));
  }
  event->accept();
}
else {
  QChartView::mouseReleaseEvent(event);
  m_rubberBand->setVisible(false);
}*/
}


//
//#include <QtGui/QPainter>
//#include <QtGui/QFontMetrics>
//#include <QtWidgets/QGraphicsSceneMouseEvent>
//#include <QtGui/QMouseEvent>
//#include <QtCharts/QChart>
//
// Callout::Callout(QChart *chart) : QGraphicsItem(chart), m_chart(chart) {}
//
// QRectF Callout::boundingRect() const
//{
//  QPointF anchor = mapFromParent(m_chart->mapToPosition(m_anchor));
//  QRectF rect;
//  rect.setLeft(qMin(m_rect.left(), anchor.x()));
//  rect.setRight(qMax(m_rect.right(), anchor.x()));
//  rect.setTop(qMin(m_rect.top(), anchor.y()));
//  rect.setBottom(qMax(m_rect.bottom(), anchor.y()));
//  return rect;
//}
//
// void Callout::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
//{
//  Q_UNUSED(option)
//  Q_UNUSED(widget)
//  QPainterPath path;
//  path.addRoundedRect(m_rect, 5, 5);
//
//  QPointF anchor = mapFromParent(m_chart->mapToPosition(m_anchor));
//  if (!m_rect.contains(anchor))
//  {
//    QPointF point1, point2;
//
//    // establish the position of the anchor point in relation to m_rect
//    bool above = anchor.y() <= m_rect.top();
//    bool aboveCenter = anchor.y() > m_rect.top() && anchor.y() <= m_rect.center().y();
//    bool belowCenter = anchor.y() > m_rect.center().y() && anchor.y() <= m_rect.bottom();
//    bool below = anchor.y() > m_rect.bottom();
//
//    bool onLeft = anchor.x() <= m_rect.left();
//    bool leftOfCenter = anchor.x() > m_rect.left() && anchor.x() <= m_rect.center().x();
//    bool rightOfCenter = anchor.x() > m_rect.center().x() && anchor.x() <= m_rect.right();
//    bool onRight = anchor.x() > m_rect.right();
//
//    // get the nearest m_rect corner.
//    qreal x = (onRight + rightOfCenter) * m_rect.width();
//    qreal y = (below + belowCenter) * m_rect.height();
//    bool cornerCase = (above && onLeft) || (above && onRight) || (below && onLeft) || (below && onRight);
//    bool vertical = qAbs(anchor.x() - x) > qAbs(anchor.y() - y);
//
//    qreal x1 = x + leftOfCenter * 10 - rightOfCenter * 20 + cornerCase * !vertical * (onLeft * 10 - onRight * 20);
//    qreal y1 = y + aboveCenter * 10 - belowCenter * 20 + cornerCase * vertical * (above * 10 - below * 20);
//    ;
//    point1.setX(x1);
//    point1.setY(y1);
//
//    qreal x2 = x + leftOfCenter * 20 - rightOfCenter * 10 + cornerCase * !vertical * (onLeft * 20 - onRight * 10);
//    ;
//    qreal y2 = y + aboveCenter * 20 - belowCenter * 10 + cornerCase * vertical * (above * 20 - below * 10);
//    ;
//    point2.setX(x2);
//    point2.setY(y2);
//
//    path.moveTo(point1);
//    path.lineTo(anchor);
//    path.lineTo(point2);
//    path = path.simplified();
//  }
//  painter->setBrush(QColor(255, 255, 255));
//  painter->drawPath(path);
//  painter->drawText(m_textRect, m_text);
//}
//
// void Callout::mousePressEvent(QGraphicsSceneMouseEvent *event)
//{
//  event->setAccepted(true);
//}
//
// void Callout::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
//{
//  if (event->buttons() & Qt::LeftButton)
//  {
//    setPos(mapToParent(event->pos() - event->buttonDownPos(Qt::LeftButton)));
//    event->setAccepted(true);
//  }
//  else
//  {
//    event->setAccepted(false);
//  }
//}
//
// void Callout::setText(const QString &text)
//{
//  m_text = text;
//  QFontMetrics metrics(m_font);
//  m_textRect = metrics.boundingRect(QRect(0, 0, 150, 150), Qt::AlignLeft, m_text);
//  m_textRect.translate(5, 5);
//  prepareGeometryChange();
//  m_rect = m_textRect.adjusted(-5, -5, 5, 5);
//}
//
// void Callout::setAnchor(QPointF point)
//{
//  m_anchor = point;
//}
//
// void Callout::updateGeometry()
//{
//  prepareGeometryChange();
//  setPos(m_chart->mapToPosition(m_anchor) + QPoint(10, -50));
//}
