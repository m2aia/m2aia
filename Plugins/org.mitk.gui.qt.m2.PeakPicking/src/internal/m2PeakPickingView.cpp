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
#include <m2CoreCommon.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2IonImageReference.h>
#include <m2MultiSliceFilter.h>
#include <m2PcaImageFilter.h>
#include <m2TSNEImageFilter.h>
#include <signal/m2MedianAbsoluteDeviation.h>
#include <signal/m2PeakDetection.h>

// itk
#include <itkExtractImageFilter.h>
#include <itkFixedArray.h>
#include <itkIdentityTransform.h>
#include <itkImageAlgorithm.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkResampleImageFilter.h>
#include <itkShrinkImageFilter.h>
#include <itkVectorImageToImageAdaptor.h>

// mitk image
#include <mitkImage.h>
#include <mitkImageAccessByItk.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkProperties.h>

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
  m_Controls.cbOverviewSpectra->setCurrentIndex(1);

  connect(
    m_Controls.btnStartPeakPicking, &QCommandLinkButton::clicked, this, &m2PeakPickingView::OnRequestProcessingNodes);

  connect(m_Controls.btnPCA, &QCommandLinkButton::clicked, this, &m2PeakPickingView::OnStartPCA);
  connect(m_Controls.btnTSNE, &QCommandLinkButton::clicked, this, &m2PeakPickingView::OnStartTSNE);

  if (auto button = m_Controls.tableWidget->findChild<QAbstractButton *>(QString(), Qt::FindDirectChildrenOnly))
  {
    // this button is not a normal button, it doesn't paint text or icon
    // so it is not easy to show text on it, the simplest way is tooltip
    button->setToolTip("Select/deselect all");

    // disconnect the connected slots to the tableview (the "selectAll" slot)
    disconnect(button, Q_NULLPTR, m_Controls.tableWidget, Q_NULLPTR);
    // connect "clear" slot to it, here I use QTableWidget's clear, you can connect your own
    auto tableWidget = m_Controls.tableWidget;
    bool status = true;
    connect(button,
            &QAbstractButton::clicked,
            m_Controls.tableWidget,
            [tableWidget, status]() mutable
            {
              for (int i = 0; i < tableWidget->rowCount(); ++i)
              {
                tableWidget->item(i, 0)->setCheckState(status ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
              }
              status = !status;
            });
  }

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
      item->setCheckState(Qt::CheckState::Unchecked);
      m_Controls.tableWidget->setItem(row++, 0, item);
    }
    m_Controls.tableWidget->blockSignals(false);
  }
}

void m2PeakPickingView::OnStartPCA()
{
  if (!m_Controls.imageSource->count())
    return;
  const auto idx = m_Controls.imageSource->currentIndex();
  auto node = m_ReceivedNodes->at(idx);

  if (auto imageBase = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    auto filter = m2::PcaImageFilter::New();
    filter->SetMaskImage(imageBase->GetMaskImage());

    const auto &peakList = m_PeakLists[idx];

    std::vector<mitk::Image::Pointer> bufferedImages;

    size_t inputIdx = 0;
    for (size_t row = 0; row < peakList.size(); ++row)
    {
      if (m_Controls.tableWidget->item(row, 0)->checkState() != Qt::CheckState::Checked)
        continue;

      bufferedImages.push_back(mitk::Image::New());
      bufferedImages.back()->Initialize(imageBase);

      imageBase->GetImage(peakList[row].mass,
                             imageBase->ApplyTolerance(peakList[row].mass),
                             imageBase->GetMaskImage(),
                             bufferedImages.back());
      filter->SetInput(inputIdx, bufferedImages.back());
      ++inputIdx;
    }

    if (bufferedImages.size() == 0)
    {
      QMessageBox::warning(nullptr,
                           "Select images first!",
                           "Select at least three peaks!",
                           QMessageBox::StandardButton::NoButton,
                           QMessageBox::StandardButton::Ok);
      return;
    }

    filter->SetNumberOfComponents(m_Controls.pca_dims->value());
    filter->Update();

    auto outputNode = mitk::DataNode::New();
    mitk::Image::Pointer data = filter->GetOutput(0);
    outputNode->SetData(data);
    outputNode->SetName("PCA");
    this->GetDataStorage()->Add(outputNode, node.GetPointer());
    imageBase->InsertImageArtifact("PCA", data);

    // auto outputNode2 = mitk::DataNode::New();
    // mitk::Image::Pointer data2 = filter->GetOutput(1);
    // outputNode2->SetData(data2);
    // outputNode2->SetName("pcs");
    // this->GetDataStorage()->Add(outputNode2, node.GetPointer());
  }
}

void m2PeakPickingView::OnStartTSNE()
{
  if (!m_Controls.imageSource->count())
    return;
  const auto idx = m_Controls.imageSource->currentIndex();
  auto node = m_ReceivedNodes->at(idx);

  {
    auto p = node->GetProperty("name")->Clone();
    static_cast<mitk::StringProperty *>(p.GetPointer())->SetValue("PCA");
    auto derivations = this->GetDataStorage()->GetDerivations(node, mitk::NodePredicateProperty::New("name", p));
    if (derivations->size() == 0)
    {
      QMessageBox::warning(nullptr,
                           "PCA required!",
                           "The t-SNE uses the top five features of the PCA transformed images! Start a PCA first.",
                           QMessageBox::StandardButton::NoButton,
                           QMessageBox::StandardButton::Ok);
      return;
    }

    auto pcaImage = dynamic_cast<mitk::Image *>(derivations->front()->GetData());
    const auto pcaComponents = pcaImage->GetPixelType().GetNumberOfComponents();

    if (auto imageBase = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
    {
      auto filter = m2::TSNEImageFilter::New();
      filter->SetPerplexity(m_Controls.tsne_perplexity->value());
      filter->SetIterations(m_Controls.tnse_iters->value());
      filter->SetTheta(m_Controls.tsne_theta->value());

      using MaskImageType = itk::Image<mitk::LabelSetImage::PixelType, 3>;
      MaskImageType::Pointer maskImageItk;
      mitk::Image::Pointer maskImage;
      mitk::CastToItkImage(imageBase->GetMaskImage(), maskImageItk);
      auto caster = itk::ShrinkImageFilter<MaskImageType, MaskImageType>::New();
      caster->SetInput(maskImageItk);
      caster->SetShrinkFactor(0, m_Controls.tsne_shrink->value());
      caster->SetShrinkFactor(1, m_Controls.tsne_shrink->value());
      caster->SetShrinkFactor(2, 1);
      caster->Update();

      mitk::CastToMitkImage(caster->GetOutput(), maskImage);

      filter->SetMaskImage(maskImage);
      // const auto &peakList = imageBase->GetPeaks();
      size_t index = 0;

      itk::VectorImage<m2::DisplayImagePixelType, 3>::Pointer pcaImageItk;
      mitk::CastToItkImage(pcaImage, pcaImageItk);

      auto adaptor = VectorImageAdaptorType::New();

      std::vector<mitk::Image::Pointer> bufferedImages(pcaComponents);
      unsigned int i = 0;
      for (auto &I : bufferedImages)
      {
        adaptor->SetExtractComponentIndex(i++);
        adaptor->SetImage(pcaImageItk);
        auto caster = itk::ShrinkImageFilter<VectorImageAdaptorType, DisplayImageType>::New();
        caster->SetInput(adaptor);
        caster->SetShrinkFactor(0, m_Controls.tsne_shrink->value());
        caster->SetShrinkFactor(1, m_Controls.tsne_shrink->value());
        caster->SetShrinkFactor(2, 1);
        caster->Update();

        mitk::CastToMitkImage(caster->GetOutput(), I);

        filter->SetInput(index, I);
        ++index;
      }
      filter->Update();

      auto outputNode = mitk::DataNode::New();
      auto data =
        m2::MultiSliceFilter::ConvertMitkVectorImageToRGB(ResampleVectorImage(filter->GetOutput(), imageBase));
      outputNode->SetData(data);
      outputNode->SetName("tSNE");
      this->GetDataStorage()->Add(outputNode, node.GetPointer());
      imageBase->InsertImageArtifact("tSNE", data);
    }
  }
}

mitk::Image::Pointer m2PeakPickingView::ResampleVectorImage(mitk::Image::Pointer vectorImage,
                                                            mitk::Image::Pointer referenceImage)
{
  const unsigned int components = vectorImage->GetPixelType().GetNumberOfComponents();

  VectorImageType::Pointer vectorImageItk;
  mitk::CastToItkImage(vectorImage, vectorImageItk);

  DisplayImageType::Pointer referenceImageItk;
  mitk::CastToItkImage(referenceImage, referenceImageItk);

  auto resampledVectorImageItk = VectorImageType::New();
  resampledVectorImageItk->SetOrigin(referenceImageItk->GetOrigin());
  resampledVectorImageItk->SetDirection(referenceImageItk->GetDirection());
  resampledVectorImageItk->SetSpacing(referenceImageItk->GetSpacing());
  resampledVectorImageItk->SetRegions(referenceImageItk->GetLargestPossibleRegion());
  resampledVectorImageItk->SetNumberOfComponentsPerPixel(components);
  resampledVectorImageItk->Allocate();
  itk::VariableLengthVector<m2::DisplayImagePixelType> v(components);
  v.Fill(0);
  resampledVectorImageItk->FillBuffer(v);

  auto inAdaptor = VectorImageAdaptorType::New();
  auto outAdaptor = VectorImageAdaptorType::New();
  using LinearInterpolatorType = itk::LinearInterpolateImageFunction<VectorImageAdaptorType>;
  using TransformType = itk::IdentityTransform<m2::DisplayImagePixelType, 3>;

  for (unsigned int i = 0; i < components; ++i)
  {
    inAdaptor->SetExtractComponentIndex(i);
    inAdaptor->SetImage(vectorImageItk);
    inAdaptor->SetOrigin(vectorImageItk->GetOrigin());
    inAdaptor->SetDirection(vectorImageItk->GetDirection());
    inAdaptor->SetSpacing(vectorImageItk->GetSpacing());

    outAdaptor->SetExtractComponentIndex(i);
    outAdaptor->SetImage(resampledVectorImageItk);

    auto resampler = itk::ResampleImageFilter<VectorImageAdaptorType, DisplayImageType>::New();
    resampler->SetInput(inAdaptor);
    resampler->SetOutputParametersFromImage(referenceImageItk);
    resampler->SetInterpolator(LinearInterpolatorType::New());
    resampler->SetTransform(TransformType::New());
    resampler->Update();

    itk::ImageAlgorithm::Copy<DisplayImageType, VectorImageAdaptorType>(
      resampler->GetOutput(),
      outAdaptor,
      resampler->GetOutput()->GetLargestPossibleRegion(),
      outAdaptor->GetLargestPossibleRegion());
  }

  mitk::Image::Pointer outImage;
  mitk::CastToMitkImage(resampledVectorImageItk, outImage);

  return outImage;
}
