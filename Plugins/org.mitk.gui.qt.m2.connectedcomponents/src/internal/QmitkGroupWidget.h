/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#pragma once

#include <QWidget>
#include <QColor>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>

/**
 * @brief Widget representing a single size group for connected component analysis.
 *
 * Each group defines a voxel-count range [minSize, maxSize]. Labels (connected
 * components) whose voxel count falls within that range are assigned the chosen
 * group color. A label showing the currently matched component count is updated
 * by the parent view.
 */
class QmitkGroupWidget : public QWidget
{
  Q_OBJECT

public:
  explicit QmitkGroupWidget(const QString &groupName,
                            int minSize,
                            int maxSize,
                            const QColor &color,
                            int absoluteMax,
                            QWidget *parent = nullptr);

  ~QmitkGroupWidget() override = default;

  // ---- Accessors ----
  QString GetGroupName() const;
  int     GetMinSize()   const;
  int     GetMaxSize()   const;
  QColor  GetColor()     const;

  // ---- Mutators ----
  void SetAbsoluteMax(int maxVoxels);
  void SetMatchCount(int count);
  void SetGroupName(const QString &name);

signals:
  void GroupChanged();
  void RemoveRequested(QmitkGroupWidget *widget);

private slots:
  void OnColorButtonClicked();
  void OnMinChanged(int value);
  void OnMaxChanged(int value);
  void OnRemoveClicked();

private:
  void UpdateColorButton();

  QGroupBox  *m_GroupBox    = nullptr;
  QPushButton *m_ColorBtn   = nullptr;
  QSpinBox   *m_MinSpin     = nullptr;
  QSpinBox   *m_MaxSpin     = nullptr;
  QLabel     *m_CountLabel  = nullptr;
  QPushButton *m_RemoveBtn  = nullptr;

  QColor      m_Color;
};
