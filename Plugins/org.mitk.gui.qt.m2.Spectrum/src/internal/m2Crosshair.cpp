/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include "m2Crosshair.h"

#include <QtCharts/QChart>
#include <QtGui/QFontMetrics>
#include <QtGui/QMouseEvent>
#include <qwidget.h>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsSceneMouseEvent>

m2Crosshair::m2Crosshair(QChart *chart) : QGraphicsItem(chart), m_chart(chart) {}

QRectF m2Crosshair::boundingRect() const
{
  QPointF anchor = mapFromParent(m_anchor);
  QRectF rect;
  rect.setLeft(qMin(m_rect.left(), anchor.x()));
  rect.setRight(qMax(m_rect.right(), anchor.x()));
  rect.setTop(qMin(m_rect.top(), anchor.y()));
  rect.setBottom(qMax(m_rect.bottom(), anchor.y()));
  return rect;
}

void m2Crosshair::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(option)
  Q_UNUSED(widget)
  
  painter->setBrush(QColor(255, 255, 255));
  painter->drawText(m_textRect, m_text);
  
}

void m2Crosshair::setText(const QString &text)
{
  m_text = text;
  QFontMetrics metrics(m_font);
  m_textRect = metrics.boundingRect(QRect(0, 0, 150, 150), Qt::AlignLeft, m_text);
  m_textRect.translate(5, 5);
  prepareGeometryChange();
}

void m2Crosshair::setAnchor(QPointF point)
{
  m_anchor = point;
}

void m2Crosshair::updateGeometry()
{
  prepareGeometryChange();
  setPos(m_anchor + QPoint(10, -50));
}
