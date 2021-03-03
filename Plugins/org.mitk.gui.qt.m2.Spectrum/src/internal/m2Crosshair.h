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

#include <QtCharts/QChartGlobal>
#include <QtCharts/QChart>
#include <QtGui/QFont>
#include <QtWidgets/QGraphicsItem>

class QGraphicsSceneMouseEvent;

QT_CHARTS_BEGIN_NAMESPACE
class QChart;
QT_CHARTS_END_NAMESPACE

QT_CHARTS_USE_NAMESPACE

class m2Crosshair : public QGraphicsItem
{
public:
  m2Crosshair(QChart *parent);

  void setText(const QString &text);
  void setAnchor(QPointF point);
  void updateGeometry();

  QRectF boundingRect() const;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

//protected:
//  void mousePressEvent(QGraphicsSceneMouseEvent *event);
//  void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

private:
  QString m_text;
  QRectF m_textRect;
  QRectF m_rect;
  QPointF m_anchor;
  QFont m_font;
  QChart *m_chart;
};