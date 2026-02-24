/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkVolcanoView.h"

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

const std::string QmitkVolcanoView::VIEW_ID = "org.mitk.views.m2.docker.volcano";

void QmitkVolcanoView::CreateQtPartControl(QWidget *parent)
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

  auto NodePredicateIsSegmentation = mitk::NodePredicateFunction::New(
    [this](const mitk::DataNode *node) -> bool
    {
      return dynamic_cast<mitk::MultiLabelSegmentation *>(node->GetData()) != nullptr;
    });
  m_Controls.maskSelection->SetDataStorage(this->GetDataStorage());
  m_Controls.maskSelection->SetSelectionIsOptional(false);
  m_Controls.maskSelection->SetEmptyInfo(QStringLiteral("Select a mask/segmentation"));
  m_Controls.maskSelection->SetNodePredicate(NodePredicateIsSegmentation);

  connect(m_Controls.btnRunVolcano, SIGNAL(clicked()), this, SLOT(OnStartVolcano()));
  connect(m_Controls.imageSelection, &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged, 
          this, &QmitkVolcanoView::OnImageSelectionChanged);
  connect(m_Controls.maskSelection, &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged, 
          this, &QmitkVolcanoView::OnMaskSelectionChanged);
  
  UpdateGroupComboBoxes();
}

void QmitkVolcanoView::UpdateGroupComboBoxes()
{
  m_Controls.comboBoxGroup1->clear();
  m_Controls.comboBoxGroup2->clear();
  
  auto selectedNodes = m_Controls.maskSelection->GetSelectedNodes();
  if (!selectedNodes.empty())
  {
    auto node = selectedNodes.front();
    if (auto segmentation = dynamic_cast<mitk::MultiLabelSegmentation *>(node->GetData()))
    {
      auto labelValues = segmentation->GetLabelValuesByGroup(0);
      
      for (auto labelValue : labelValues)
      {
        if (labelValue == 0) continue; // Skip background
        
        QString labelName = QString::number(labelValue);
        auto label = segmentation->GetLabel(labelValue);
        if (label && !label->GetName().empty())
        {
          labelName = QString::fromStdString(label->GetName());
        }
        
        m_Controls.comboBoxGroup1->addItem(labelName, QVariant(QString::number(labelValue)));
        m_Controls.comboBoxGroup2->addItem(labelName, QVariant(QString::number(labelValue)));
      }
      
      if (m_Controls.comboBoxGroup2->count() > 1)
        m_Controls.comboBoxGroup2->setCurrentIndex(1);
    }
  }
}

void QmitkVolcanoView::OnImageSelectionChanged(QmitkSingleNodeSelectionWidget::NodeList)
{
  MITK_INFO << "OnImageSelectionChanged()";
}

void QmitkVolcanoView::OnMaskSelectionChanged(QmitkSingleNodeSelectionWidget::NodeList)
{
  MITK_INFO << "OnMaskSelectionChanged() - Updating group combo boxes";
  UpdateGroupComboBoxes();
}

void QmitkVolcanoView::SetFocus()
{
  m_Controls.btnRunVolcano->setFocus();
}

void QmitkVolcanoView::EnableWidgets(bool enable)
{
  m_Controls.btnRunVolcano->setEnabled(enable);
}

void QmitkVolcanoView::OnStartVolcano()
{
  MITK_INFO << "OnStartVolcano() - Starting Volcano Analysis execution";
  
  if (mitk::DockerHelper::CanRunDocker())
  {
    MITK_INFO << "Docker is available and can run";
    
    if(auto node = m_Controls.imageSelection->GetSelectedNode())
    {
      if (auto image = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
      {
        auto maskNodes = m_Controls.maskSelection->GetSelectedNodes();
        if (maskNodes.empty())
        {
          QMessageBox::warning(nullptr, "No Mask Selected", "Please select a segmentation mask for group comparison.");
          return;
        }
        
        auto maskNode = maskNodes.front();
        auto segmentation = dynamic_cast<mitk::MultiLabelSegmentation *>(maskNode->GetData());
        if (!segmentation)
        {
          QMessageBox::warning(nullptr, "Invalid Mask", "The selected mask is not a valid segmentation.");
          return;
        }
        
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

          MITK_INFO << "Setting up Docker helper with image: ghcr.io/cemos-mannheim/volcanodocker:main";
          mitk::DockerHelper helper("ghcr.io/cemos-mannheim/volcanodocker:main");
          helper.EnableAutoRemoveContainer(true);
          
          MITK_INFO << "Adding " << data.size() << " ion images to Docker helper";
          helper.AddAutoSaveData(data, "--input", "input/feature%1%", ".nrrd");
          
          // Add mask
          auto maskImage = segmentation->GetGroupImage(0);
          MITK_INFO << "Adding mask image to Docker helper";
          helper.AddAutoSaveData(maskImage, "--mask", "mask", ".nrrd");
          
          // Get parameters from UI
          QString group1 = m_Controls.comboBoxGroup1->currentData().toString();
          QString group2 = m_Controls.comboBoxGroup2->currentData().toString();
          MITK_INFO << "Group 1: " << group1.toStdString() << ", Group 2: " << group2.toStdString();
          helper.AddApplicationArgument("--group1", group1.toStdString());
          helper.AddApplicationArgument("--group2", group2.toStdString());
          
          auto alpha = m_Controls.spinBoxAlpha->value();
          MITK_INFO << "Alpha (significance threshold): " << alpha;
          helper.AddApplicationArgument("--alpha", std::to_string(alpha));
          
          auto lfc = m_Controls.spinBoxLFC->value();
          MITK_INFO << "Log2 Fold Change threshold: " << lfc;
          helper.AddApplicationArgument("--lfc", std::to_string(lfc));
          
          auto minRegions = m_Controls.spinBoxMinRegions->value();
          MITK_INFO << "Min regions: " << minRegions;
          helper.AddApplicationArgument("--min_regions", std::to_string(minRegions));
          
          auto bootN = m_Controls.spinBoxBootN->value();
          MITK_INFO << "Bootstrap N: " << bootN;
          helper.AddApplicationArgument("--boot_n", std::to_string(bootN));
          
          auto bootPixels = m_Controls.spinBoxBootPixels->value();
          MITK_INFO << "Bootstrap pixels: " << bootPixels;
          helper.AddApplicationArgument("--boot_pixels", std::to_string(bootPixels));
          
          helper.AddAutoLoadOutput("--output", "volcano_results.csv");
          MITK_INFO << "Docker helper configured - executing container...";
          mitk::ProgressBar::GetInstance()->Progress();

          const auto results = helper.GetResults();
          MITK_INFO << "Docker execution completed - number of results: " << results.size();
          
          if (results.empty())
          {
            MITK_WARN << "No results returned from Docker container";
            QMessageBox::information(nullptr, "Volcano Analysis Complete", 
              "Volcano analysis completed. Results may have been saved to output directory.");
          }
          
          mitk::ProgressBar::GetInstance()->Progress();
          mitk::ProgressBar::GetInstance()->Progress();
          MITK_INFO << "Volcano Analysis execution completed successfully for node: " << node->GetName();
          
          QMessageBox::information(nullptr, "Volcano Analysis Complete", 
            "Volcano analysis completed successfully. Check the output for results.");
        }
        catch (std::exception &e)
        {
          MITK_ERROR << "EXCEPTION CAUGHT in Volcano Analysis execution!";
          MITK_ERROR << "Exception message: " << e.what();
          mitk::ProgressBar::GetInstance()->Reset();
          mitkThrow() << "Running Volcano Analysis failed.\nException: " << e.what();
        }
      }
    }
  }
  else
  {
    MITK_ERROR << "Docker is not available or cannot run! Check Docker installation.";
    QMessageBox::critical(nullptr, "Docker Error", "Docker is not available. Please ensure Docker is installed and running.");
  }
  
  MITK_INFO << "OnStartVolcano() - Execution finished";
}
