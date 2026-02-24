/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkPeakLearningView.h"

#include <QMessageBox>
#include <m2SpectrumImage.h>
#include <m2SpectrumImageHelper.h>
#include <mitkDockerHelper.h>
#include <mitkLabelSetImage.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateFunction.h>
#include <mitkProgressBar.h>

// Don't forget to initialize the VIEW_ID.
const std::string QmitkPeakLearningView::VIEW_ID = "org.mitk.views.m2.docker.peaklearning";

void QmitkPeakLearningView::CreateQtPartControl(QWidget *parent)
{
  // Setting up the UI is a true pleasure when using .ui files, isn't it?
  m_Controls.setupUi(parent);

  auto NodePredicateIsContinuousSpectrumImage = mitk::NodePredicateFunction::New(
    [this](const mitk::DataNode *node) -> bool
    {
      if (auto spectrumImage = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
        return ((unsigned int)(spectrumImage->GetSpectrumType().Format)) &
               ((unsigned int)(m2::SpectrumFormat::Continuous));
      return false;
    });

  // m_Controls.imageSelection->SetAutoSelectNewNodes(true);
  m_Controls.imageSelection->SetDataStorage(this->GetDataStorage());
  m_Controls.imageSelection->SetSelectionIsOptional(true);
  m_Controls.imageSelection->SetEmptyInfo(QStringLiteral("Select an image"));
  m_Controls.imageSelection->SetNodePredicate(NodePredicateIsContinuousSpectrumImage);

  m_Controls.maskSelection->SetDataStorage(this->GetDataStorage());
  m_Controls.maskSelection->SetSelectionIsOptional(true);
  m_Controls.maskSelection->SetEmptyInfo(QStringLiteral("Select a mask image"));
  m_Controls.maskSelection->SetNodePredicate(mitk::TNodePredicateDataType<mitk::MultiLabelSegmentation>::New());

  // Wire up the UI widgets with our functionality.
  connect(m_Controls.btnRun, SIGNAL(clicked()), this, SLOT(OnStartDockerProcessing()));
}

void QmitkPeakLearningView::SetFocus()
{
  m_Controls.btnRun->setFocus();
}

void QmitkPeakLearningView::EnableWidgets(bool enable)
{
  m_Controls.btnRun->setEnabled(enable);
}

void QmitkPeakLearningView::OnStartDockerProcessing()
{
  if (mitk::DockerHelper::CanRunDocker())
  {
    for (auto node : m_Controls.imageSelection->GetSelectedNodes())
    {
      if (auto image = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
      {
        if (!image->GetImageAccessInitialized())
          return;

        mitk::Image::Pointer mask;
        std::vector<std::string> expectedOutputs;
        
        if (auto maskNode = m_Controls.maskSelection->GetSelectedNode())
        {  
          if (auto maskImage = dynamic_cast<mitk::Image *>(maskNode->GetData()))
          {
            mask = maskImage;
          }
          else if (auto lsi = dynamic_cast<mitk::MultiLabelSegmentation *>(maskNode->GetData()))
          {  
            // Get labels from the segmentation for group 0
            auto labelValues = lsi->GetLabelValuesByGroup(0);
            mask = lsi->GetGroupImage(0);
            if(labelValues.empty())
            {
              MITK_WARN << "Selected mask segmentation has no labels in group 0!";
              mitkThrow() << "Selected mask segmentation has no labels in group 0!";
            }
            MITK_INFO << "Expected peaks_"<< labelValues[0]  << ".csv" ;
            std::transform(labelValues.begin(), labelValues.end(), std::back_inserter(expectedOutputs), [](auto val) { return "peaks_" + std::to_string(val) + ".csv"; });
          }
        }
        else
        {
          MITK_WARN << "No mask image selected! Please select a mask image.";
          QMessageBox::warning(nullptr, "Missing Mask Image", "No mask image selected! Please select a mask image.");
          return;
        }

        try
        {
          mitk::ProgressBar::GetInstance()->AddStepsToDo(3);

          mitk::DockerHelper helper("ghcr.io/m2aia/msipl:latest");
          m2::SpectrumImageHelper::AddArguments(helper.GetAdditionalApplicationArguments());
          
          helper.EnableGPUs(true);
          helper.EnableAutoRemoveContainer(true);

          helper.AddAutoSaveData(image, "--imzml", "processData", ".imzML");
          helper.AddAutoSaveData(mask, "--mask", "mask", ".nrrd");
          helper.AddApplicationArgument("--latent_dim", m_Controls.latent_dim->text().toStdString());
          helper.AddApplicationArgument("--interim_dim", m_Controls.interim_dim->text().toStdString());
          helper.AddApplicationArgument("--beta", m_Controls.beta->text().toStdString());
          helper.AddApplicationArgument("--epochs", m_Controls.epochs->text().toStdString());
          helper.AddApplicationArgument("--batch_size", m_Controls.batch_size->text().toStdString());

          helper.AddAutoLoadOutputFolder("--output", "output", expectedOutputs);
          helper.AddLoadLaterOutput("--model", "model.weights.h5");

          mitk::ProgressBar::GetInstance()->Progress();
          const auto results = helper.GetResults();
          
          
          if(results.empty())
          {
            MITK_ERROR << "No results returned from Docker container!";
            mitkThrow() << "Docker container returned no results";
          }

          for(unsigned int  i = 0 ; i < results.size(); ++i)
          {
            auto newNode = mitk::DataNode::New();
            newNode->SetData(results[i]);
            newNode->SetName(expectedOutputs[i]);
            GetDataStorage()->Add(newNode, node);
            mitk::ProgressBar::GetInstance()->Progress();
            m2::CopyNodeProperties(node, newNode);
          }


          mitk::ProgressBar::GetInstance()->Progress();
        }
        catch (std::exception &e)
        {
          mitk::ProgressBar::GetInstance()->Reset();
          mitkThrow() << "Running PeakLearning failed.\nException: " << e.what();
        }
      }
    }
  }
}
