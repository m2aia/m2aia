/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "m2DataPlotView.h"
#include "ui_m2DataPlotView.h"

#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QTemporaryFile>
#include <QUrl>
#include <QDir>
#include <sstream>
#include <fstream>
#include <mitkImageReadAccessor.h>
#include <algorithm>
#include <cmath>
#include <m2HtmlData.h>

const std::string m2DataPlotView::VIEW_ID = "org.mitk.views.m2.dataplot";

m2DataPlotView::m2DataPlotView()
  : m_Controls(nullptr)
{
}

m2DataPlotView::~m2DataPlotView()
{
  delete m_Controls;
}

void m2DataPlotView::CreateQtPartControl(QWidget *parent)
{
  m_Controls = new Ui::m2DataPlotViewControls;
  m_Controls->setupUi(parent);

  // Enable JavaScript in the web view
  if (m_Controls->webView && m_Controls->webView->page())
  {
    auto* settings = m_Controls->webView->page()->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::ErrorPageEnabled, true);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
  }

  // Add plot type options
  m_Controls->plotTypeComboBox->addItem("Box Plot");
  m_Controls->plotTypeComboBox->addItem("Scatter Plot");
  m_Controls->plotTypeComboBox->addItem("Line Plot");
  m_Controls->plotTypeComboBox->addItem("Bar Plot");
  m_Controls->plotTypeComboBox->addItem("Histogram");
  m_Controls->plotTypeComboBox->addItem("Components");
  m_Controls->plotTypeComboBox->addItem("Component Scatter");

  // Connect signals
  connect(m_Controls->plotTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &m2DataPlotView::OnPlotTypeChanged);
  connect(m_Controls->generateButton, &QPushButton::clicked,
          this, &m2DataPlotView::OnGeneratePlot);
  connect(m_Controls->exportButton, &QPushButton::clicked,
          this, &m2DataPlotView::OnExportPlot);
}

void m2DataPlotView::SetFocus()
{
  if (m_Controls)
    m_Controls->generateButton->setFocus();
}

void m2DataPlotView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/,
                                        const QList<mitk::DataNode::Pointer> &nodes)
{
  m_CurrentPlotData = nullptr;
  m_CurrentImage = nullptr;
  m_CurrentHtmlData = nullptr;

  for (const auto &node : nodes)
  {
    if (node.IsNotNull())
    {
      auto *plotData = dynamic_cast<m2::PlotData *>(node->GetData());
      if (plotData)
      {
        m_CurrentPlotData = plotData;
        MITK_INFO << "PlotData selected.";
        UpdatePlotData();
        break;
      }
      
      auto *htmlData = dynamic_cast<m2::HtmlData *>(node->GetData());
      if (htmlData)
      {
        m_CurrentHtmlData = htmlData;
        MITK_INFO << "HTML data selected.";
        UpdateHtmlData();
        break;
      }
      
      auto *image = dynamic_cast<mitk::Image *>(node->GetData());
      if (image)
      {
        m_CurrentImage = image;
        MITK_INFO << "Image data selected.";
        UpdateImageData();
        break;
      }
    }
  }

  if (m_Controls)
  {
    bool hasData = m_CurrentPlotData.IsNotNull() || m_CurrentImage.IsNotNull() || m_CurrentHtmlData.IsNotNull();
    m_Controls->generateButton->setEnabled(hasData);
    m_Controls->exportButton->setEnabled(hasData);
  }
}

void m2DataPlotView::UpdatePlotData()
{
  if (m_CurrentPlotData.IsNull() || !m_Controls)
    return;

  // Update info label
  QString info = QString("Rows: %1 | Numeric Columns: %2 | String Columns: %3")
                   .arg(m_CurrentPlotData->GetNumberOfRows())
                   .arg(m_CurrentPlotData->GetNumberOfColumns())
                   .arg(m_CurrentPlotData->GetStringColumnNames().size());
  m_Controls->dataInfoLabel->setText(info);

  // Auto-generate plot on selection
  OnGeneratePlot();
}

void m2DataPlotView::OnPlotTypeChanged(int /*index*/)
{
  if (m_CurrentPlotData.IsNotNull() || m_CurrentImage.IsNotNull())
  {
    OnGeneratePlot();
  }
  // HtmlData doesn't need regeneration on plot type change
}

void m2DataPlotView::OnGeneratePlot()
{
  if (!m_Controls)
    return;

  if (m_CurrentPlotData.IsNotNull())
  {
    MITK_INFO << "Generating plot for PlotData.";
    GeneratePlotlyHtml(m_CurrentPlotData);
  }
  else if (m_CurrentImage.IsNotNull())
  {
    MITK_INFO << "Generating plot for Image.";
    GenerateImagePlotlyHtml(m_CurrentImage);
  }
  else if (m_CurrentHtmlData.IsNotNull())
  {
    MITK_INFO << "Displaying HTML content.";
    DisplayHtmlContent(m_CurrentHtmlData);
  }
}

void m2DataPlotView::GeneratePlotlyHtml(const m2::PlotData* plotData)
{
  if (!plotData || !m_Controls)
    return;

  std::string htmlContent;
  int plotType = m_Controls->plotTypeComboBox->currentIndex();

  switch (plotType)
  {
  case 0: // Box Plot
    htmlContent = GenerateBoxPlotHtml(plotData);
    break;
  case 1: // Scatter Plot
    htmlContent = GenerateScatterPlotHtml(plotData);
    break;
  case 2: // Line Plot
    htmlContent = GenerateLinePlotHtml(plotData);
    break;
  case 3: // Bar Plot
    htmlContent = GenerateBarPlotHtml(plotData);
    break;
  default:
    htmlContent = GenerateScatterPlotHtml(plotData);
  }

  // Write to temporary file for proper JavaScript execution
  QTemporaryFile *tempFile = new QTemporaryFile(QDir::tempPath() + "/m2aia_plot_XXXXXX.html", this);
  tempFile->setAutoRemove(true);
  
  if (tempFile->open())
  {
    tempFile->write(htmlContent.c_str(), htmlContent.length());
    tempFile->flush();
    m_Controls->webView->setUrl(QUrl::fromLocalFile(tempFile->fileName()));
  }
  
  MITK_INFO << "Plot generated and displayed.";
}

std::string m2DataPlotView::GenerateBoxPlotHtml(const m2::PlotData* plotData)
{
  std::stringstream html;
  
  html << R"(<!DOCTYPE html>
<html>
<head>
  <script src="https://cdn.plot.ly/plotly-2.27.0.min.js"></script>
  <style>
    html, body { margin: 0; padding: 0; font-family: Arial, sans-serif; background-color: #2b2b2b; }
    body { padding: 10px; }
    #plot { width: 100%; height: calc(100vh - 20px); }
  </style>
</head>
<body>
  <div id="plot"></div>
  <script>
    var data = [)";

  auto stringColumns = plotData->GetStringColumnNames();
  auto numericColumns = plotData->GetColumnNames();

  // If we have a string column (categories) and numeric columns (statistics)
  if (!stringColumns.empty() && !numericColumns.empty())
  {
    auto categories = plotData->GetStringColumn(stringColumns[0]);
    
    // Check if we have boxplot statistics columns
    bool hasMin = false, hasQ1 = false, hasMedian = false, hasQ3 = false, hasMax = false;
    for (const auto& col : numericColumns)
    {
      if (col == "min" || col == "minimum") hasMin = true;
      if (col == "q1") hasQ1 = true;
      if (col == "median") hasMedian = true;
      if (col == "q3") hasQ3 = true;
      if (col == "max" || col == "maximum") hasMax = true;
    }

    if (hasMin && hasQ1 && hasMedian && hasQ3 && hasMax)
    {
      // Generate boxplot traces
      for (size_t i = 0; i < categories.size(); ++i)
      {
        if (i > 0) html << ",\n";
        
        auto minVal = plotData->GetColumn("min");
        auto q1Val = plotData->GetColumn("q1");
        auto medianVal = plotData->GetColumn("median");
        auto q3Val = plotData->GetColumn("q3");
        auto maxVal = plotData->GetColumn("max");

        html << "{\n";
        html << "  type: 'box',\n";
        html << "  name: '" << categories[i] << "',\n";
        html << "  y: [" << minVal[i] << ", " << q1Val[i] << ", " << medianVal[i] 
             << ", " << q3Val[i] << ", " << maxVal[i] << "],\n";
        html << "  boxpoints: false,\n";
        html << "  marker: { color: 'rgb(" << (i * 50 % 200) << "," << (i * 80 % 200) 
             << "," << (i * 120 % 200) << ")' }\n";
        html << "}";
      }
    }
  }

  html << R"(
    ];
    
    var layout = {
      title: ')" << plotData->GetDescription() << R"(',
      showlegend: true,
      autosize: true,
      margin: { l: 50, r: 50, t: 50, b: 50 },
      paper_bgcolor: '#2b2b2b',
      plot_bgcolor: '#2b2b2b',
      font: { color: '#ffffff' },
      xaxis: { gridcolor: '#444444', zerolinecolor: '#666666' },
      yaxis: { gridcolor: '#444444', zerolinecolor: '#666666' }
    };
    
    var config = { responsive: true };
    
    Plotly.newPlot('plot', data, layout, config);
  </script>
</body>
</html>)";

  return html.str();
}

std::string m2DataPlotView::GenerateScatterPlotHtml(const m2::PlotData* plotData)
{
  std::stringstream html;
  
  html << R"(<!DOCTYPE html>
<html>
<head>
  <script src="https://cdn.plot.ly/plotly-2.27.0.min.js"></script>
  <style>
    html, body { margin: 0; padding: 0; font-family: Arial, sans-serif; background-color: #2b2b2b; }
    body { padding: 10px; }
    #plot { width: 100%; height: calc(100vh - 20px); }
  </style>
</head>
<body>
  <div id="plot"></div>
  <script>
    var data = [{
      type: 'scatter',
      mode: 'markers',)";

  auto numericColumns = plotData->GetColumnNames();
  if (numericColumns.size() >= 2)
  {
    auto xData = plotData->GetColumn(numericColumns[0]);
    auto yData = plotData->GetColumn(numericColumns[1]);

    html << "\n      x: [";
    for (size_t i = 0; i < xData.size(); ++i)
    {
      if (i > 0) html << ", ";
      html << xData[i];
    }
    html << "],\n      y: [";
    for (size_t i = 0; i < yData.size(); ++i)
    {
      if (i > 0) html << ", ";
      html << yData[i];
    }
    html << "],\n";
    html << "      marker: { size: 8, color: 'rgb(31, 119, 180)' }\n";
  }

  html << R"(    }];
    
    var layout = {
      title: ')" << plotData->GetDescription() << R"(',
      xaxis: { title: ')" << (numericColumns.size() > 0 ? numericColumns[0] : "X") << R"(', gridcolor: '#444444', zerolinecolor: '#666666' },
      yaxis: { title: ')" << (numericColumns.size() > 1 ? numericColumns[1] : "Y") << R"(', gridcolor: '#444444', zerolinecolor: '#666666' },
      autosize: true,
      margin: { l: 50, r: 50, t: 50, b: 50 },
      paper_bgcolor: '#2b2b2b',
      plot_bgcolor: '#2b2b2b',
      font: { color: '#ffffff' }
    };
    
    var config = { responsive: true };
    
    Plotly.newPlot('plot', data, layout, config);
  </script>
</body>
</html>)";

  return html.str();
}

std::string m2DataPlotView::GenerateLinePlotHtml(const m2::PlotData* plotData)
{
  std::stringstream html;
  
  html << R"(<!DOCTYPE html>
<html>
<head>
  <script src="https://cdn.plot.ly/plotly-2.27.0.min.js"></script>
  <style>
    html, body { margin: 0; padding: 0; font-family: Arial, sans-serif; background-color: #2b2b2b; }
    body { padding: 10px; }
    #plot { width: 100%; height: calc(100vh - 20px); }
  </style>
</head>
<body>
  <div id="plot"></div>
  <script>
    var data = [)";

  auto numericColumns = plotData->GetColumnNames();
  if (!numericColumns.empty())
  {
    auto xData = plotData->GetColumn(numericColumns[0]);
    
    for (size_t col = 1; col < numericColumns.size(); ++col)
    {
      if (col > 1) html << ",\n";
      
      auto yData = plotData->GetColumn(numericColumns[col]);
      
      html << "{\n";
      html << "  type: 'scatter',\n";
      html << "  mode: 'lines+markers',\n";
      html << "  name: '" << numericColumns[col] << "',\n";
      html << "  x: [";
      for (size_t i = 0; i < xData.size(); ++i)
      {
        if (i > 0) html << ", ";
        html << xData[i];
      }
      html << "],\n  y: [";
      for (size_t i = 0; i < yData.size(); ++i)
      {
        if (i > 0) html << ", ";
        html << yData[i];
      }
      html << "]\n}";
    }
  }

  html << R"(];
    
    var layout = {
      title: ')" << plotData->GetDescription() << R"(',
      xaxis: { title: ')" << (numericColumns.size() > 0 ? numericColumns[0] : "X") << R"(', gridcolor: '#444444', zerolinecolor: '#666666' },
      yaxis: { title: 'Value', gridcolor: '#444444', zerolinecolor: '#666666' },
      autosize: true,
      margin: { l: 50, r: 50, t: 50, b: 50 },
      paper_bgcolor: '#2b2b2b',
      plot_bgcolor: '#2b2b2b',
      font: { color: '#ffffff' }
    };
    
    var config = { responsive: true };
    
    Plotly.newPlot('plot', data, layout, config);
  </script>
</body>
</html>)";

  return html.str();
}

std::string m2DataPlotView::GenerateBarPlotHtml(const m2::PlotData* plotData)
{
  std::stringstream html;
  
  html << R"(<!DOCTYPE html>
<html>
<head>
  <script src="https://cdn.plot.ly/plotly-2.27.0.min.js"></script>
  <style>
    html, body { margin: 0; padding: 0; font-family: Arial, sans-serif; background-color: #2b2b2b; }
    body { padding: 10px; }
    #plot { width: 100%; height: calc(100vh - 20px); }
  </style>
</head>
<body>
  <div id="plot"></div>
  <script>
    var data = [)";

  auto stringColumns = plotData->GetStringColumnNames();
  auto numericColumns = plotData->GetColumnNames();

  if (!stringColumns.empty() && !numericColumns.empty())
  {
    auto categories = plotData->GetStringColumn(stringColumns[0]);
    
    for (size_t col = 0; col < numericColumns.size(); ++col)
    {
      if (col > 0) html << ",\n";
      
      auto values = plotData->GetColumn(numericColumns[col]);
      
      html << "{\n";
      html << "  type: 'bar',\n";
      html << "  name: '" << numericColumns[col] << "',\n";
      html << "  x: [";
      for (size_t i = 0; i < categories.size(); ++i)
      {
        if (i > 0) html << ", ";
        html << "'" << categories[i] << "'";
      }
      html << "],\n  y: [";
      for (size_t i = 0; i < values.size(); ++i)
      {
        if (i > 0) html << ", ";
        html << values[i];
      }
      html << "]\n}";
    }
  }

  html << R"(];
    
    var layout = {
      title: ')" << plotData->GetDescription() << R"(',
      barmode: 'group',
      autosize: true,
      margin: { l: 50, r: 50, t: 50, b: 50 },
      paper_bgcolor: '#2b2b2b',
      plot_bgcolor: '#2b2b2b',
      font: { color: '#ffffff' },
      xaxis: { gridcolor: '#444444', zerolinecolor: '#666666' },
      yaxis: { gridcolor: '#444444', zerolinecolor: '#666666' }
    };
    
    var config = { responsive: true };
    
    Plotly.newPlot('plot', data, layout, config);
  </script>
</body>
</html>)";

  return html.str();
}

void m2DataPlotView::OnExportPlot()
{
  if ((m_CurrentPlotData.IsNull() && m_CurrentImage.IsNull() && m_CurrentHtmlData.IsNull()) || !m_Controls)
    return;

  QString filename = QFileDialog::getSaveFileName(
    m_Controls->webView,
    "Export Plot",
    QString(),
    "HTML Files (*.html)");

  if (filename.isEmpty())
    return;

  std::string htmlContent;
  int plotType = m_Controls->plotTypeComboBox->currentIndex();

  if (m_CurrentHtmlData.IsNotNull())
  {
    htmlContent = m_CurrentHtmlData->GetHtmlContent();
  }
  else if (m_CurrentPlotData.IsNotNull())
  {
    switch (plotType)
    {
    case 0:
      htmlContent = GenerateBoxPlotHtml(m_CurrentPlotData);
      break;
    case 1:
      htmlContent = GenerateScatterPlotHtml(m_CurrentPlotData);
      break;
    case 2:
      htmlContent = GenerateLinePlotHtml(m_CurrentPlotData);
      break;
    case 3:
      htmlContent = GenerateBarPlotHtml(m_CurrentPlotData);
      break;
    }
  }
  else if (m_CurrentImage.IsNotNull())
  {
    if (plotType == 4 || (plotType < 4 && m_CurrentImage->GetPixelType().GetNumberOfComponents() == 1))
      htmlContent = GenerateHistogramHtml(m_CurrentImage);
    else if (plotType == 5 || (plotType < 4 && m_CurrentImage->GetPixelType().GetNumberOfComponents() > 1))
      htmlContent = GenerateComponentsHtml(m_CurrentImage);
    else if (plotType == 6)
      htmlContent = GenerateComponentScatterHtml(m_CurrentImage);
    else
      htmlContent = GenerateHistogramHtml(m_CurrentImage);
  }

  std::ofstream file(filename.toStdString());
  if (file.is_open())
  {
    file << htmlContent;
    file.close();
    QMessageBox::information(m_Controls->webView, "Export Successful", 
                            "Plot exported to " + filename);
  }
  else
  {
    QMessageBox::warning(m_Controls->webView, "Export Failed", 
                        "Could not write to file: " + filename);
  }
}

void m2DataPlotView::UpdateImageData()
{
  if (m_CurrentImage.IsNull() || !m_Controls)
    return;

  // Update info label
  unsigned int dims = m_CurrentImage->GetDimension();
  unsigned int components = m_CurrentImage->GetPixelType().GetNumberOfComponents();
  
  QString info = QString("Image: %1D | Components: %2 | Type: %3")
                   .arg(dims)
                   .arg(components)
                   .arg(m_CurrentImage->GetPixelType().GetTypeAsString().c_str());
  m_Controls->dataInfoLabel->setText(info);

  // Auto-generate plot on selection
  OnGeneratePlot();
}

void m2DataPlotView::GenerateImagePlotlyHtml(mitk::Image* image)
{
  if (!image || !m_Controls)
    return;

  std::string htmlContent;
  int plotType = m_Controls->plotTypeComboBox->currentIndex();

  // Map plot types: 0-3 are for PlotData, 4-6 for Images
  if (plotType == 4 || (plotType < 4 && image->GetPixelType().GetNumberOfComponents() == 1))
  {
    // Histogram for single component or explicitly selected
    htmlContent = GenerateHistogramHtml(image);
  }
  else if (plotType == 5 || (plotType < 4 && image->GetPixelType().GetNumberOfComponents() > 1))
  {
    // Components view for multi-component
    htmlContent = GenerateComponentsHtml(image);
  }
  else if (plotType == 6)
  {
    // Component scatter (e.g., PC1 vs PC2)
    htmlContent = GenerateComponentScatterHtml(image);
  }
  else
  {
    // Default to histogram
    htmlContent = GenerateHistogramHtml(image);
  }

  MITK_INFO << "Generated HTML content for image plot.";
  
  // Write to temporary file for proper JavaScript execution
  QTemporaryFile *tempFile = new QTemporaryFile(QDir::tempPath() + "/m2aia_plot_XXXXXX.html", this);
  tempFile->setAutoRemove(true);
  
  if (tempFile->open())
  {
    tempFile->write(htmlContent.c_str(), htmlContent.length());
    tempFile->flush();
    m_Controls->webView->setUrl(QUrl::fromLocalFile(tempFile->fileName()));
  }
}

std::vector<std::pair<double, unsigned int>> m2DataPlotView::ComputeHistogram(mitk::Image* image, unsigned int component, unsigned int bins)
{
  std::vector<double> values;
  ExtractComponentData(image, component, values);
  
  if (values.empty())
    return {};

  auto minmax = std::minmax_element(values.begin(), values.end());
  double minVal = *minmax.first;
  double maxVal = *minmax.second;
  double range = maxVal - minVal;
  
  if (range == 0)
    return {{minVal, static_cast<unsigned int>(values.size())}};

  std::vector<unsigned int> counts(bins, 0);
  for (double val : values)
  {
    int bin = static_cast<int>((val - minVal) / range * (bins - 1));
    bin = std::max(0, std::min(static_cast<int>(bins - 1), bin));
    counts[bin]++;
  }

  std::vector<std::pair<double, unsigned int>> histogram;
  for (unsigned int i = 0; i < bins; ++i)
  {
    double binCenter = minVal + (i + 0.5) * range / bins;
    histogram.push_back({binCenter, counts[i]});
  }

  return histogram;
}

void m2DataPlotView::ExtractComponentData(mitk::Image* image, unsigned int component, std::vector<double>& values)
{
  if (!image)
    return;

  try
  {
    auto dims = image->GetDimensions();
    unsigned int numComponents = image->GetPixelType().GetNumberOfComponents();
    
    if (component >= numComponents)
      component = 0;

    // Access raw data buffer
    mitk::ImageReadAccessor accessor(image);
    const void* data = accessor.GetData();
    
    if (!data)
      return;

    // Determine total number of pixels
    size_t totalPixels = dims[0] * dims[1] * dims[2];
    
    // Extract data based on pixel type
    auto pixelType = image->GetPixelType().GetComponentType();
    
    if (pixelType == itk::IOComponentEnum::FLOAT)
    {
      const float* floatData = static_cast<const float*>(data);
      for (size_t i = 0; i < totalPixels; ++i)
      {
        values.push_back(floatData[i * numComponents + component]);
      }
    }
    else if (pixelType == itk::IOComponentEnum::DOUBLE)
    {
      const double* doubleData = static_cast<const double*>(data);
      for (size_t i = 0; i < totalPixels; ++i)
      {
        values.push_back(doubleData[i * numComponents + component]);
      }
    }
    else if (pixelType == itk::IOComponentEnum::SHORT)
    {
      const short* shortData = static_cast<const short*>(data);
      for (size_t i = 0; i < totalPixels; ++i)
      {
        values.push_back(static_cast<double>(shortData[i * numComponents + component]));
      }
    }
    else if (pixelType == itk::IOComponentEnum::USHORT)
    {
      const unsigned short* ushortData = static_cast<const unsigned short*>(data);
      for (size_t i = 0; i < totalPixels; ++i)
      {
        values.push_back(static_cast<double>(ushortData[i * numComponents + component]));
      }
    }
    else if (pixelType == itk::IOComponentEnum::INT)
    {
      const int* intData = static_cast<const int*>(data);
      for (size_t i = 0; i < totalPixels; ++i)
      {
        values.push_back(static_cast<double>(intData[i * numComponents + component]));
      }
    }
    else if (pixelType == itk::IOComponentEnum::UINT)
    {
      const unsigned int* uintData = static_cast<const unsigned int*>(data);
      for (size_t i = 0; i < totalPixels; ++i)
      {
        values.push_back(static_cast<double>(uintData[i * numComponents + component]));
      }
    }
    else if (pixelType == itk::IOComponentEnum::UCHAR)
    {
      const unsigned char* ucharData = static_cast<const unsigned char*>(data);
      for (size_t i = 0; i < totalPixels; ++i)
      {
        values.push_back(static_cast<double>(ucharData[i * numComponents + component]));
      }
    }
  }
  catch (const mitk::Exception& e)
  {
    MITK_ERROR << "Error accessing image data: " << e.what();
  }
}

std::string m2DataPlotView::GenerateHistogramHtml(mitk::Image* image)
{
  std::stringstream html;
  
  html << R"(<!DOCTYPE html>
<html>
<head>
  <script src="https://cdn.plot.ly/plotly-2.27.0.min.js"></script>
  <style>
    html, body { margin: 0; padding: 0; font-family: Arial, sans-serif; background-color: #2b2b2b; }
    body { padding: 10px; }
    #plot { width: 100%; height: calc(100vh - 20px); }
  </style>
</head>
<body>
  <div id="plot"></div>
  <script>
)";

  unsigned int numComponents = image->GetPixelType().GetNumberOfComponents();
  
  html << "    var data = [\n";
  
  for (unsigned int comp = 0; comp < std::min(numComponents, 10u); ++comp)
  {
    auto histogram = ComputeHistogram(image, comp, 100);
    
    if (comp > 0) html << ",\n";
    
    html << "      {\n";
    html << "        type: 'bar',\n";
    html << "        name: 'Component " << comp << "',\n";
    html << "        x: [";
    for (size_t i = 0; i < histogram.size(); ++i)
    {
      if (i > 0) html << ", ";
      html << histogram[i].first;
    }
    html << "],\n        y: [";
    for (size_t i = 0; i < histogram.size(); ++i)
    {
      if (i > 0) html << ", ";
      html << histogram[i].second;
    }
    html << "],\n        opacity: 0.7\n";
    html << "      }";
  }
  
  html << R"(
    ];
    
    var layout = {
      title: 'Grayscale Distribution',
      barmode: 'overlay',
      xaxis: { title: 'Intensity', gridcolor: '#444444', zerolinecolor: '#666666' },
      yaxis: { title: 'Frequency', gridcolor: '#444444', zerolinecolor: '#666666' },
      autosize: true,
      margin: { l: 50, r: 50, t: 50, b: 50 },
      paper_bgcolor: '#2b2b2b',
      plot_bgcolor: '#2b2b2b',
      font: { color: '#ffffff' }
    };
    
    var config = { responsive: true };
    
    Plotly.newPlot('plot', data, layout, config);
  </script>
</body>
</html>)";

  return html.str();
}

std::string m2DataPlotView::GenerateComponentsHtml(mitk::Image* image)
{
  std::stringstream html;
  
  html << R"(<!DOCTYPE html>
<html>
<head>
  <script src="https://cdn.plot.ly/plotly-2.27.0.min.js"></script>
  <style>
    html, body { margin: 0; padding: 0; font-family: Arial, sans-serif; background-color: #2b2b2b; }
    body { padding: 10px; }
    #plot { width: 100%; height: calc(100vh - 20px); }
  </style>
</head>
<body>
  <div id="plot"></div>
  <script>
)";

  unsigned int numComponents = image->GetPixelType().GetNumberOfComponents();
  
  html << "    var data = [\n";
  
  // Line plot showing distribution of each component
  for (unsigned int comp = 0; comp < std::min(numComponents, 10u); ++comp)
  {
    std::vector<double> values;
    ExtractComponentData(image, comp, values);
    
    if (comp > 0) html << ",\n";
    
    html << "      {\n";
    html << "        type: 'box',\n";
    html << "        name: 'PC" << (comp + 1) << "',\n";
    html << "        y: [";
    
    // Sample values for box plot (use every nth value to avoid too much data)
    size_t step = std::max(size_t(1), values.size() / 1000);
    for (size_t i = 0; i < values.size(); i += step)
    {
      if (i > 0) html << ", ";
      html << values[i];
    }
    html << "],\n        boxpoints: false\n";
    html << "      }";
  }
  
  html << R"(
    ];
    
    var layout = {
      title: 'Principal Component Distribution',
      xaxis: { gridcolor: '#444444', zerolinecolor: '#666666' },
      yaxis: { title: 'Value', gridcolor: '#444444', zerolinecolor: '#666666' },
      autosize: true,
      margin: { l: 50, r: 50, t: 50, b: 50 },
      paper_bgcolor: '#2b2b2b',
      plot_bgcolor: '#2b2b2b',
      font: { color: '#ffffff' }
    };
    
    var config = { responsive: true };
    
    Plotly.newPlot('plot', data, layout, config);
  </script>
</body>
</html>)";

  return html.str();
}

std::string m2DataPlotView::GenerateComponentScatterHtml(mitk::Image* image)
{
  std::stringstream html;
  
  html << R"(<!DOCTYPE html>
<html>
<head>
  <script src="https://cdn.plot.ly/plotly-2.27.0.min.js"></script>
  <style>
    html, body { margin: 0; padding: 0; font-family: Arial, sans-serif; background-color: #2b2b2b; }
    body { padding: 10px; }
    #plot { width: 100%; height: calc(100vh - 20px); }
  </style>
</head>
<body>
  <div id="plot"></div>
  <script>
)";

  unsigned int numComponents = image->GetPixelType().GetNumberOfComponents();
  
  if (numComponents >= 2)
  {
    std::vector<double> comp0, comp1;
    ExtractComponentData(image, 0, comp0);
    ExtractComponentData(image, 1, comp1);
    
    // Sample data to avoid overwhelming the browser
    size_t step = std::max(size_t(1), comp0.size() / 5000);
    
    html << "    var data = [{\n";
    html << "      type: 'scatter',\n";
    html << "      mode: 'markers',\n";
    html << "      name: 'PC1 vs PC2',\n";
    html << "      x: [";
    for (size_t i = 0; i < comp0.size(); i += step)
    {
      if (i > 0) html << ", ";
      html << comp0[i];
    }
    html << "],\n      y: [";
    for (size_t i = 0; i < comp1.size(); i += step)
    {
      if (i > 0) html << ", ";
      html << comp1[i];
    }
    html << "],\n      marker: { size: 3, opacity: 0.5, color: 'rgb(31, 119, 180)' }\n";
    html << "    }];\n";
  }
  else
  {
    html << "    var data = [];\n";
  }
  
  html << R"(
    var layout = {
      title: 'Component Scatter Plot (PC1 vs PC2)',
      xaxis: { title: 'PC1', gridcolor: '#444444', zerolinecolor: '#666666' },
      yaxis: { title: 'PC2', gridcolor: '#444444', zerolinecolor: '#666666' },
      autosize: true,
      margin: { l: 50, r: 50, t: 50, b: 50 },
      paper_bgcolor: '#2b2b2b',
      plot_bgcolor: '#2b2b2b',
      font: { color: '#ffffff' }
    };
    
    var config = { responsive: true };
    
    Plotly.newPlot('plot', data, layout, config);
  </script>
</body>
</html>)";

  return html.str();
}

void m2DataPlotView::UpdateHtmlData()
{
  if (!m_Controls || m_CurrentHtmlData.IsNull())
    return;

  // Update info label
  m_Controls->dataInfoLabel->setText("HTML Data loaded. Click 'Generate Plot' to display.");

  // Auto-display the HTML content
  DisplayHtmlContent(m_CurrentHtmlData);
}

void m2DataPlotView::DisplayHtmlContent(const m2::HtmlData *htmlData)
{
  if (!m_Controls || !htmlData)
    return;

  // Get HTML content
  std::string htmlContent = htmlData->GetHtmlContent();
  
  // Write HTML to a temporary file and load it via file:// URL
  // This provides proper security context for JavaScript execution
  QTemporaryFile *tempFile = new QTemporaryFile(QDir::tempPath() + "/m2aia_plot_XXXXXX.html", this);
  tempFile->setAutoRemove(true);
  
  if (tempFile->open())
  {
    tempFile->write(htmlContent.c_str(), htmlContent.length());
    tempFile->flush();
    
    QString filePath = tempFile->fileName();
    QUrl fileUrl = QUrl::fromLocalFile(filePath);
    
    MITK_INFO << "Loading HTML from temporary file: " << filePath.toStdString();
    m_Controls->webView->setUrl(fileUrl);
    
    // Keep the file open during display (it will be auto-removed when deleted)
  }
  else
  {
    MITK_ERROR << "Failed to create temporary HTML file";
  }
}
