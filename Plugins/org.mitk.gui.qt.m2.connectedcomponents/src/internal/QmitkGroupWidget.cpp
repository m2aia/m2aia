/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "QmitkGroupWidget.h"

#include <QColorDialog>
#include <QFrame>

QmitkGroupWidget::QmitkGroupWidget(const QString &groupName,
                                   int minSize,
                                   int maxSize,
                                   const QColor &color,
                                   int absoluteMax,
                                   QWidget *parent)
  : QWidget(parent), m_Color(color)
{
  // ---- outer group box ----
  m_GroupBox = new QGroupBox(groupName, this);
  m_GroupBox->setCheckable(false);

  auto *outerLayout = new QVBoxLayout(this);
  outerLayout->setContentsMargins(0, 0, 0, 0);
  outerLayout->addWidget(m_GroupBox);

  auto *innerLayout = new QVBoxLayout(m_GroupBox);
  innerLayout->setContentsMargins(6, 4, 6, 4);
  innerLayout->setSpacing(4);

  // ---- Row 1: color button + remove button ----
  auto *row1 = new QHBoxLayout();
  m_ColorBtn = new QPushButton("Color", this);
  m_ColorBtn->setToolTip("Click to change group color");
  m_ColorBtn->setFixedHeight(24);
  UpdateColorButton();

  m_RemoveBtn = new QPushButton("Remove", this);
  m_RemoveBtn->setFixedHeight(24);
  m_RemoveBtn->setToolTip("Remove this group");

  row1->addWidget(m_ColorBtn, 1);
  row1->addWidget(m_RemoveBtn);
  innerLayout->addLayout(row1);

  // ---- Row 2: Min/Max spinboxes ----
  auto *row2 = new QHBoxLayout();
  auto *minLabel = new QLabel("Min voxels:", this);
  m_MinSpin = new QSpinBox(this);
  m_MinSpin->setRange(0, absoluteMax > 0 ? absoluteMax : 999999999);
  m_MinSpin->setValue(minSize);
  m_MinSpin->setToolTip("Minimum voxel count (inclusive)");

  auto *maxLabel = new QLabel("Max voxels:", this);
  m_MaxSpin = new QSpinBox(this);
  m_MaxSpin->setRange(0, absoluteMax > 0 ? absoluteMax : 999999999);
  m_MaxSpin->setValue(maxSize);
  m_MaxSpin->setToolTip("Maximum voxel count (inclusive)");

  row2->addWidget(minLabel);
  row2->addWidget(m_MinSpin, 1);
  row2->addWidget(maxLabel);
  row2->addWidget(m_MaxSpin, 1);
  innerLayout->addLayout(row2);

  // ---- Row 3: matched component count ----
  m_CountLabel = new QLabel("Components in group: -", this);
  m_CountLabel->setAlignment(Qt::AlignCenter);
  innerLayout->addWidget(m_CountLabel);

  // ---- Connections ----
  connect(m_ColorBtn,  &QPushButton::clicked, this, &QmitkGroupWidget::OnColorButtonClicked);
  connect(m_RemoveBtn, &QPushButton::clicked, this, &QmitkGroupWidget::OnRemoveClicked);
  connect(m_MinSpin, qOverload<int>(&QSpinBox::valueChanged), this, &QmitkGroupWidget::OnMinChanged);
  connect(m_MaxSpin, qOverload<int>(&QSpinBox::valueChanged), this, &QmitkGroupWidget::OnMaxChanged);
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

QString QmitkGroupWidget::GetGroupName() const
{
  return m_GroupBox->title();
}

int QmitkGroupWidget::GetMinSize() const
{
  return m_MinSpin->value();
}

int QmitkGroupWidget::GetMaxSize() const
{
  return m_MaxSpin->value();
}

QColor QmitkGroupWidget::GetColor() const
{
  return m_Color;
}

// ---------------------------------------------------------------------------
// Mutators
// ---------------------------------------------------------------------------

void QmitkGroupWidget::SetAbsoluteMax(int maxVoxels)
{
  m_MinSpin->setMaximum(maxVoxels);
  m_MaxSpin->setMaximum(maxVoxels);
}

void QmitkGroupWidget::SetMatchCount(int count)
{
  m_CountLabel->setText(QString("Components in group: %1").arg(count));
}

void QmitkGroupWidget::SetGroupName(const QString &name)
{
  m_GroupBox->setTitle(name);
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------

void QmitkGroupWidget::OnColorButtonClicked()
{
  QColor chosen = QColorDialog::getColor(m_Color, this, "Select Group Color");
  if (chosen.isValid())
  {
    m_Color = chosen;
    UpdateColorButton();
    emit GroupChanged();
  }
}

void QmitkGroupWidget::OnMinChanged(int value)
{
  // Clamp: min must not exceed max
  if (value > m_MaxSpin->value())
  {
    QSignalBlocker blocker(m_MaxSpin);
    m_MaxSpin->setValue(value);
  }
  emit GroupChanged();
}

void QmitkGroupWidget::OnMaxChanged(int value)
{
  // Clamp: max must not be below min
  if (value < m_MinSpin->value())
  {
    QSignalBlocker blocker(m_MinSpin);
    m_MinSpin->setValue(value);
  }
  emit GroupChanged();
}

void QmitkGroupWidget::OnRemoveClicked()
{
  emit RemoveRequested(this);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void QmitkGroupWidget::UpdateColorButton()
{
  // Paint button background with the selected color
  QString style = QString("QPushButton { background-color: %1; color: %2; border: 1px solid #555; border-radius: 3px; }")
    .arg(m_Color.name())
    .arg(m_Color.lightness() > 128 ? "#000000" : "#ffffff");
  m_ColorBtn->setStyleSheet(style);
}
