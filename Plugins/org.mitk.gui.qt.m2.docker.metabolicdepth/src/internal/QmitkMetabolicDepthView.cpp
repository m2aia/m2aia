/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkMetabolicDepthView.h"

#include <QMessageBox>
#include <QtConcurrent>
#include <m2IntervalVector.h>
#include <m2SpectrumImage.h>
#include <m2SpectrumImageHelper.h>
#include <mitkDockerHelper.h>
#include <mitkNodePredicateFunction.h>
#include <mitkProgressBar.h>
#include <mitkLabelSetImage.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <algorithm>
#include <cmath>

const std::string QmitkMetabolicDepthView::VIEW_ID = "org.mitk.views.m2.docker.metabolicdepth";

void QmitkMetabolicDepthView::CreateQtPartControl(QWidget *parent)
{
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

  m_Controls.imageSelection->SetDataStorage(this->GetDataStorage());
  m_Controls.imageSelection->SetSelectionIsOptional(true);
  m_Controls.imageSelection->SetEmptyInfo(QStringLiteral("Select a spectrum image"));
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
  m_Controls.centroidsSelection->SetEmptyInfo(QStringLiteral("Select a centroid list"));
  m_Controls.centroidsSelection->SetNodePredicate(NodePredicateIsCentroidList);

  auto NodePredicateIsLabelSetImage = mitk::NodePredicateFunction::New(
    [this](const mitk::DataNode *node) -> bool
    {
      return dynamic_cast<mitk::MultiLabelSegmentation *>(node->GetData()) != nullptr;
    });
  m_Controls.maskSelection->SetDataStorage(this->GetDataStorage());
  m_Controls.maskSelection->SetSelectionIsOptional(true);
  m_Controls.maskSelection->SetEmptyInfo(QStringLiteral("Select a mask (optional)"));
  m_Controls.maskSelection->SetNodePredicate(NodePredicateIsLabelSetImage);

  // Wire up the UI widgets
  connect(m_Controls.btnRun, SIGNAL(clicked()), this, SLOT(OnStartMetabolicDepthProcessing()));
  connect(m_Controls.btnCancel, SIGNAL(clicked()), this, SLOT(OnCancelProcessing()));
  connect(m_Controls.maskSelection, &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged, 
          this, &QmitkMetabolicDepthView::OnImageSelectionChanged);
  
  // Connect QFutureWatcher signals
  connect(&m_DockerProcessWatcher, &QFutureWatcher<MetabolicDepthResult>::finished, 
          this, &QmitkMetabolicDepthView::OnMetabolicDepthProcessingFinished);
  connect(&m_DockerProcessWatcher, &QFutureWatcher<MetabolicDepthResult>::canceled, 
          this, &QmitkMetabolicDepthView::OnMetabolicDepthProcessingCanceled);
  connect(&m_DockerProcessWatcher, &QFutureWatcher<MetabolicDepthResult>::progressValueChanged, 
          this, [this](int){
    mitk::ProgressBar::GetInstance()->Progress();
  });
}

void QmitkMetabolicDepthView::SetFocus()
{
  m_Controls.btnRun->setFocus();
}

void QmitkMetabolicDepthView::EnableWidgets(bool enable)
{
  m_Controls.btnRun->setEnabled(enable);
  m_Controls.btnCancel->setEnabled(!enable);
  m_Controls.imageSelection->setEnabled(enable);
  m_Controls.centroidsSelection->setEnabled(enable);
  m_Controls.maskSelection->setEnabled(enable);
  m_Controls.spinBoxIsodepths->setEnabled(enable);
  m_Controls.spinBoxEpochs->setEnabled(enable);
  m_Controls.spinBoxRestarts->setEnabled(enable);
  m_Controls.doubleSpinBoxTolerance->setEnabled(enable);
}

void QmitkMetabolicDepthView::UpdateStatusLabel(const QString& message, const QString& styleSheet)
{
  m_Controls.labelStatus->setText(message);
  m_Controls.labelStatus->setStyleSheet(styleSheet);
}

void QmitkMetabolicDepthView::OnImageSelectionChanged(QmitkSingleNodeSelectionWidget::NodeList)
{
  // Update label combo box when mask is selected
  if (auto maskNode = m_Controls.maskSelection->GetSelectedNode())
  {
    if (auto labelSetImage = dynamic_cast<mitk::MultiLabelSegmentation *>(maskNode->GetData()))
    {
      m_Controls.comboBoxMaskLabel->clear();
      
      int lowestNonZeroValue = -1;
      
      // Add all available labels to combo box
      MITK_INFO << "Number of labels in selected mask: " << labelSetImage->GetNumberOfLabels(0);
      for (auto label : labelSetImage->GetLabelValuesByGroup(0)) // Assuming single group (0)
      {
        
        if (label > 0) // Skip background label (0)
        {
          QString labelText = QString::fromStdString(std::to_string(label)) + 
                             " (" + QString::number(label) + ")";
          m_Controls.comboBoxMaskLabel->addItem(labelText, label);
          
          // Track the lowest non-zero value
          if (lowestNonZeroValue == -1 || label < lowestNonZeroValue)
          {
            lowestNonZeroValue = label;
          }
        }
      }
      
      // Set the lowest non-zero value as default
      if (lowestNonZeroValue > 0)
      {
        int index = m_Controls.comboBoxMaskLabel->findData(lowestNonZeroValue);
        if (index >= 0)
        {
          m_Controls.comboBoxMaskLabel->setCurrentIndex(index);
        }
      }
      
      m_Controls.comboBoxMaskLabel->setEnabled(true);
    }
  }
  else
  {
    m_Controls.comboBoxMaskLabel->clear();
    m_Controls.comboBoxMaskLabel->setEnabled(false);
  UpdateStatusLabel("Preparing metabolic depth analysis...", "color: blue;");
  }
}

void QmitkMetabolicDepthView::OnStartMetabolicDepthProcessing()
{
  if (!mitk::DockerHelper::CanRunDocker())
  {
    QMessageBox::warning(nullptr, "Docker Error", 
                        "Docker is not available. Please ensure Docker is installed and running.");
    return;
  }
  
  // Disable the run button while processing
  EnableWidgets(false);
  
  // Get selected nodes
  auto imageNode = m_Controls.imageSelection->GetSelectedNode();
  auto centroidNode = m_Controls.centroidsSelection->GetSelectedNode();
  auto maskNode = m_Controls.maskSelection->GetSelectedNode();
  
  if (!imageNode || !centroidNode)
  {
    QMessageBox::warning(nullptr, "Selection Error", 
                        "Please select both a spectrum image and a centroid list.");
    UpdateStatusLabel("");
    EnableWidgets(true);
    return;
  }
  
  // Capture UI parameters
  const auto n_isodepths = m_Controls.spinBoxIsodepths->value();
  const auto num_epochs = m_Controls.spinBoxEpochs->value();
  const auto num_restarts = m_Controls.spinBoxRestarts->value();
  const auto tolerance = m_Controls.doubleSpinBoxTolerance->value();
  
  // Get mask label if mask is selected
  int maskLabel = -1;
  if (maskNode && m_Controls.comboBoxMaskLabel->count() > 0)
  {
    bool ok = false;
    maskLabel = m_Controls.comboBoxMaskLabel->currentData().toInt(&ok);
    if (!ok || maskLabel <= 0)
    {
      QMessageBox::warning(nullptr, "Selection Error", 
                          "Please select a valid mask label.");
      UpdateStatusLabel("");
      return;
    }
  }
  else if (maskNode)
  {
    QMessageBox::warning(nullptr, "Selection Error", 
                        "The selected mask has no valid labels. Please select a mask with at least one non-background label.");
    EnableWidgets(true);
    UpdateStatusLabel(""  "The selected mask has no valid labels. Please select a mask with at least one non-background label.");
    EnableWidgets(true);
    return;
  }
  UpdateStatusLabel("Running metabolic depth analysis... This may take several minutes.", "color: blue; font-weight: bold;");
  
  // Progress tracking
  mitk::ProgressBar::GetInstance()->AddStepsToDo(3);
  
  // Launch async processing
  m_DockerProcessWatcher.setFuture(QtConcurrent::run([this, imageNode, centroidNode, maskNode, 
                                                        n_isodepths, num_epochs, num_restarts, 
                                                        tolerance, maskLabel]() -> MetabolicDepthResult {
    MetabolicDepthResult result;
    result.referenceNode = imageNode;
    result.maskNode = maskNode;
    
    try
    {
      // Docker image name
      mitk::DockerHelper helper("ghcr.io/m2aia/metabolicdepth:latest");
      m2::SpectrumImageHelper::AddArguments(helper.GetAdditionalApplicationArguments());

      helper.EnableAutoRemoveContainer(true);
      helper.EnableGPUs(true);

      // Add spectrum image
      helper.AddAutoSaveData(imageNode->GetData(), "--imzml", "input", ".imzML");
      
      // Add centroids
      helper.AddAutoSaveData(centroidNode->GetData(), "--centroids", "data", ".centroids");
      
      // Add mask if provided
      if (maskNode)
      {
        helper.AddAutoSaveData(maskNode->GetData(), "--mask", "mask", ".nrrd");
        helper.AddApplicationArgument("--mask_label", std::to_string(maskLabel));
      }
      
      // Add output directory - load both continuous depth and contour segmentation
      helper.AddLoadLaterOutput("--output", "output_folder");
      helper.AddAutoLoadOutput("--out_depthmap", "metabolic_depth.nrrd");
      helper.AddAutoLoadOutput("--out_contours", "metabolic_depth_contours.nrrd");
      
      // Add algorithm parameters
      helper.AddApplicationArgument("--n_isodepths", std::to_string(n_isodepths));
      helper.AddApplicationArgument("--num_epochs", std::to_string(num_epochs));
      helper.AddApplicationArgument("--num_restarts", std::to_string(num_restarts));
      helper.AddApplicationArgument("--tolerance", std::to_string(tolerance));

      mitk::ProgressBar::GetInstance()->Progress();

      // Start processing and get results
      const auto results = helper.GetResults();
      
      if (results.size() >= 2)
      {
        result.depthImage = results[0];      // Continuous depth values
        result.contourImage = results[1];    // Multi-label contour segmentation
      }
      else if (results.size() == 1)
      {
        result.depthImage = results[0];
        MITK_WARN << "Only continuous depth image returned, contour segmentation missing";
      }
      else
      {
        MITK_ERROR << "No results returned from Metabolic Depth Docker container";
      }

      mitk::ProgressBar::GetInstance()->Progress();
    }
    catch (std::exception &e)
    {
      MITK_ERROR << "Running Metabolic Depth failed. Exception: " << e.what();
    }
    
    return result;
  }));
}

void QmitkMetabolicDepthView::OnMetabolicDepthProcessingFinished()
{
  // Check if processing was canceled
  if (m_DockerProcessWatcher.isCanceled())
  {
    return; // Cancellation is handled separately
  }
  
  // Get results from the future
  MetabolicDepthResult result;
  try
  {
    result = m_DockerProcessWatcher.result();
  }
  catch (const std::exception& e)
  {
    MITK_ERROR << "Exception during metabolic depth processing: " << e.what();
    QMessageBox::critical(nullptr, "Processing Error", 
                         QString("An error occurred during processing: %1").arg(e.what()));
    EnableWidgets(true);
    UpdateStatusLabel("Processing failed.", "color: red; font-weight: bold;");
    mitk::ProgressBar::GetInstance()->Reset();
    return;
  }
  catch (...)
  {
    MITK_ERROR << "Unknown exception during metabolic depth processing";
    QMessageBox::critical(nullptr, "Processing Error", 
                         "An unknown error occurred during processing.");
    EnableWidgets(true);
    UpdateStatusLabel("Processing failed.", "color: red; font-weight: bold;");
    mitk::ProgressBar::GetInstance()->Reset();
    return;
  }
  
  // Process results in main thread
  if (result.depthImage && result.referenceNode)
  {
    auto refNode = result.referenceNode;
    auto refImage = dynamic_cast<m2::SpectrumImage *>(refNode->GetData());
    mitk::Image::Pointer depthImage = dynamic_cast<mitk::Image *>(result.depthImage.GetPointer());
    mitk::Image::Pointer contourImage = dynamic_cast<mitk::Image *>(result.contourImage.GetPointer());
    
    if (depthImage && refImage)
    {
      // Copy geometry from reference image
      depthImage->GetGeometry()->SetSpacing(refImage->GetGeometry()->GetSpacing());
      depthImage->GetGeometry()->SetOrigin(refImage->GetGeometry()->GetOrigin());

      // Create node for continuous depth image
      auto depthNode = mitk::DataNode::New();
      depthNode->SetData(depthImage);
      
      // Create meaningful name for continuous depth
      std::string depthNodeName = refNode->GetName() + "_metabolic_depth";
      if (result.maskNode)
      {
        depthNodeName += "_" + result.maskNode->GetName();
      }
      depthNode->SetName(depthNodeName);

      // Add continuous depth to data storage as child of reference image
      GetDataStorage()->Add(depthNode, const_cast<mitk::DataNode *>(refNode.GetPointer()));
      
      // Process contour segmentation if available
      if (contourImage)
      {
        // Copy geometry from reference image
        contourImage->GetGeometry()->SetSpacing(refImage->GetGeometry()->GetSpacing());
        contourImage->GetGeometry()->SetOrigin(refImage->GetGeometry()->GetOrigin());

        // Convert to MultiLabelSegmentation
        auto contourSegmentation = mitk::MultiLabelSegmentation::New();
        try
        {
          contourSegmentation->InitializeByLabeledImage(contourImage);
        }
        catch (mitk::Exception &e)
        {
          MITK_ERROR << "Failed to convert contour image to MultiLabelSegmentation: " << e.GetDescription();
          contourSegmentation = nullptr;
        }

        if (contourSegmentation)
        {
          // Apply isomap-style colormap: blue (label 1) -> cyan -> green -> yellow -> red (label N)
          // using HSV rainbow H: 240 -> 0
          auto isomapColor = [](float t) -> mitk::Color
          {
            float H = (1.0f - t) * 240.0f; // t=0 -> H=240 (blue), t=1 -> H=0 (red)
            const float S = 1.0f, V = 1.0f;
            float C = V * S;
            float X = C * (1.0f - std::abs(std::fmod(H / 60.0f, 2.0f) - 1.0f));
            float m = V - C;
            float r = 0.0f, g = 0.0f, b = 0.0f;
            if      (H < 60.0f)  { r = C; g = X; b = 0; }
            else if (H < 120.0f) { r = X; g = C; b = 0; }
            else if (H < 180.0f) { r = 0; g = C; b = X; }
            else if (H < 240.0f) { r = 0; g = X; b = C; }
            else if (H < 300.0f) { r = X; g = 0; b = C; }
            else                 { r = C; g = 0; b = X; }
            mitk::Color color;
            color.SetRed(r + m);
            color.SetGreen(g + m);
            color.SetBlue(b + m);
            return color;
          };

          // Collect and sort non-background label values
          auto labelValues = contourSegmentation->GetLabelValuesByGroup(0);
          labelValues.erase(
            std::remove(labelValues.begin(), labelValues.end(),
                        mitk::MultiLabelSegmentation::UNLABELED_VALUE),
            labelValues.end());
          std::sort(labelValues.begin(), labelValues.end());

          const int N = static_cast<int>(labelValues.size());
          for (int i = 0; i < N; ++i)
          {
            float t = (N > 1) ? static_cast<float>(i) / static_cast<float>(N - 1) : 0.5f;
            auto color = isomapColor(t);
            auto labelValue = labelValues[i];
            contourSegmentation->RenameLabel(
              labelValue,
              "isodepth-" + std::to_string(labelValue),
              color);
          }

          // Create meaningful name for contours
          std::string contourNodeName = refNode->GetName() + "_metabolic_depth_contours";
          if (result.maskNode)
          {
            contourNodeName += "_" + result.maskNode->GetName();
          }

          auto contourNode = mitk::DataNode::New();
          contourNode->SetData(contourSegmentation);
          contourNode->SetName(contourNodeName);

          // Add contour segmentation to data storage as child of reference image
          GetDataStorage()->Add(contourNode, const_cast<mitk::DataNode *>(refNode.GetPointer()));
        }
      }
      
      mitk::ProgressBar::GetInstance()->Progress();
      UpdateStatusLabel("Processing completed successfully!", "color: green; font-weight: bold;");
      
      QString message = "Metabolic depth analysis completed successfully!\n\n";
      message += "Created:\n";
      message += "- Continuous depth image: " + QString::fromStdString(depthNodeName) + "\n";
      if (contourImage)
      {
        message += "- Contour segmentation: " + QString::fromStdString(depthNodeName) + "_contours";
      }
      
      QMessageBox::information(nullptr, "Metabolic Depth Complete", message);
    }
    else
    {
      UpdateStatusLabel("Processing failed.", "color: red; font-weight: bold;");
      QMessageBox::warning(nullptr, "Processing Error", 
                          "Failed to process metabolic depth results.");
    }
  }
  else
  {
    UpdateStatusLabel("Processing failed.", "color: red; font-weight: bold;");
    QMessageBox::warning(nullptr, "Processing Error", 
                        "Metabolic depth processing did not produce valid results.");
  }
  
  // Re-enable the run button
  EnableWidgets(true);
  
  // Reset progress bar
  mitk::ProgressBar::GetInstance()->Reset();
}

void QmitkMetabolicDepthView::OnMetabolicDepthProcessingCanceled()
{
  UpdateStatusLabel("Processing canceled by user.", "color: orange; font-weight: bold;");
  QMessageBox::information(nullptr, "Processing Canceled", 
                          "Metabolic depth analysis was canceled.");
  
  // Re-enable the run button
  EnableWidgets(true);
  
  // Reset progress bar
  mitk::ProgressBar::GetInstance()->Reset();
}

void QmitkMetabolicDepthView::OnCancelProcessing()
{
  if (m_DockerProcessWatcher.isRunning())
  {
    UpdateStatusLabel("Canceling...", "color: orange;");
    m_DockerProcessWatcher.cancel();
  }
}
