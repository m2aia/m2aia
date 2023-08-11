/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkDataNodePlotColorAction.h"

// mitk core
#include <m2IntervalVector.h>
#include <m2SpectrumImage.h>
#include <mitkColorProperty.h>
#include <mitkRenderingManager.h>

// qt
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>

QmitkDataNodePlotColorAction::QmitkDataNodePlotColorAction(QWidget *parent,
                                                           berry::IWorkbenchPartSite::Pointer workbenchPartSite)
  : QWidgetAction(parent), QmitkAbstractDataNodeAction(workbenchPartSite)
{
  InitializeAction();
}

QmitkDataNodePlotColorAction::QmitkDataNodePlotColorAction(QWidget *parent,
                                                           berry::IWorkbenchPartSite *workbenchPartSite)
  : QWidgetAction(parent), QmitkAbstractDataNodeAction(berry::IWorkbenchPartSite::Pointer(workbenchPartSite))
{
  InitializeAction();
}

void QmitkDataNodePlotColorAction::InitializeAction()
{
  m_ColorButton = new QPushButton;
  m_ColorButton->setToolTip("Change color of the plot line (node property key: spectrum.plot.color)");
  m_ColorButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  connect(m_ColorButton, &QPushButton::clicked, this, &QmitkDataNodePlotColorAction::OnPlotColorChanged);

  m_ColorButtonMarker = new QPushButton;
  m_ColorButtonMarker->setToolTip("Change color of the true measurements (node property key: spectrum.marker.color)");
  m_ColorButtonMarker->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  connect(m_ColorButtonMarker, &QPushButton::clicked, this, &QmitkDataNodePlotColorAction::OnMarkerColorChanged);

  QLabel *colorLabel = new QLabel(tr("Line: "));
  colorLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

  QHBoxLayout *colorWidgetLayout = new QHBoxLayout;
  colorWidgetLayout->setContentsMargins(4, 4, 4, 4);
  colorWidgetLayout->addWidget(colorLabel);
  colorWidgetLayout->addWidget(m_ColorButton);
  colorWidgetLayout->addWidget(m_ColorButtonMarker);

  QWidget *colorWidget = new QWidget;
  colorWidget->setLayout(colorWidgetLayout);

  setDefaultWidget(colorWidget);

  connect(this, &QmitkDataNodePlotColorAction::changed, this, &QmitkDataNodePlotColorAction::OnActionChanged);
}

void QmitkDataNodePlotColorAction::InitializeWithDataNode(const mitk::DataNode *dataNode)
{
  if (auto genericProperty = dataNode->GetProperty("spectrum.plot.color"))
  {
    if (auto colorProperty = dynamic_cast<mitk::ColorProperty *>(genericProperty))
    {
      auto c = colorProperty->GetColor();
      QColor color;
      color.setRgbF(c.GetRed(), c.GetGreen(), c.GetBlue());
      QString styleSheet = QString("background-color: ") + color.name(QColor::HexRgb);
      m_ColorButton->setAutoFillBackground(true);
      m_ColorButton->setStyleSheet(styleSheet);
    }
  }

  if (auto genericProperty = dataNode->GetProperty("spectrum.marker.color"))
  {
    if (auto colorProperty = dynamic_cast<mitk::ColorProperty *>(genericProperty))
    {
      auto c = colorProperty->GetColor();
      QColor color;
      color.setRgbF(c.GetRed(), c.GetGreen(), c.GetBlue());
      QString styleSheet = QString("background-color: ") + color.name(QColor::HexRgb);
      m_ColorButtonMarker->setAutoFillBackground(true);
      m_ColorButtonMarker->setStyleSheet(styleSheet);
    }
  }
}

void QmitkDataNodePlotColorAction::OnPlotColorChanged()
{
  auto dataNodes = GetSelectedNodes();
  if (dataNodes.isEmpty())
  {
    return;
  }

  for (auto &dataNode : dataNodes)
  {
    if (dataNode.IsNull())
    {
      continue;
    }
    QColor color;
    if (auto genericProperty = dataNode->GetProperty("spectrum.plot.color"))
    {
      if (auto colorProperty = dynamic_cast<mitk::ColorProperty *>(genericProperty))
      {
        auto c = colorProperty->GetColor();
        color.setRgbF(c.GetRed(), c.GetGreen(), c.GetBlue());
      }
    }
    color = QColorDialog::getColor(color, nullptr, QString(tr("Change plot color")));
    dataNode->SetProperty("spectrum.plot.color", mitk::ColorProperty::New(color.redF(), color.greenF(), color.blueF()));

     if (dynamic_cast<m2::SpectrumImage *>(dataNode->GetData()))
    {
      auto derivations = m_DataStorage.Lock()->GetDerivations(dataNode);
      for (auto node : *derivations)
      {
        if (dynamic_cast<m2::IntervalVector *>(node->GetData()))
        {
          node->SetProperty("spectrum.plot.color",
                            mitk::ColorProperty::New(color.redF(), color.greenF(), color.blueF()));
        }
      }
    }
  }
}

void QmitkDataNodePlotColorAction::OnMarkerColorChanged()
{
  auto dataNodes = GetSelectedNodes();
  if (dataNodes.isEmpty())
  {
    return;
  }

  for (auto &dataNode : dataNodes)
  {
    if (dataNode.IsNull())
    {
      continue;
    }
    QColor color;
    if (auto genericProperty = dataNode->GetProperty("spectrum.marker.color"))
    {
      if (auto colorProperty = dynamic_cast<mitk::ColorProperty *>(genericProperty))
      {
        auto c = colorProperty->GetColor();
        color.setRgbF(c.GetRed(), c.GetGreen(), c.GetBlue());
      }
    }
    color = QColorDialog::getColor(color, nullptr, QString(tr("Change marker color")));
    dataNode->SetProperty("spectrum.marker.color",
                          mitk::ColorProperty::New(color.redF(), color.greenF(), color.blueF()));

    if (dynamic_cast<m2::SpectrumImage *>(dataNode->GetData()))
    {
      auto derivations = m_DataStorage.Lock()->GetDerivations(dataNode);
      for (auto node : *derivations)
      {
        if (dynamic_cast<m2::IntervalVector *>(node->GetData()))
        {
          node->SetProperty("spectrum.marker.color",
                            mitk::ColorProperty::New(color.redF(), color.greenF(), color.blueF()));
        }
      }
    }
  }
}

void QmitkDataNodePlotColorAction::OnActionChanged()
{
  auto dataNode = GetSelectedNode();
  if (dataNode.IsNull())
  {
    return;
  }

  InitializeWithDataNode(dataNode);
}
