/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkMoleculaRView.h"

#include <QMessageBox>
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>
#include <m2SpectrumImageBase.h>
#include <mitkDockerHelper.h>
#include <mitkImage.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkLabelSetImage.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateOr.h>
#include <mitkNodePredicateProperty.h>
#include <mitkProgressBar.h>
#include <usModuleRegistry.h>

// Don't forget to initialize the VIEW_ID.
const std::string QmitkMoleculaRView::VIEW_ID = "org.mitk.views.m2.docker.molecular";

void QmitkMoleculaRView::CreateQtPartControl(QWidget *parent)
{
  // Setting up the UI is a true pleasure when using .ui files, isn't it?
  m_Controls.setupUi(parent);

  m_Controls.imageSelection->SetAutoSelectNewNodes(true);
  m_Controls.imageSelection->SetDataStorage(this->GetDataStorage());
  m_Controls.imageSelection->SetSelectionIsOptional(true);
  m_Controls.imageSelection->SetEmptyInfo(QStringLiteral("Select an image"));
  m_Controls.imageSelection->SetNodePredicate(mitk::NodePredicateAnd::New(
    mitk::TNodePredicateDataType<mitk::Image>::New(),
    mitk::NodePredicateNot::New(mitk::NodePredicateOr::New(mitk::NodePredicateProperty::New("helper object"),
                                                           mitk::NodePredicateProperty::New("hidden object")))));

  // Wire up the UI widgets with our functionality.
  connect(m_Controls.imageSelection,
          &QmitkSingleNodeSelectionWidget::CurrentSelectionChanged,
          this,
          &QmitkMoleculaRView::OnImageChanged);
  connect(m_Controls.btnRunMoleculaR, SIGNAL(clicked()), this, SLOT(OnStartMoleculaR()));

  // Make sure to have a consistent UI state at the very beginning.
  this->OnImageChanged(m_Controls.imageSelection->GetSelectedNodes());
}

void QmitkMoleculaRView::SetFocus()
{
  m_Controls.btnRunMoleculaR->setFocus();
}

void QmitkMoleculaRView::OnImageChanged(const QmitkSingleNodeSelectionWidget::NodeList &)
{
  this->EnableWidgets(m_Controls.imageSelection->GetSelectedNode().IsNotNull());
}

void QmitkMoleculaRView::EnableWidgets(bool enable)
{
  m_Controls.btnRunMoleculaR->setEnabled(enable);
}

void QmitkMoleculaRView::OnStartMoleculaR()
{
  if (auto node = m_Controls.imageSelection->GetSelectedNode())
  {
    if (auto image = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
    {
      if (!image->GetIsDataAccessInitialized())
        return;

      if (mitk::DockerHelper::CheckDocker())
      {
        mitk::ProgressBar::GetInstance()->AddStepsToDo(4);
        auto newImage = image->Clone();
        newImage->GetPropertyList()->RemoveProperty("MITK.IO.reader.inputlocation");
        mitk::DockerHelper helper("m2aia/extensions:mpm");
        helper.AddData(newImage, "--ionimage", "ionimage.nrrd");
        helper.AddData(image->GetMaskImage(), "--mask", "mask.nrrd");
        helper.AddApplicationArgument("--pval", std::to_string(m_Controls.spnBxMPMPValue->value()));
        helper.AddAutoLoadOutput("--out", "mpm.nrrd");
        mitk::ProgressBar::GetInstance()->Progress();

        const auto results = helper.GetResults();
        auto lsImage = mitk::LabelSetImage::New();
        lsImage->InitializeByLabeledImage(dynamic_cast<mitk::Image *>(results[0].GetPointer()));
        mitk::ProgressBar::GetInstance()->Progress();

        {
          using namespace std;
          mitk::ImagePixelReadAccessor<mitk::LabelSetImage::PixelType> mAcc(image->GetMaskImage());
          mitk::ImagePixelWriteAccessor<mitk::LabelSetImage::PixelType> lsAcc(lsImage);
          auto N = accumulate(lsImage->GetDimensions(), lsImage->GetDimensions() + 3, 1, multiplies<unsigned long>());
          transform(mAcc.GetData(), mAcc.GetData() + N, lsAcc.GetData(), lsAcc.GetData(), [](auto a, auto b) {
            return a > 0 ? b : 0;
          });
        }
        mitk::ProgressBar::GetInstance()->Progress();

        auto newNode = mitk::DataNode::New();
        newNode->SetData(lsImage);
        newNode->SetName(node->GetName() + "_mpm");
        GetDataStorage()->Add(newNode, node);
        lsImage->Update();
        RequestRenderWindowUpdate();

        mitk::Color c;
        c.Set(66 / 255.0, 151 / 255.0, 160 / 255.0);
        lsImage->GetLabel(1)->SetName("Coldspots");
        lsImage->GetLabel(1)->SetColor(c);
        lsImage->GetLabel(1)->SetOpacity(0.1);

        mitk::Color h;
        h.Set(245 / 255.0, 93 / 255.0, 80 / 255.0);
        lsImage->GetLabel(2)->SetName("Hotspots");
        lsImage->GetLabel(2)->SetColor(h);
        lsImage->GetLabel(2)->SetOpacity(0.1);

        lsImage->GetActiveLabelSet()->UpdateLookupTable(1);
        lsImage->GetActiveLabelSet()->UpdateLookupTable(2);
        mitk::ProgressBar::GetInstance()->Progress();
      }
    }
  }
}
