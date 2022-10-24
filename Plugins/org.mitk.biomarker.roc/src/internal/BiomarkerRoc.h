/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#ifndef BiomarkerRoc_h
#define BiomarkerRoc_h

#include "ui_BiomarkerRocControls.h"
#include <QmitkAbstractView.h>
#include <QmitkSingleNodeSelectionWidget.h>
#include <berryISelectionListener.h>
#include <mitkImage.h>
#include <mitkLabelSetImage.h>
#include <utility>
#include <vector>

QT_CHARTS_USE_NAMESPACE
/**
  \brief BiomarkerRoc
  This class is the main class of the view containing the ROC curve.
  It has two buttons (select file and calculate) and the field for the actual curve
*/
class BiomarkerRoc : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;
  BiomarkerRoc();

protected:
  virtual void CreateQtPartControl(QWidget *parent) override;
  virtual void SetFocus() override;

  /// \brief Called when the user clicks the Calculate button
  void OnButtonCalcPressed();
  /// \brief Called when the user clicks the Render Chart button
  void OnButtonRenderChartPressed();

private:
  void RefreshImageWithNewMz(double mz);
  void AddToTable(double, double);
  std::tuple<std::vector<std::tuple<double, bool>>, size_t, size_t> GetLabeledMz();

  static constexpr const double m_Tolerance = 0.45;
  Ui::BiomarkerRocControls m_Controls;
  mitk::Image::Pointer m_Image;
  const mitk::Label::PixelType *m_MaskData;
  const double *m_ImageData;
  std::size_t m_ImageDataSize;
};

#endif // BiomarkerRoc_h
