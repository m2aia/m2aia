/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkSpatialKMeansView.h"

#include <QMessageBox>
#include <m2IntervalVector.h>
#include <m2SpectrumImage.h>
#include <mitkCoreServices.h>
#include <mitkDockerHelper.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>
#include <mitkImage.h>
#include <mitkLabelSetImage.h>
#include <mitkNodePredicateFunction.h>
#include <mitkProgressBar.h>
#include <usModuleRegistry.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>

const std::string QmitkSpatialKMeansView::VIEW_ID = "org.mitk.views.m2.docker.spatialkmeans";

void QmitkSpatialKMeansView::CreateQtPartControl(QWidget *parent)
{
  m_Controls.setupUi(parent);

  auto NodePredicateIsSpectrumImage = mitk::NodePredicateFunction::New(
    [this](const mitk::DataNode *node) -> bool
    {
      if (auto image = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
        return image->GetImageAccessInitialized();
      return false;
    });

  m_Controls.imageSelection->SetDataStorage(this->GetDataStorage());
  m_Controls.imageSelection->SetSelectionIsOptional(true);
  m_Controls.imageSelection->SetEmptyInfo(QStringLiteral("Select an image"));
  m_Controls.imageSelection->SetNodePredicate(NodePredicateIsSpectrumImage);

  auto NodePredicateIsCentroidList = mitk::NodePredicateFunction::New(
    [this](const mitk::DataNode *node) -> bool
    {
      if (auto centroidList = dynamic_cast<m2::IntervalVector *>(node->GetData()))
        return ((unsigned int)(centroidList->GetType())) & ((unsigned int)(m2::SpectrumFormat::Centroid));
      return false;
    });
  m_Controls.centroidsSelection->SetDataStorage(this->GetDataStorage());
  m_Controls.centroidsSelection->SetSelectionIsOptional(true);
  m_Controls.centroidsSelection->SetEmptyInfo(QStringLiteral("Select centroid list(s)"));
  m_Controls.centroidsSelection->SetNodePredicate(NodePredicateIsCentroidList);

  connect(m_Controls.btnRunSpatialKMeans, SIGNAL(clicked()), this, SLOT(OnStartSpatialKMeans()));
  connect(m_Controls.imageSelection, &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged, 
          this, &QmitkSpatialKMeansView::OnImageSelectionChanged);
}

void QmitkSpatialKMeansView::OnImageSelectionChanged(QmitkSingleNodeSelectionWidget::NodeList)
{
  MITK_INFO << "OnImageSelectionChanged()";
}

void QmitkSpatialKMeansView::SetFocus()
{
  m_Controls.btnRunSpatialKMeans->setFocus();
}

void QmitkSpatialKMeansView::EnableWidgets(bool enable)
{
  m_Controls.btnRunSpatialKMeans->setEnabled(enable);
}

void QmitkSpatialKMeansView::OnStartSpatialKMeans()
{
  MITK_INFO << "OnStartSpatialKMeans() - Starting Spatial K-Means execution";
  
  if (mitk::DockerHelper::CanRunDocker())
  {
    MITK_INFO << "Docker is available and can run";
    
    if(auto node = m_Controls.imageSelection->GetSelectedNode())
    {
      if (auto image = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
      {
        std::vector<mitk::BaseData::Pointer> data;
        try
        {
          mitk::ProgressBar::GetInstance()->AddStepsToDo(3);

          auto *preferencesService = mitk::CoreServices::GetPreferencesService();
          auto *preferences = preferencesService->GetSystemPreferences();
          auto tolerance = preferences->GetFloat("m2aia.signal.Tolerance", 75.0);
          MITK_INFO << "Tolerance value: " << tolerance;
          
          auto centroidNodes = m_Controls.centroidsSelection->GetSelectedNodesStdVector();
          MITK_INFO << "Number of selected centroid nodes: " << centroidNodes.size();
          
          // get current shown image as ion image
          if (centroidNodes.empty())
          {
            MITK_INFO << "No centroid nodes selected - using current displayed image";
            auto newImage = mitk::Image::New();
            newImage->Initialize(image);
            mitk::ImagePixelReadAccessor<m2::DisplayImagePixelType, 3> racc(image);
            mitk::ImagePixelWriteAccessor<m2::DisplayImagePixelType, 3> wacc(newImage);
            auto d = image->GetDimensions();
            MITK_INFO << "Copying image data - total pixels: " << (d[0]*d[1]*d[2]);
            std::copy(racc.GetData(), racc.GetData() + (d[0]*d[1]*d[2]), wacc.GetData());
            data.push_back(newImage);
            MITK_INFO << "Created 1 ion image from current display";
          }
          // create ion image for list of centroids
          else
          {
            MITK_INFO << "Processing centroid nodes to create ion images";
            for (auto centroidNode : centroidNodes)
            {
              MITK_INFO << "Processing centroid node: " << centroidNode->GetName();
              
              if (auto centroids = dynamic_cast<m2::IntervalVector *>(centroidNode->GetData()))
              {
                auto intervals = centroids->GetIntervals();
                MITK_INFO << "Number of intervals in centroid list: " << intervals.size();
                
                for (auto i : intervals)
                {
                  MITK_INFO << "Creating ion image for m/z: " << i.x.mean() << " (tolerance: " << tolerance << ")";
                  auto newImage = mitk::Image::New();
                  newImage->Initialize(image);
                  image->GetImage(i.x.mean(), tolerance, nullptr, newImage);
                  data.push_back(newImage);
                }
              }
              else
              {
                MITK_WARN << "Failed to cast centroid node to IntervalVector: " << centroidNode->GetName();
              }
            }
            MITK_INFO << "Total ion images created: " << data.size();
          }

          MITK_INFO << "Setting up Docker helper with image: ghcr.io/cemos-mannheim/spatialkmeansdocker:latest";
          mitk::DockerHelper helper("ghcr.io/cemos-mannheim/spatialkmeansdocker:latest");
          helper.EnableAutoRemoveContainer(true);
          
          MITK_INFO << "Adding " << data.size() << " ion images to Docker helper";
          // Docker expects --input to be a directory containing .nrrd files
          helper.AddAutoSaveData(data, "--input", "input/feature%1%", ".nrrd");
          
          // Get parameters from UI
          auto k = m_Controls.spinBoxK->value();
          MITK_INFO << "Number of clusters (k): " << k;
          helper.AddApplicationArgument("--k", std::to_string(k));
          
          auto radius = m_Controls.spinBoxRadius->value();
          MITK_INFO << "Smoothing radius (r): " << radius;
          helper.AddApplicationArgument("--r", std::to_string(radius));
          
          helper.AddAutoLoadOutput("--output", "output.nrrd");
          MITK_INFO << "Docker helper configured - executing container...";
          mitk::ProgressBar::GetInstance()->Progress();

          const auto results = helper.GetResults();
          MITK_INFO << "Docker execution completed - number of results: " << results.size();
          
          if (results.empty())
          {
            MITK_ERROR << "No results returned from Docker container!";
            throw std::runtime_error("Docker container returned no results");
          }
          
          MITK_INFO << "Creating MultiLabelSegmentation from results";
          auto lsImage = mitk::MultiLabelSegmentation::New();
          auto resultImage = dynamic_cast<mitk::Image *>(results[0].GetPointer());
          if (!resultImage)
          {
            MITK_ERROR << "First result is not a valid MITK Image!";
            throw std::runtime_error("Invalid result image from Docker");
          }
          
          lsImage->InitializeByLabeledImage(resultImage);
          MITK_INFO << "MultiLabelSegmentation initialized - number of labels: " << lsImage->GetNumberOfLabels(0);
          
          mitk::ProgressBar::GetInstance()->Progress();
          
          // Create result node
          MITK_INFO << "Creating result node";
          auto newNode = mitk::DataNode::New();
          newNode->SetData(lsImage);
          auto nodeName = node->GetName() + "_spatialkmeans_k" + std::to_string(k);
          MITK_INFO << "Result node name: " << nodeName;
          newNode->SetName(nodeName);
          GetDataStorage()->Add(newNode, node);
          RequestRenderWindowUpdate();

          // Assign colors to clusters
          MITK_INFO << "Assigning colors to clusters";
          for (unsigned int i = 1; i <= lsImage->GetNumberOfLabels(0); ++i)
          {
            auto label = lsImage->GetLabel(i);
            if (label)
            {
              // Generate distinct colors for each cluster
              float hue = (i - 1) * 360.0f / k;
              mitk::Color color;
              // Simple HSV to RGB conversion for hue with S=1, V=1
              float c = 1.0f;
              float x = c * (1.0f - std::abs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
              float r, g, b;
              if (hue < 60) { r = c; g = x; b = 0; }
              else if (hue < 120) { r = x; g = c; b = 0; }
              else if (hue < 180) { r = 0; g = c; b = x; }
              else if (hue < 240) { r = 0; g = x; b = c; }
              else if (hue < 300) { r = x; g = 0; b = c; }
              else { r = c; g = 0; b = x; }
              
              color.Set(r, g, b);
              label->SetName("Cluster " + std::to_string(i));
              label->SetColor(color);
              label->SetOpacity(0.3);
              lsImage->UpdateLookupTable(i);
            }
          }
          lsImage->SetActiveLabel(1);
          
          mitk::ProgressBar::GetInstance()->Progress();
          MITK_INFO << "Spatial K-Means execution completed successfully for node: " << node->GetName();
        }
        catch (std::exception &e)
        {
          MITK_ERROR << "EXCEPTION CAUGHT in Spatial K-Means execution!";
          MITK_ERROR << "Exception message: " << e.what();
          mitk::ProgressBar::GetInstance()->Reset();
          mitkThrow() << "Running Spatial K-Means failed.\nException: " << e.what();
        }
      }
    }
  }
  else
  {
    MITK_ERROR << "Docker is not available or cannot run! Check Docker installation.";
    QMessageBox::critical(nullptr, "Docker Error", "Docker is not available. Please ensure Docker is installed and running.");
  }
  
  MITK_INFO << "OnStartSpatialKMeans() - Execution finished";
}
