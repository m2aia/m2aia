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

#include <QmitkAbstractView.h>
#include <berryISelectionListener.h>
#include <berryIPreferences.h>

#include <itkCommand.h>

#include "m2ChartView.h"
#include "m2Crosshair.h"
#include <m2SpectrumImageBase.h>
#include "ui_m2Spectrum.h"
#include "Qm2SpectrumChartDataProvider.h"
#include <qlegendmarker.h>
#include <qscatterseries.h>
#include <qslider.h>



class QMenu;
class QActionGroup;
namespace QtCharts
{
  class QValueAxis;
}
namespace itk{
  template<class T>
  class NodeMemberCommand : public itk::MemberCommand<T>{
    public:

    const mitk::DataNode * m_Node;

    /** Standard class typedefs. */
    typedef NodeMemberCommand        Self;
    typedef itk::SmartPointer< Self >  Pointer;

    /** Method for creation through the object factory. */
    itkNewMacro(Self);

    /** Run-time type information (and related methods). */
    itkTypeMacro(NodeMemberCommand, itk::MemberCommand);

    virtual void SetNode(const mitk::DataNode * node ){m_Node = node;}
      
    void Execute(itk::Object *, const itk::EventObject & event) ITK_OVERRIDE{
      itk::MemberCommand<T>::Execute(m_Node,event);
    }

    void Execute(const itk::Object *, const itk::EventObject & event) ITK_OVERRIDE{
      itk::MemberCommand<T>::Execute(m_Node,event);
    }
  };
}

class m2Spectrum : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:
  static const std::string VIEW_ID;
  ~m2Spectrum() {}

protected:
  virtual void CreateQtPartControl(QWidget *parent) override;
  void CreateQChartView();
  void CreateQChartViewMenu();
  m2Crosshair *m_Crosshair;

  std::map<const mitk::DataNode *, std::shared_ptr<Qm2SpectrumChartDataProvider>> m_DataProvider;
  
  void UpdateLineSeriesWindow(const mitk::DataNode *);
  void UpdateAxisLabels(const mitk::DataNode *, bool remove = false);
  void UpdateZoomLevel(const mitk::DataNode *);
  void UpdateSelectedArea();

protected slots:
  void OnDataNodeReceived(const mitk::DataNode *node);

  void OnMassRangeChanged(qreal x, qreal tol);
  void OnSeriesFocused(const mitk::DataNode *node);
  void OnLegnedHandleMarkerClicked();
  void OnResetView();

  void OnAxisXTicksChanged(int v);
  void OnAxisYTicksChanged(int v);
  // void OnSpectrumArtifactChanged(const mitk::DataNode *, m2::SpectrumType);

  void OnMousePress(QPoint pos, qreal mz, qreal intValue, Qt::MouseButton button, Qt::KeyboardModifiers mod);
  void OnMouseMove(QPoint pos, qreal mz, qreal intValue, Qt::MouseButton button, Qt::KeyboardModifiers mod);
  void OnMouseRelease(QPoint pos, qreal mz, qreal intValue, Qt::MouseButton button, Qt::KeyboardModifiers mod);
  void OnMouseDoubleClick(QPoint pos, qreal mz, qreal intValue, Qt::MouseButton button, Qt::KeyboardModifiers mod);
  void OnMouseWheel(QPoint pos, qreal mz, qreal intValue, int angle, Qt::KeyboardModifiers mod);  

  void OnRangeChangedAxisX(qreal min, qreal max);
  void OnRangeChangedAxisY(qreal min, qreal max);

protected:
  void OnPropertyChanged(const itk::Object *caller, const itk::EventObject &event);
  void OnPeakListChanged(const itk::Object *caller, const itk::EventObject &event);

  unsigned int m_yAxisTicks = 4;
  unsigned int m_xAxisTicks = 9;

  berry::IPreferences::Pointer m_M2aiaPreferences;

  void UpdateGlobalMinMaxValues();
  void UpdateLocalMinMaxValues(double minX, double maxX);
  void SetSelectedAreaStartX(double v) { m_SelectedAreaStartX = v; }
  void SetSelectedAreaEndX(double v) { m_SelectedAreaEndX = v; }
  void DrawSelectedArea();
  void AutoZoomUseLocalExtremaY();


  const std::vector<double> m_Scales = {1.0, 2.0, 4.0, 8.0, 16.0};
  QtCharts::QLineSeries *m_SelectedAreaLeft = nullptr;
  QtCharts::QLineSeries *m_SelectedAreaRight = nullptr;
  QtCharts::QLineSeries *m_SelectedAreaLower = nullptr;
  double m_SelectedAreaStartX = 0;
  double m_SelectedAreaEndX = 0;

  char m_StatusBarTextBuffer[500];
  double m_CurrentMousePosMz = 0;
  double m_CurrentVisibleDataPoints = 0;

  m2::SpectrumType m_CurrentOverviewSpectrumType = m2::SpectrumType::Mean;
  bool m_CurrentOverviewSpectrumTypeChanged = false;

  virtual void SetFocus() override {}
  Ui::imsSpectrumControls m_Controls;

  std::unique_ptr<QtCharts::QScatterSeries> m_LastMzMarker;
  std::vector<std::pair<double, double>> m_AlignmentRegions;

  // virtual void NodeAdded(const mitk::DataNode *node) override;
  virtual void NodeRemoved(const mitk::DataNode *node) override;

  QtCharts::QChart *m_chart = nullptr;
  QtCharts::QAbstractSeries *m_IonImageIndicator = nullptr;

  double m_LastDa = 1;
  double m_LastPPM = 75;

  double m_LastMz = -1;
  double m_LastTol = -1;

  double m_GlobalMinimumY;
  double m_GlobalMaximumY;
  double m_GlobalMaximumX;
  double m_GlobalMinimumX;

  double m_LocalMaximumY;
  double m_LocalMinimumY;

  double m_MouseDragCenterPos = 0;
  double m_MouseDragLowerDelta = 0;
  double m_MouseDragUpperDelta = 0;
  bool m_DraggingActive = false;
  QRectF m_DragBounds = QRectF(0, 0, 0, 0);
  QFuture<void> m_DragFuture;

  bool m_RangeSelectionStarted = false;

private:
  QMenu *m_Menu;
  QAction *m_SpectrumSkyline;
  QAction *m_SpectrumMean;
  QAction *m_SpectrumSum;
  QActionGroup *m_SpectrumSelectionGroup;
  QAction *m_ShowLegend;
  QMenu *m_FocusMenu;
  QAction *m_ShowAxesTitles;
  QSlider *m_TickCountX;
  QSlider *m_TickCountY;
  QtCharts::QValueAxis *m_xAxis;
  QtCharts::QValueAxis *m_yAxis;

  QVector<QString> m_xAxisTitels;
};
