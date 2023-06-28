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
#include "ui_Qm2PeakListStatisitcsWidget.h"
#include <QWidget>
#include <itkSmartPointer.h>
#include <m2IntervalVector.h>
#include <mitkLabelSetImage.h>

/**
 * \brief This class provides statistics for a SpectrumImage-LabelSetImage-PeakList triplet.
 */
class Qm2PeakListStatisitcsWidget : public QWidget
{
  Q_OBJECT

public:
  Qm2PeakListStatisitcsWidget(QWidget *parent = nullptr);

  /**
   *
   */
  mitk::LabelSetImage::Pointer GetSpectrumImage() const;

  /**
   *
   */
  mitk::LabelSetImage::Pointer GetLabelSetImage() const;

  /**
   *
   */
  m2::IntervalVector::Pointer GetIntervalVector() const;

  /**
   *
   */
  std::string GetStatistics() const;

private:
  Ui::Qm2PeakListStatisticsWidget m_Controls;

  void UpdateIntervalList();
  void UpdateStatistics();
};
