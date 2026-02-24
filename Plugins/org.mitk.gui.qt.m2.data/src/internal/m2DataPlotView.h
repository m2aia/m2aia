/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef m2DataPlotView_h
#define m2DataPlotView_h

#include <QmitkAbstractView.h>
#include <berryISelectionListener.h>
#include <m2PlotData.h>
#include <m2HtmlData.h>
#include <mitkImage.h>

namespace Ui
{
  class m2DataPlotViewControls;
}

class m2DataPlotView : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;

  m2DataPlotView();
  ~m2DataPlotView() override;

protected:
  void CreateQtPartControl(QWidget *parent) override;
  void SetFocus() override;

  void OnSelectionChanged(berry::IWorkbenchPart::Pointer source,
                          const QList<mitk::DataNode::Pointer> &nodes) override;

private slots:
  void OnPlotTypeChanged(int index);
  void OnGeneratePlot();
  void OnExportPlot();

private:
  void UpdatePlotData();
  void UpdateImageData();
  void UpdateHtmlData();
  void GeneratePlotlyHtml(const m2::PlotData* plotData);
  void GenerateImagePlotlyHtml(mitk::Image* image);
  void DisplayHtmlContent(const m2::HtmlData* htmlData);
  
  // PlotData visualization methods
  std::string GenerateBoxPlotHtml(const m2::PlotData* plotData);
  std::string GenerateScatterPlotHtml(const m2::PlotData* plotData);
  std::string GenerateLinePlotHtml(const m2::PlotData* plotData);
  std::string GenerateBarPlotHtml(const m2::PlotData* plotData);
  
  // Image visualization methods
  std::string GenerateHistogramHtml(mitk::Image* image);
  std::string GenerateComponentsHtml(mitk::Image* image);
  std::string GenerateComponentScatterHtml(mitk::Image* image);
  
  // Helper methods
  std::vector<std::pair<double, unsigned int>> ComputeHistogram(mitk::Image* image, unsigned int component, unsigned int bins = 100);
  void ExtractComponentData(mitk::Image* image, unsigned int component, std::vector<double>& values);

  Ui::m2DataPlotViewControls *m_Controls;
  m2::HtmlData::Pointer m_CurrentHtmlData;
  m2::PlotData::Pointer m_CurrentPlotData;
  mitk::Image::Pointer m_CurrentImage;
};

#endif // m2DataPlotView_h
