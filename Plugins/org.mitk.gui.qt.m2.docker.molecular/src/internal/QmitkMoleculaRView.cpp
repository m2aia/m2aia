/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkMoleculaRView.h"

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
#include <set>

// Don't forget to initialize the VIEW_ID.
const std::string QmitkMoleculaRView::VIEW_ID = "org.mitk.views.m2.docker.molecular";

void QmitkMoleculaRView::CreateQtPartControl(QWidget *parent)
{
  // Setting up the UI is a true pleasure when using .ui files, isn't it?
  m_Controls.setupUi(parent);

  auto NodePredicateIsSpectrumImage = mitk::NodePredicateFunction::New(
    [this](const mitk::DataNode *node) -> bool
    {
      if (auto image = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
        return image->GetImageAccessInitialized();
      return false;
    });

  // m_Controls.imageSelection->SetAutoSelectNewNodes(true);
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
  m_Controls.centroidsSelection->SetEmptyInfo(QStringLiteral("Select an centroid list"));
  m_Controls.centroidsSelection->SetNodePredicate(NodePredicateIsCentroidList);

  // Initialize mask label combo box with default values
  UpdateMaskLabelComboBox();

  // Wire up the UI widgets with our functionality.
  connect(m_Controls.btnRunMoleculaR, SIGNAL(clicked()), this, SLOT(OnStartMoleculaR()));
  connect(m_Controls.imageSelection, &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged, this, &QmitkMoleculaRView::OnImageSelectionChanged);
}

void QmitkMoleculaRView::UpdateMaskLabelComboBox()
{
  m_Controls.comboBoxMaskLabel->blockSignals(true);
  m_Controls.comboBoxMaskLabel->clear();
  
  // Add fixed entries
  m_Controls.comboBoxMaskLabel->addItem("All Labels (≥1)", QVariant("all"));
  m_Controls.comboBoxMaskLabel->addItem("Background (0)", QVariant("0"));
  
  // Try to get labels from the selected image's mask
  auto selectedNodes = m_Controls.imageSelection->GetSelectedNodes();
  if (!selectedNodes.empty())
  {
    auto node = selectedNodes.front();
    if (auto image = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
    {
      auto segmentation = image->GetMultilabelSegmentation();
      if (segmentation)
      {
        // Get labels from the segmentation for group 0
        auto labelValues = segmentation->GetLabelValuesByGroup(0);
        
        // Add each label to combo box
        for (auto labelValue : labelValues)
        {
          if (labelValue == 0) continue; // Skip background
          
          QString labelName = QString("Label %1").arg(labelValue);
          
          // Try to get the actual label name
          auto label = segmentation->GetLabel(labelValue);
          if (label && !label->GetName().empty())
          {
            labelName = QString("%1 (%2)").arg(QString::fromStdString(label->GetName())).arg(labelValue);
          }
          
          m_Controls.comboBoxMaskLabel->addItem(labelName, QVariant(QString::number(labelValue)));
        }
        
        MITK_INFO << "Updated mask label combo box with " << labelValues.size() << " labels from mask";
      }
    }
  }
  
  m_Controls.comboBoxMaskLabel->blockSignals(false);
}

void QmitkMoleculaRView::OnImageSelectionChanged(QmitkSingleNodeSelectionWidget::NodeList )
{
  MITK_INFO << "OnImageSelectionChanged() - Updating mask label combo box";
  UpdateMaskLabelComboBox();
}

void QmitkMoleculaRView::OnMaskLabelChanged(int )
{
  QString labelValue = m_Controls.comboBoxMaskLabel->currentData().toString();
  MITK_INFO << "OnMaskLabelChanged() - Selected mask label: " << labelValue.toStdString();
}

void QmitkMoleculaRView::SetFocus()
{
  m_Controls.btnRunMoleculaR->setFocus();
}

void QmitkMoleculaRView::EnableWidgets(bool enable)
{
  m_Controls.btnRunMoleculaR->setEnabled(enable);
}

void QmitkMoleculaRView::OnStartMoleculaR()
{
  MITK_INFO << "OnStartMoleculaR() - Starting MoleculaR execution";
  
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

          MITK_INFO << "Setting up Docker helper with image: ghcr.io/m2aia/molecular:latest";
          mitk::DockerHelper helper("ghcr.io/m2aia/molecular:latest");
          helper.EnableAutoRemoveContainer(true);
          
          MITK_INFO << "Adding " << data.size() << " ion images to Docker helper";
          helper.AddAutoSaveData(data, "--ionimage", "ionimages/image%1%", ".nrrd");
          
          auto maskImage = image->GetMultilabelSegmentation()->GetGroupImage(0);
          MITK_INFO << "Adding mask image to Docker helper - mask is " << (maskImage ? "valid" : "null");
          helper.AddAutoSaveData(maskImage, "--mask", "mask", ".nrrd");
          
          auto pValue = m_Controls.spnBxMPMPValue->value();
          MITK_INFO << "P-value threshold: " << pValue;
          helper.AddApplicationArgument("--pval", std::to_string(pValue));
          
          // Get label selection from combo box data
          QString labelArg = m_Controls.comboBoxMaskLabel->currentData().toString();
          MITK_INFO << "Mask label filter: " << labelArg.toStdString();
          helper.AddApplicationArgument("--label", labelArg.toStdString());
          
          helper.AddAutoLoadOutput("--out", "mpm.nrrd");
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
          MITK_INFO << "MultiLabelSegmentation initialized - number of groups: " << lsImage->GetNumberOfGroups();
          MITK_INFO << "Number of labels in group 0: " << lsImage->GetNumberOfLabels(0);
          
          mitk::ProgressBar::GetInstance()->Progress();
          

          
          if (lsImage->GetNumberOfLabels(0) == 2) {
            MITK_INFO << "Valid segmentation detected (2 labels) - creating result node";
            auto newNode = mitk::DataNode::New();
            newNode->SetData(lsImage);
            auto nodeName = node->GetName() + "_mpm_" + std::to_string(image->GetCurrentX());
            MITK_INFO << "Result node name: " << nodeName;
            newNode->SetName(nodeName);
            GetDataStorage()->Add(newNode, node);
            RequestRenderWindowUpdate();

            mitk::Color h;
            h.Set(245 / 255.0, 93 / 255.0, 80 / 255.0);
            // lsImage->Ini
            MITK_INFO << "Configuring label 1 (Hotspots) - color: RGB(245,93,80)";
            lsImage->GetLabel(1)->SetName("Hotspots");
            lsImage->GetLabel(1)->SetColor(h);
            lsImage->GetLabel(1)->SetOpacity(0.1);
            lsImage->UpdateLookupTable(1);
            lsImage->SetActiveLabel(1);
          
            mitk::Color c;
            c.Set(66 / 255.0, 151 / 255.0, 160 / 255.0);
            MITK_INFO << "Configuring label 2 (Coldspots) - color: RGB(66,151,160)";
            lsImage->GetLabel(2)->SetName("Coldspots");
            lsImage->GetLabel(2)->SetColor(c);
            lsImage->GetLabel(2)->SetOpacity(0.1);
            lsImage->UpdateLookupTable(2);  
          }
          else{
            MITK_WARN << "Segmentation has insufficient labels (<=2) - no hotspots/coldspots detected";
            QMessageBox::warning(nullptr, "MoleculaR Warning", "The segmentation maps were empty! No hotspots or coldspots were detected.");
          }
          mitk::ProgressBar::GetInstance()->Progress();
          MITK_INFO << "MoleculaR execution completed successfully for node: " << node->GetName();
        }
        catch (std::exception &e)
        {
          MITK_ERROR << "EXCEPTION CAUGHT in MoleculaR execution!";
          MITK_ERROR << "Exception type: " << typeid(e).name();
          MITK_ERROR << "Exception message: " << e.what();
          
          mitk::ProgressBar::GetInstance()->Reset();
          
          MITK_INFO << "Debug information at time of exception:";
          MITK_INFO << "  - Node name: " << node->GetName();
          MITK_INFO << "  - Image pointer: " << image;
          MITK_INFO << "  - Data vector size: " << data.size();
          if (!data.empty()) {
            MITK_INFO << "  - First data element: " << data[0].GetPointer();
          }
          
          mitkThrow() << "Running MoleculaR failed.\nException: " << e.what();
        }
      }
    }
  }
  else
  {
    MITK_ERROR << "Docker is not available or cannot run! Check Docker installation.";
    QMessageBox::critical(nullptr, "Docker Error", "Docker is not available. Please ensure Docker is installed and running.");
  }
  
  MITK_INFO << "OnStartMoleculaR() - Execution finished";
}
