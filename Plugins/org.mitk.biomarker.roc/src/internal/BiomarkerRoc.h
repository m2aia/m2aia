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

#include <chrono>
#include <tuple>
#include <utility>
#include <vector>

// for logging purposes
#define ROC_SIG "[BiomarkerRoc] "
/*
struct Timer
{
  Timer() : num_measurements(0), storage(0) { time = std::chrono::high_resolution_clock::now(); }
  ~Timer()
  {
    auto stop_time = std::chrono::high_resolution_clock::now();
    auto start = std::chrono::time_point_cast<std::chrono::microseconds>(time).time_since_epoch().count();
    auto end = std::chrono::time_point_cast<std::chrono::microseconds>(stop_time).time_since_epoch().count();
    auto duration = end - start;
    MITK_INFO << ROC_SIG << "execution took " << duration << " microseconds";
  }
  void start() { time = std::chrono::high_resolution_clock::now(); ++num_measurements; }
  long getDuration() 
  {
    auto stop_time = std::chrono::high_resolution_clock::now();
    auto start = std::chrono::time_point_cast<std::chrono::microseconds>(time).time_since_epoch().count();
    auto end = std::chrono::time_point_cast<std::chrono::microseconds>(stop_time).time_since_epoch().count();
    return end - start;
  }
  long num_measurements;
  long storage;
private:
  std::chrono::_V2::system_clock::time_point time;
};
*/

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
  void DoRocAnalysisMannWhitneyU();
  void DoRocAnalysisWithThresholds();
  std::tuple<std::vector<double>, std::vector<double>, size_t, size_t> PrepareTumorVectors();
  std::tuple<std::vector<std::tuple<double, bool>>, size_t, size_t> GetLabeledMz();
  void AddToTable(double, double);

  static constexpr const double m_Tolerance = 0.45;
  static constexpr const size_t m_numThresholds = 20;
  Ui::BiomarkerRocControls m_Controls;
  mitk::Image::Pointer m_Image;
  const mitk::Label::PixelType *m_MaskData;
  const double *m_ImageData;
  std::size_t m_ImageDataSize;
  //Timer m_timer;
};

#endif // BiomarkerRoc_h
