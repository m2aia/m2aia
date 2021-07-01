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
#include <m2ElxUtil.h>
// Qmitk
#include "m2PeakPickingView.h"

// Qt
#include <QFileDialog>
#include <QMessageBox>

// m2
#include <m2CommunicationService.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2IonImageReference.h>
#include <m2MedianAbsoluteDeviation.h>
#include <m2MultiSliceFilter.h>
#include <m2PcaImageFilter.h>
#include <m2PeakDetection.h>
#include <m2TSNEImageFilter.h>

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
  m_Controls.cbOverviewSpectra->addItems({"Skyline", "Mean", "Sum"});

  connect(
    m_Controls.btnStartPeakPicking, &QCommandLinkButton::clicked, this, &m2PeakPickingView::OnRequestProcessingNodes);

  connect(m_Controls.btnPCA, &QCommandLinkButton::clicked, this, &m2PeakPickingView::OnStartPCA);
  connect(m_Controls.btnTSNE, &QCommandLinkButton::clicked, this, &m2PeakPickingView::OnStartTSNE);

  connect(m2::CommunicationService::Instance(),
          &m2::CommunicationService::SendProcessingNodes,
          this,
          &m2PeakPickingView::OnProcessingNodesReceived);

  
  const auto itemHandler = [this](QTableWidgetItem *item)
  {
    const size_t idx = m_Controls.imageSource->currentIndex();

    if (m_ReceivedNodes->empty())
      return;
    if (idx >= m_ReceivedNodes->size())
      return;

    auto node = m_ReceivedNodes->at(idx);
    if (auto spImage = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
    {
      auto mz = std::stod(item->text().toStdString());
      emit m2::CommunicationService::Instance()->UpdateImage(mz, spImage->ApplyTolerance(mz));
    }
  };

  connect(m_Controls.tableWidget, &QTableWidget::itemActivated, this, itemHandler);

  connect(m_Controls.tableWidget, &QTableWidget::itemDoubleClicked, this, itemHandler);
}

void m2PeakPickingView::OnRequestProcessingNodes()
{
  emit m2::CommunicationService::Instance()->RequestProcessingNodes(QString::fromStdString(VIEW_ID));
}

void m2PeakPickingView::OnProcessingNodesReceived(const QString &id,
                                                  m2::CommunicationService::NodesVectorType::Pointer nodes)
{
  if (id != VIEW_ID.c_str())
    return;
  m_ReceivedNodes = nodes;

  if (m_Connection)
    disconnect(m_Connection);
  m_Controls.imageSource->clear();
  m_PeakLists.clear();

  for (auto node : *m_ReceivedNodes)
    if (auto imageBase = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
    {
      m_Controls.imageSource->addItem(node->GetName().c_str());
      if (imageBase->GetImportMode() != m2::SpectrumFormatType::ContinuousProfile)
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

        m = imageBase->GetXAxis();

        auto mad = m2::Signal::mad(s);
        std::vector<m2::MassValue> peaks;
        m2::Signal::localMaxima(std::begin(s),
                                std::end(s),
                                std::begin(m),
                                std::back_inserter(peaks),
                                m_Controls.sbHalfWindowSize->value(),
                                mad * m_Controls.sbSNR->value());
        if (m_Controls.ckbMonoisotopic->isChecked())
        {
          peaks = m2::Signal::monoisotopic(peaks,
                                           {3, 4, 5, 6, 7, 8, 9, 10},
                                           m_Controls.sbMinCor->value(),
                                           m_Controls.sbTolerance->value(),
                                           m_Controls.sbDistance->value());
        }

        // visualization
        auto &outputvec = imageBase->PeakIndicators();
        outputvec.clear();
        outputvec.resize(imageBase->GetXAxis().size(), 0.0);

        // true peak list
        auto &peakList = imageBase->GetPeaks();
        peakList.clear();

        for (auto &p : peaks)
        {
          outputvec[p.massAxisIndex] = 0.0005;
          peakList.push_back(p);

          // m_Controls.tableWidget->setItem()
        }

        m_PeakLists.push_back(peakList);

        emit m2::CommunicationService::Instance()->OverviewSpectrumChanged(node.GetPointer(),
                                                                           m2::OverviewSpectrumType::PeakIndicators);
      }
    }
  OnImageSelectionChangedUpdatePeakList(0);
  m_Connection = connect(
    m_Controls.imageSource, SIGNAL(currentIndexChanged(int)), this, SLOT(OnImageSelectionChangedUpdatePeakList(int)));
  emit m2::CommunicationService::Instance()->SendProcessingNodes("", m_ReceivedNodes);
}

void m2PeakPickingView::OnImageSelectionChangedUpdatePeakList(int idx)
{
  if (!m_PeakLists.empty() && idx < (int)m_PeakLists.size())
  {
    auto &peaks = m_PeakLists.at(idx);
    m_Controls.tableWidget->clearContents();
    m_Controls.tableWidget->setRowCount(peaks.size());
    m_Controls.labelPaklist->setText(QString("Peaks list (#%1)").arg((int)peaks.size()));
    unsigned int row = 0;
    m_Controls.tableWidget->blockSignals(true);
    for (auto &p : peaks)
    {
      auto item = new QTableWidgetItem(std::to_string(p.mass).c_str());
      item->setCheckState(Qt::CheckState::Checked);
      m_Controls.tableWidget->setItem(row++, 0, item);
    }
    m_Controls.tableWidget->blockSignals(false);
  }
}

void m2PeakPickingView::OnStartPCA()
{
  for (auto node : *m_ReceivedNodes)
  {
    if (auto imageBase = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
    {
      auto filter = m2::PcaImageFilter::New();
      filter->SetNumberOfComponents(3);
      filter->SetMaskImage(imageBase->GetMaskImage());
      const auto &peakList = imageBase->GetPeaks();
      size_t index = 0;

      std::vector<mitk::Image::Pointer> bufferedImages(peakList.size());
      for (auto &p : peakList)
      {
        bufferedImages[index] = mitk::Image::New();
        bufferedImages[index]->Initialize(imageBase);

        imageBase->UpdateImage(
          p.mass, imageBase->ApplyTolerance(p.mass), imageBase->GetMaskImage(), bufferedImages[index]);
        filter->SetInput(index, bufferedImages[index]);
        ++index;
      }
      filter->Update();

      auto outputNode = mitk::DataNode::New();
      auto data = m2::MultiSliceFilter::ConvertMitkVectorImageToRGB(filter->GetOutput());
      outputNode->SetData(data);

      outputNode->SetName("PCA");
      node->SetVisibility(false);
      this->GetDataStorage()->Add(outputNode, node.GetPointer());
    }
  }
}

void m2PeakPickingView::OnStartTSNE()
{
  for (auto node : *m_ReceivedNodes)
  {
    if (auto imageBase = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
    {
      auto filter = m2::TSNEImageFilter::New();
      filter->SetNumberOfComponents(3);
      filter->SetMaskImage(imageBase->GetMaskImage());
      const auto &peakList = imageBase->GetPeaks();
      size_t index = 0;

      std::vector<mitk::Image::Pointer> bufferedImages(peakList.size());
      for (auto &p : peakList)
      {
        bufferedImages[index] = mitk::Image::New();
        bufferedImages[index]->Initialize(imageBase);

        imageBase->UpdateImage(
          p.mass, imageBase->ApplyTolerance(p.mass), imageBase->GetMaskImage(), bufferedImages[index]);
        filter->SetInput(index, bufferedImages[index]);
        ++index;
      }
      filter->Update();

      auto outputNode = mitk::DataNode::New();
      auto data = m2::MultiSliceFilter::ConvertMitkVectorImageToRGB(filter->GetOutput());
      outputNode->SetData(data);

      outputNode->SetName("PCA");
      node->SetVisibility(false);
      this->GetDataStorage()->Add(outputNode, node.GetPointer());
    }
  }
}
