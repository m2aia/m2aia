/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkUMAPView.h"

#include <QMessageBox>
#include <QtConcurrent>
#include <m2IntervalVector.h>
#include <m2SpectrumImage.h>
#include <m2SpectrumImageHelper.h>
#include <mitkDockerHelper.h>
#include <mitkNodePredicateFunction.h>
#include <mitkProgressBar.h>
#include <m2MassSpecVisualizationFilter.h>

// Don't forget to initialize the VIEW_ID.
const std::string QmitkUMAPView::VIEW_ID = "org.mitk.views.m2.docker.umap";

void QmitkUMAPView::CreateQtPartControl(QWidget *parent)
{
  // Setting up the UI is a true pleasure when using .ui files, isn't it?
  m_Controls.setupUi(parent);

  auto NodePredicateSpectrumImage = mitk::NodePredicateFunction::New(
    [this](const mitk::DataNode *node) -> bool
    {
      if (auto image = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
      {
        return image->GetImageAccessInitialized();
      }
      return false;
    });

  // m_Controls.imageSelection->SetAutoSelectNewNodes(true);
  m_Controls.imageSelection->SetDataStorage(this->GetDataStorage());
  m_Controls.imageSelection->SetSelectionIsOptional(true);
  m_Controls.imageSelection->SetEmptyInfo(QStringLiteral("Select an image"));
  m_Controls.imageSelection->SetNodePredicate(NodePredicateSpectrumImage);

  auto NodePredicateIsCentroidList = mitk::NodePredicateFunction::New(
    [this](const mitk::DataNode *node) -> bool
    {
      if (auto centroidList = dynamic_cast<m2::IntervalVector *>(node->GetData()))
        return ((unsigned int)(centroidList->GetType())) & ((unsigned int)(m2::SpectrumFormat::Centroid));
      return false;
    });
  m_Controls.centroidsSelection->SetDataStorage(this->GetDataStorage());
  m_Controls.centroidsSelection->SetSelectionIsOptional(true);
  m_Controls.centroidsSelection->SetEmptyInfo(QStringLiteral("Select an centroid list"));
  m_Controls.centroidsSelection->SetNodePredicate(NodePredicateIsCentroidList);

  // Wire up the UI widgets with our functionality.
  connect(m_Controls.btnRun, SIGNAL(clicked()), this, SLOT(OnStartDockerProcessing()));
  
  // Connect QFutureWatcher signals
  connect(&m_DockerProcessWatcher, &QFutureWatcher<void>::finished, this, &QmitkUMAPView::OnDockerProcessingFinished);
  connect(&m_DockerProcessWatcher, &QFutureWatcher<void>::progressValueChanged, this, [this](int){
    mitk::ProgressBar::GetInstance()->Progress();
  });
}

void QmitkUMAPView::SetFocus()
{
  m_Controls.btnRun->setFocus();
}

void QmitkUMAPView::EnableWidgets(bool enable)
{
  m_Controls.btnRun->setEnabled(enable);
}

void QmitkUMAPView::OnStartDockerProcessing()
{
  if (!mitk::DockerHelper::CanRunDocker())
  {
    QMessageBox::warning(nullptr, "Docker Error", "Docker is not available. Please ensure Docker is installed and running.");
    return;
  }
  
  // Disable the run button while processing
  EnableWidgets(false);
  
  // Collect all selected nodes
  auto centroidNodes = m_Controls.centroidsSelection->GetSelectedNodesStdVector();
  auto imageNodes = m_Controls.imageSelection->GetSelectedNodesStdVector();
  
  if (centroidNodes.empty() || imageNodes.empty())
  {
    QMessageBox::warning(nullptr, "Selection Error", "Please select both image(s) and a centroid list.");
    EnableWidgets(true);
    return;
  }
  
  // Capture UI parameters
  const auto n_neighbors = m_Controls.n_neighbors->text().toStdString();
  const auto n_components = m_Controls.n_components->text().toStdString();
  const auto metric = m_Controls.metric->currentText().toStdString();
  const auto n_epochs = m_Controls.n_epochs->text().toStdString();
  const auto learning_rate = m_Controls.learning_rate->text().toStdString();
  const auto min_dist = m_Controls.min_dist->text().toStdString();
  const auto spread = m_Controls.spread->text().toStdString();
  const auto local_connectivity = m_Controls.local_connectivity->text().toStdString();
  const int n_components_value = m_Controls.n_components->value();
  
  // Calculate total steps for progress bar (one step per centroid list)
  const int totalSteps = centroidNodes.size();
  mitk::ProgressBar::GetInstance()->AddStepsToDo(totalSteps);
  
  // Launch async processing
  m_DockerProcessWatcher.setFuture(QtConcurrent::run([this, centroidNodes, imageNodes, n_neighbors, n_components, metric, n_epochs, learning_rate, min_dist, spread, local_connectivity, n_components_value]() -> UMAPResult {
    UMAPResult umapResult;
    umapResult.n_components = n_components_value;
    
    for (auto centroidNode : centroidNodes)
    {
      try
      {
        // docker image name
        mitk::DockerHelper helper("ghcr.io/m2aia/umap:latest");
        m2::SpectrumImageHelper::AddArguments(helper.GetAdditionalApplicationArguments());

        helper.EnableAutoRemoveContainer(true);
        helper.EnableGPUs(false);

        // Collect all image data into a vector
        std::vector<mitk::BaseData::Pointer> imageData;
        for (auto imageNode : imageNodes)
        {
          imageData.push_back(imageNode->GetData());
        }
        
        // Add all images as a vector (creates a folder with numbered files)
        helper.AddAutoSaveData(imageData, "--imzml", "imzml_images/image%1%", ".imzML");
        
        // Add centroids (common for all images)
        helper.AddAutoSaveData(centroidNode->GetData(), "--centroids", "data", ".centroids");
        
        // Prepare output file names for loading
        std::vector<std::string> expectedOutputs;
        for (size_t i = 0; i < imageNodes.size(); ++i)
        {
          expectedOutputs.push_back("umap_" + std::to_string(i) + ".nrrd");
        }
        
        // Add output directory and specify expected files
        helper.AddAutoLoadOutputFolder("--out", "umap_output", expectedOutputs);
        
        helper.AddApplicationArgument("--n_neighbors", n_neighbors);
        helper.AddApplicationArgument("--n_components", n_components);
        helper.AddApplicationArgument("--metric", metric);
        helper.AddApplicationArgument("--n_epochs", n_epochs);
        helper.AddApplicationArgument("--learning_rate", learning_rate);
        helper.AddApplicationArgument("--min_dist", min_dist);
        helper.AddApplicationArgument("--spread", spread);
        helper.AddApplicationArgument("--local_connectivity", local_connectivity);

        // start processing
        const auto results = helper.GetResults();
        
        // Store results and reference nodes for processing in main thread
        if (results.size() != imageNodes.size())
        {
          MITK_ERROR << "Expected " << imageNodes.size() << " output images, got " << results.size();
        }
        else
        {
          // Collect results to be processed in main thread
          umapResult.images.insert(umapResult.images.end(), results.begin(), results.end());
          umapResult.referenceNodes.insert(umapResult.referenceNodes.end(), imageNodes.begin(), imageNodes.end());
        }

        mitk::ProgressBar::GetInstance()->Progress();
      }
      catch (std::exception &e)
      {
        MITK_ERROR << "Running UMAP failed. Exception: " << e.what();
      }
    }
    
    return umapResult;
  }));
}

void QmitkUMAPView::OnDockerProcessingFinished()
{
  // Get results from the future
  UMAPResult umapResult = m_DockerProcessWatcher.result();
  
  // Process results in main thread
  if (umapResult.images.size() == umapResult.referenceNodes.size())
  {
    for (size_t i = 0; i < umapResult.images.size(); ++i)
    {
      auto refNode = umapResult.referenceNodes[i];
      auto refImage = dynamic_cast<mitk::Image *>(refNode->GetData());
      mitk::Image::Pointer image = dynamic_cast<mitk::Image *>(umapResult.images[i].GetPointer());
      
      if (umapResult.n_components == 3)
        image = m2::MassSpecVisualizationFilter::ConvertMitkVectorImageToRGB(image);
      
      image->GetGeometry()->SetSpacing(refImage->GetGeometry()->GetSpacing());
      image->GetGeometry()->SetOrigin(refImage->GetGeometry()->GetOrigin());

      auto newNode = mitk::DataNode::New();
      newNode->SetData(image);
      newNode->SetName(refNode->GetName() + "_umap");

      GetDataStorage()->Add(newNode, const_cast<mitk::DataNode *>(refNode.GetPointer()));
    }
  }
  
  // Re-enable the run button
  EnableWidgets(true);
  
  // Reset progress bar
  mitk::ProgressBar::GetInstance()->Reset();
  
  QMessageBox::information(nullptr, "UMAP Complete", "UMAP processing completed successfully!");
}
