/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include "MultiModalMaskExport.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QmitkMultiNodeSelectionWidget.h>
#include <m2CommunicationService.h>
#include <m2SpectrumImageBase.h>
#include <m2MultiModalMaskExportHelper.h>
#include <mitkImage.h>
#include <mitkLabelSetImage.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>

#include <mitkImagePixelReadAccessor.h>

const std::string MultiModalMaskExport::VIEW_ID = "org.mitk.views.m2.MultiModalMaskExport";

void MultiModalMaskExport::SetFocus() {}

void MultiModalMaskExport::CreateQtPartControl(QWidget *parent)
{
  m_Controls.setupUi(parent);

  m_Controls.exportPeaks->setVisible(false);
  m_Controls.exportFullSpectra->setVisible(false);
  m_Controls.exportOptionsLabel->setVisible(false);
  {
    auto m_MassSpecPredicate = mitk::TNodePredicateDataType<m2::SpectrumImageBase>::New();
    m_MassSpecDataNodeSelectionWidget = new QmitkMultiNodeSelectionWidget();
    m_MassSpecDataNodeSelectionWidget->SetDataStorage(GetDataStorage());
    m_MassSpecDataNodeSelectionWidget->SetNodePredicate(
      mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::SpectrumImageBase>::New(),
                                  mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
    m_MassSpecDataNodeSelectionWidget->SetSelectionIsOptional(true);
    m_MassSpecDataNodeSelectionWidget->SetEmptyInfo(QString("MS image selection"));
    m_MassSpecDataNodeSelectionWidget->SetPopUpTitel(QString("MS image"));
    ((QVBoxLayout *)(parent->layout()))->insertWidget(1, m_MassSpecDataNodeSelectionWidget);
  }

  {
    auto maskPredicate = mitk::TNodePredicateDataType<mitk::LabelSetImage>::New();
    m_MaskImageSelectionWidget = new QmitkMultiNodeSelectionWidget();
    m_MaskImageSelectionWidget->SetDataStorage(GetDataStorage());
    m_MaskImageSelectionWidget->SetNodePredicate(
      mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::LabelSetImage>::New(),
                                  mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
    m_MaskImageSelectionWidget->SetSelectionIsOptional(true);
    m_MaskImageSelectionWidget->SetEmptyInfo(QString("Mask image selection"));
    m_MaskImageSelectionWidget->SetPopUpTitel(QString("Mask image"));
    ((QVBoxLayout *)(parent->layout()))->insertWidget(2, m_MaskImageSelectionWidget);
    m_MaskImageSelectionWidget->setMinimumHeight(20);
  }

  connect(m_MassSpecDataNodeSelectionWidget,
          &QmitkAbstractNodeSelectionWidget::CurrentSelectionChanged,
          this,
          [&](QList<mitk::DataNode::Pointer> nodesList) {
            bool isPeakOptionVisible = true;
            for (auto const &node : nodesList)
            {
              if (dynamic_cast<m2::SpectrumImageBase *>(node->GetData())->GetPeaks().empty())
              {
                isPeakOptionVisible = false;
                break;
              }
            }
            this->SetSelectionVisibility(isPeakOptionVisible);
          });

  connect(m_Controls.exportButton, &QPushButton::clicked, this, &MultiModalMaskExport::Export);
}

void MultiModalMaskExport::Export()
{
  auto maskNodes = m_MaskImageSelectionWidget->GetSelectedNodes();
  auto selectedNodes = m_MassSpecDataNodeSelectionWidget->GetSelectedNodes();
  auto helper = m2::MultiModalMaskExportHelper::New();

  for (auto maskNode : maskNodes)
  {
    helper->AddMaskNode(maskNode);
  }

  for (auto node : selectedNodes)
  {
    helper->AddMsiDataNode(node);
  }

  helper->SetLowerMzBound(m_Controls.lowerBoundSpinBox->value());
  helper->SetUpperMzBound(m_Controls.upperBoundSpinBox->value());
  helper->SetExportOption(m_Controls.exportFullSpectra->isChecked(), m_Controls.exportBackground->isChecked());

  QString filename = QFileDialog::getSaveFileName(nullptr, tr("Save spectra"), "", tr("CSV (*.csv)"));
  helper->SetFilePath(filename.toStdString());
  helper->Export();
}

void MultiModalMaskExport::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/,
                                              const QList<mitk::DataNode::Pointer> &nodes)
{
  // iterate all selected objects, adjust warning visibility
  foreach (mitk::DataNode::Pointer node, nodes)
  {
    if (node.IsNotNull() && dynamic_cast<mitk::Image *>(node->GetData()))
    {
      return;
    }
  }
}

void MultiModalMaskExport::SetSelectionVisibility(bool isSelectionVisible)
{
  m_Controls.exportPeaks->setVisible(isSelectionVisible);
  m_Controls.exportFullSpectra->setVisible(isSelectionVisible);
  m_Controls.exportOptionsLabel->setVisible(isSelectionVisible);
}
