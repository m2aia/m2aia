/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include "m2PeakPickingView.h"

// Qt
#include <QMessageBox>

// m2
#include <m2CommunicationService.h>
#include <m2ImzMLMassSpecImage.h>
#include <m2IonImageReference.h>
#include <m2NoiseEstimators.hpp>
#include <m2PeakDetection.hpp>

// mitk image
#include <mitkImage.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>

const std::string m2PeakPickingView::VIEW_ID = "org.mitk.views.m2.peakpickingview";

void m2PeakPickingView::SetFocus()
{
  //  m_Controls.buttonPerformImageProcessing->setFocus();
}

void m2PeakPickingView::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);

  // auto m_MassSpecPredicate = mitk::TNodePredicateDataType<m2::MSImageBase>::New();
  // m_MassSpecDataNodeSelectionWidget = new QmitkSingleNodeSelectionWidget();
  // m_MassSpecDataNodeSelectionWidget->SetDataStorage(GetDataStorage());
  // m_MassSpecDataNodeSelectionWidget->SetNodePredicate(
  //  mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::MSImageBase>::New(),
  //                              mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  // m_MassSpecDataNodeSelectionWidget->SetSelectionIsOptional(true);
  // m_MassSpecDataNodeSelectionWidget->SetEmptyInfo(QString("Mass spectrometry image"));
  // m_MassSpecDataNodeSelectionWidget->SetPopUpTitel(QString("Select a mass spec. image node"));
  // m_MassSpecDataNodeSelectionWidget->SetAutoSelectNewNodes(true);

  //((QVBoxLayout *)(parent->layout()))->insertWidget(0, m_MassSpecDataNodeSelectionWidget);

  m_Controls.cbOverviewSpectra->addItems({"Skyline", "Mean", "Sum"});
  connect(m_Controls.btnStartPeakPicking, &QCommandLinkButton::clicked, this, &m2PeakPickingView::StartPeakPicking);

  connect(m2::CommunicationService::Instance(),
          &m2::CommunicationService::SendProcessingNodes,
          this,
          &m2PeakPickingView::OnProcessingNodesReceived);
}

void m2PeakPickingView::OnProcessingNodesReceived(const QString &id,
                                                  m2::CommunicationService::NodesVectorType::Pointer nodes)
{
  if (id != VIEW_ID.c_str())
    return;
  m_ReceivedNodes = nodes;

  for (auto node : *m_ReceivedNodes)
    if (auto imageBase = dynamic_cast<m2::ImzMLMassSpecImage *>(node->GetData()))
    {
      if (imageBase->GetSourceList().front().ImportMode != m2::ImzMLFormatType::ContinuousProfile)
      {
        QMessageBox::warning(nullptr, "Warning", "Centroid data are not supported for peak picking operations!");
      }
      else
      {
        std::vector<double> s, m;
        if (m_Controls.cbOverviewSpectra->currentIndex() == 0) // skyline
          s = imageBase->SkylineSpectrum();
        if (m_Controls.cbOverviewSpectra->currentIndex() == 1) // mean
          s = imageBase->MeanSpectrum();
        if (m_Controls.cbOverviewSpectra->currentIndex() == 2) // sum
          s = imageBase->SumSpectrum();

        m = imageBase->MassAxis();

        auto mad = m2::Noise::mad(s);
        std::vector<m2::MassValue> peaks;
        m2::Peaks::localMaxima(std::begin(s),
                               std::end(s),
                               std::begin(m),
                               std::back_inserter(peaks),
                               m_Controls.sbHalfWindowSize->value(),
                               mad * m_Controls.sbSNR->value());
        if (m_Controls.ckbMonoisotopic->isChecked())
        {
          peaks = m2::Peaks::monoisotopic(peaks,
                                          {3, 4, 5, 6, 7, 8, 9, 10},
                                          m_Controls.sbMinCor->value(),
                                          m_Controls.sbTolerance->value(),
                                          m_Controls.sbDistance->value());
        }

        auto &outputvec = imageBase->PeakIndicators();
        outputvec.clear();
        outputvec.resize(imageBase->MassAxis().size(), 0.0);

        auto &maskIndices = imageBase->GetPeaks();
        maskIndices.clear();

        for (auto &p : peaks)
        {
          outputvec[p.massAxisIndex] = 0.0005;
          maskIndices.push_back(p);
        }

        emit m2::CommunicationService::Instance()->OverviewSpectrumChanged(node.GetPointer(),
                                                                           m2::OverviewSpectrumType::PeakIndicators);
      }
    }
}

void m2PeakPickingView::StartPeakPicking()
{
  emit m2::CommunicationService::Instance()->RequestProcessingNodes(QString::fromStdString(VIEW_ID));
}
