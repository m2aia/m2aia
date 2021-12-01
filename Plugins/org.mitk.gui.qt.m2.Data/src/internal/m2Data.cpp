/*===================================================================

Mass Spectrometry Imaging applications for interactive
analysis in MITK (M2aia)

Copyright (c) Jonas Cordes, Hochschule Mannheim.
Division of Medical Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include "m2Data.h"
#include "m2OpenSlideImageIOHelperDialog.h"

#include <boost/format.hpp>

#include <QtConcurrent>
#include <QShortcut>

#include <berryPlatform.h>

#include <itksys/SystemTools.hxx>

#include <itkRescaleIntensityImageFilter.h>

#include <QmitkRenderWindow.h>

#include <mitkLayoutAnnotationRenderer.h>
#include <mitkLookupTableProperty.h>
#include <mitkImageCast.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateProperty.h>

#include <m2SubdivideImage2DFilter.h>
#include <m2Timer.h>



const std::string m2Data::VIEW_ID = "org.mitk.views.m2.Data";

// 20201023: custom selection service did not work as expected
// void m2Data::SetSelectionProvider()
//{
//  this->GetSite()->SetSelectionProvider(m_SelectionProvider);
//}

void m2Data::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  m_Parent = parent;

  InitBaselineCorrectionStrategyComboBox();
  InitNormalizationStrategyComboBox();
  InitRangePoolingStrategyComboBox();
  InitSmoothingStrategyComboBox();

  auto serviceRef = m2::CommunicationService::Instance();
  connect(serviceRef, SIGNAL(UpdateImage(qreal, qreal)), this, SLOT(OnGenerateImageData(qreal, qreal)));
  connect(serviceRef,
          SIGNAL(RequestProcessingNodes(const QString &)),
          this,
          SLOT(OnProcessingNodesRequested(const QString &)));

  connect(m_Controls.btnGrabIonImage,
          &QAbstractButton::clicked,
          this,
          [&] { OnGenerateImageData(m_Controls.spnBxMz->value(), -1); });

  // step through
  QShortcut *shortcutLeft = new QShortcut(QKeySequence(Qt::Key_Left), parent);
  connect(shortcutLeft, SIGNAL(activated()), this, SLOT(OnCreatePrevImage()));

  QShortcut *shortcutRight = new QShortcut(QKeySequence(Qt::Key_Right), parent);
  connect(shortcutRight, SIGNAL(activated()), this, SLOT(OnCreateNextImage()));
}

void m2Data::InitNormalizationStrategyComboBox()
{
  auto cb = Controls()->CBNormalization;
  for (unsigned int i = 0; i < m2::NormalizationStrategyTypeNames.size(); ++i)  
    cb->addItem(m2::NormalizationStrategyTypeNames[i].c_str(), {i});

  auto defaultValue = static_cast<unsigned int>(m2::NormalizationStrategyType::None);
  cb->setCurrentIndex(defaultValue);
}

m2::NormalizationStrategyType m2Data::GuiToNormalizationStrategyType()
{
  return static_cast<m2::NormalizationStrategyType>(this->Controls()->CBNormalization->currentData().toUInt());
}

void m2Data::InitRangePoolingStrategyComboBox()
{
  auto cb = Controls()->CBImagingStrategy;
  for (unsigned int i = 0; i < m2::RangePoolingStrategyTypeNames.size(); ++i)  
    cb->addItem(m2::RangePoolingStrategyTypeNames[i].c_str(), {i});
    
  auto defaultValue = static_cast<unsigned int>(m2::RangePoolingStrategyType::Maximum);
  cb->setCurrentIndex(defaultValue);
  cb->removeItem(static_cast<unsigned int>(m2::RangePoolingStrategyType::None));
}

m2::RangePoolingStrategyType m2Data::GuiToRangePoolingStrategyType()
{
  return static_cast<m2::RangePoolingStrategyType>(this->Controls()->CBImagingStrategy->currentData().toUInt());
}

void m2Data::InitSmoothingStrategyComboBox()
{
  auto cb = Controls()->CBSmoothing;
  for (unsigned int i = 0; i < m2::SmoothingTypeNames.size(); ++i)  
    cb->addItem(m2::SmoothingTypeNames[i].c_str(), {i});
  
  auto defaultValue = static_cast<unsigned int>(m2::SmoothingType::None);
  cb->setCurrentIndex(defaultValue);
}

m2::SmoothingType m2Data::GuiToSmoothingStrategyType()
{
  return static_cast<m2::SmoothingType>(this->Controls()->CBSmoothing->currentData().toUInt());
}

void m2Data::InitBaselineCorrectionStrategyComboBox()
{
  auto cb = Controls()->CBBaselineCorrection;
  for (unsigned int i = 0; i < m2::BaselineCorrectionTypeNames.size(); ++i)  
    cb->addItem(m2::BaselineCorrectionTypeNames[i].c_str(), {i});
  
  auto defaultValue = static_cast<unsigned int>(m2::BaselineCorrectionType::None);
  cb->setCurrentIndex(defaultValue);
}

m2::BaselineCorrectionType m2Data::GuiToBaselineCorrectionStrategyType()
{
  return static_cast<m2::BaselineCorrectionType>(this->Controls()->CBBaselineCorrection->currentData().toUInt());
}

void m2Data::OnCreateNextImage()
{
  if (m_IonImageReference)
  {
    auto center = m_IonImageReference->mz;
    auto offset = m_Controls.spnBxTol->value();
    if (m_Controls.rbtnTolPPM->isChecked())
    {
      offset = m2::PartPerMillionToFactor(offset) * center;
    }
    this->OnGenerateImageData(center + offset, -1);
  }
}

void m2Data::OnCreatePrevImage()
{
  if (m_IonImageReference)
  {
    auto center = m_IonImageReference->mz;
    auto offset = m_Controls.spnBxTol->value();
    if (m_Controls.rbtnTolPPM->isChecked())
    {
      offset = m2::PartPerMillionToFactor(offset) * center;
    }
    this->OnGenerateImageData(center - offset, -1);
  }
}

void m2Data::OnProcessingNodesRequested(const QString &id)
{
  auto nodes = AllNodes();
  ApplySettingsToNodes(nodes);
  emit m2::CommunicationService::Instance()->BroadcastProcessingNodes(id, nodes);
}

void m2Data::OnGrabIonImageFinished(mitk::DataNode * /*dataNode*/, mitk::Image * /*image*/) {}

m2Data::NodesVectorType::Pointer m2Data::AllNodes()
{
  // Create a container vor all nodes that should be processed
  auto nodesToProcess = itk::VectorContainer<unsigned int, mitk::DataNode::Pointer>::New();
  // if (!m_Controls.checkBoxOnlyComboBoxSelection->isChecked())
  { // is checked: all nodes that fit the predicate are processed
    auto predicate =
      mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::SpectrumImageBase>::New(),
                                  mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object")));

    auto processableNodes = GetDataStorage()->GetSubset(predicate);

    for (auto node : processableNodes->CastToSTLConstContainer())
    {
      bool visibility;
      if (node->GetBoolProperty("visible", visibility))
        if (visibility)
          nodesToProcess->push_back(node);
    }
  }
  // else
  // { // or only the current index is used
  //   nodesToProcess->clear();
  //   auto nodes = this->m_Controls.MassSpecDataNodeSelectionWidget->GetSelectedNodes();
  //   if (!nodes.empty())
  //   {
  //     for (auto n : nodes)
  //     {
  //       nodesToProcess->push_back(n);
  //     }
  //   }
  // }
  return nodesToProcess;
}

void m2Data::ApplySettingsToNodes(m2Data::NodesVectorType::Pointer v)
{
  for (auto dataNode : *v)
  {
    if (auto data = dynamic_cast<m2::SpectrumImageBase *>(dataNode->GetData()))
      ApplySettingsToImage(data); // ImzML
  }
}

void m2Data::ApplySettingsToImage(m2::SpectrumImageBase *data)
{
  if (data)
  {
    data->SetNormalizationStrategy(GuiToNormalizationStrategyType());
    data->SetBaselineCorrectionStrategy(GuiToBaselineCorrectionStrategyType());
    data->SetSmoothingStrategy(GuiToSmoothingStrategyType());
    data->SetRangePoolingStrategy(GuiToRangePoolingStrategyType());

    if (data->GetSmoothingStrategy() == m2::SmoothingType::Gaussian)
    {
      auto d = int(m_Controls.spnBxSigma->value() * 4 + 0.5);
      m_Controls.spnBxSmoothing->setValue(d);
    }
    data->SetSmoothingHalfWindowSize(m_Controls.spnBxSmoothing->value());
    data->SetBaseLineCorrectionHalfWindowSize(m_Controls.spnBxBaseline->value());
    data->SetUseToleranceInPPM(m_Controls.rbtnTolPPM->isChecked());

    data->SetNumberOfBins(m_Controls.spnBxNumberOfBins->value());
    // data->SetBinningTolerance(m_Controls.spnBxPeakBinning->value());
  }
}

void m2Data::OnGenerateImageData(qreal xRangeCenter, qreal xRangeTol)
{
  const bool initializeNew = m2Data::Controls()->chkBxInitNew->isChecked();

  // get the selection
  auto nodesToProcess = this->AllNodes();
  if (nodesToProcess->empty())
    return;

  if (xRangeTol < 0)
  {
    xRangeTol = Controls()->spnBxTol->value();
    bool isPpm = Controls()->rbtnTolPPM->isChecked();
    xRangeTol = isPpm ? m2::PartPerMillionToFactor(xRangeTol) * xRangeCenter : xRangeTol;
  }
  emit m2::CommunicationService::Instance()->RangeChanged(xRangeCenter, xRangeTol);

  auto flag = std::make_shared<std::atomic<bool>>(0);
  QString labelText = str(boost::format("%.4f +/- %.2f") % xRangeCenter % xRangeTol).c_str();
  if (nodesToProcess->size() == 1)
  {
    auto node = nodesToProcess->front();

    if (auto image = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
    {
      std::string xLabel = image->GetPropertyValue<std::string>("x_label");
      labelText = "[" + QString(xLabel.c_str()) + "]" + labelText;
    }
    labelText = QString(node->GetName().c_str()) + "\n" + labelText;
  }

  // this->m_Controls.labelIonImageInfo->setWordWrap(true);
  // this->m_Controls.labelIonImageInfo->setText(labelText);
  this->m_Controls.spnBxMz->setValue(xRangeCenter);

  this->UpdateTextAnnotations(labelText.toStdString());
  this->ApplySettingsToNodes(nodesToProcess);

  // all images share a new created ion image reference object
  // this ensures unified coloring across multiple images
  // the object is set as the currentIonImageReference during the
  // OnIonImageGrab method call. An extra action is required to add this
  // current object to the permament ion image reference vector.
  m_IonImageReference = m2::IonImageReference::New(xRangeCenter, xRangeTol, "");

  for (mitk::DataNode::Pointer dataNode : *nodesToProcess)
  {
    if (m2::SpectrumImageBase::Pointer data = dynamic_cast<m2::SpectrumImageBase *>(dataNode->GetData()))
    {
      if (!data->IsInitialized())
        mitkThrow() << "Trying to grab an ion image but data access was not initialized properly!";

      auto xMin = data->GetPropertyValue<double>("x_min");
      auto xMax = data->GetPropertyValue<double>("x_max");

      if (xRangeCenter > xMax || xRangeCenter < xMin)
        continue;

      mitk::Image::Pointer maskImage;
      auto maskNode = FindChildNodeWithNameContaining(dataNode, "mask");

      if (maskNode && Controls()->chkBxUseMask->isChecked())
        maskImage = dynamic_cast<mitk::Image *>(maskNode->GetData());

      // if it is null, it's the first time an ion image is grabbed - disable visibility
      // so that the level window works fine for the ion image and not for the mask
      if (maskNode && maskNode->GetProperty("x_value") == nullptr)
        maskNode->SetVisibility(false);

      // The smartpointer will stay alive until all captured copies are relesed. Additional
      // all connected signals must be disconnected to make sure that the future is not kept
      // alive after the 'finished-callback' is processed.
      auto future = std::make_shared<QFutureWatcher<mitk::Image::Pointer>>();
      data->SetCurrentIonImageReference(m_IonImageReference);

      //*************** Worker Finished Callback ******************//
      // capture holds a copy of the smartpointer, so it will stay alive. Make the lambda mutable to
      // allow the manipulation of captured varaibles that are copied by '='.
      const auto futureFinished = [future, dataNode, nodesToProcess, labelText, this]() mutable
      {
        auto image = future->result();

        if (this->m_Controls.chkBxInitNew->isChecked())
        {
          using SourceImageType = itk::Image<m2::DisplayImagePixelType, 3>;
          auto Caster = [&image](auto *itkImage)
          {
            SourceImageType::Pointer srcImage;

            mitk::CastToItkImage(image, srcImage);
            using ImageType = typename std::remove_pointer<decltype(itkImage)>::type;
            using FilterType = itk::RescaleIntensityImageFilter<SourceImageType, ImageType>;
            auto filter = FilterType::New();
            filter->SetInput(srcImage);
            filter->SetOutputMinimum(0);
            filter->SetOutputMaximum(std::numeric_limits<typename ImageType::PixelType>::max());
            filter->Update();
            mitk::CastToMitkImage(filter->GetOutput(), image);
          };

          auto id = m_Controls.cmbBxType->currentIndex();
          if (id == 0)
          {
            // do nothing
          }
          else if (id == 1)
          {
            using TargetImageType = itk::Image<unsigned char, 3>;
            TargetImageType::Pointer itkIonImage;
            Caster(itkIonImage.GetPointer());
          }
          else if (id == 2)
          {
            using TargetImageType = itk::Image<unsigned short, 3>;
            TargetImageType::Pointer itkIonImage;
            Caster(itkIonImage.GetPointer());
          }
          else if (id == 3)
          {
            using TargetImageType = itk::Image<unsigned int, 3>;
            TargetImageType::Pointer itkIonImage;
            Caster(itkIonImage.GetPointer());
          }

          auto dataNode = mitk::DataNode::New();
          auto lut = mitk::LookupTable::New();
          lut->SetType(mitk::LookupTable::LookupTableType::GRAYSCALE_TRANSPARENT);
          dataNode->SetProperty("LookupTable", mitk::LookupTableProperty::New(lut));
          dataNode->SetData(image);
          this->GetDataStorage()->Add(dataNode);
          if (nodesToProcess->size() == 1)
            dataNode->SetName(labelText.toStdString());
          else
            dataNode->SetName(dataNode->GetName() + "\n" + labelText.toStdString());
        }
        else
        {
          UpdateLevelWindow(dataNode);
        }
        dataNode->SetProperty("x_range_center", image->GetProperty("x_range_center"));
        dataNode->SetProperty("x_range_tol", image->GetProperty("x_range_tol"));
        mitk::LevelWindow lw;
        dataNode->GetLevelWindow(lw);
        lw.SetToImageRange(image);
        dataNode->SetLevelWindow(lw);

        this->RequestRenderWindowUpdate();
        future->disconnect();
      };

      //*************** Worker Block******************//
      const auto futureWorker = [xRangeCenter, xRangeTol, data, maskImage, initializeNew, this]()
      {
        m2::Timer t("Create image @[" + std::to_string(xRangeCenter) + " " + std::to_string(xRangeTol) + "]");
        if (initializeNew)
        {
          auto geom = data->GetGeometry()->Clone();
          auto image = mitk::Image::New();
          image->Initialize(mitk::MakeScalarPixelType<m2::DisplayImagePixelType>(), *geom);
          data->GetImage(xRangeCenter, xRangeTol, maskImage, image);
          return image;
        }
        else
        {
          data->GetImage(xRangeCenter, xRangeTol, maskImage, data);
          mitk::Image::Pointer imagePtr = data.GetPointer();
          return imagePtr;
        }
      };

      //*************** Start Worker ******************//

      connect(future.get(), &QFutureWatcher<mitk::Image::Pointer>::finished, future.get(), futureFinished);
      future->setFuture(QtConcurrent::run(&m_pool, futureWorker));

      // if the future main process finished, we need to update the data nodes and their properties to
      // correctly visualize the new ion image. It requires an update on the level window propery and
      // a 'new init' if the data node is binary.

      // future->waitForFinished();
    }
  }
}

void m2Data::UpdateTextAnnotations(std::string text)
{
  static const std::array<std::string, 3> windownames = {"axial", "sagittal", "coronal"};
  if (m_TextAnnotations.size() != 3)
  {
    m_TextAnnotations.clear();
    for (int i = 0; i < 3; i++)
    {
      m_TextAnnotations.push_back(mitk::TextAnnotation2D::New());
      auto renderer = GetRenderWindowPart()->GetQmitkRenderWindow(windownames[i].c_str())->GetRenderer();
      mitk::LayoutAnnotationRenderer::AddAnnotation(m_TextAnnotations.back(), renderer);
      m_TextAnnotations.back()->SetFontSize(15);
      float color[] = {0.7, 0.7, 0.7};
      m_TextAnnotations.back()->SetFontSize(20);

      m_TextAnnotations.back()->SetColor(color);
    }
  }
  for (auto anno : m_TextAnnotations)
  {
    anno->SetText(text);
  }
}

mitk::DataNode::Pointer m2Data::FindChildNodeWithNameContaining(mitk::DataNode::Pointer &parent,
                                                                const std::string &subStr)
{
  auto deriv = this->GetDataStorage()->GetDerivations(parent.GetPointer(), nullptr, true);
  for (auto p : *deriv)
    if (p->GetName().find(subStr) != std::string::npos)
      return p;
  return nullptr;
}

void m2Data::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*part*/, const QList<mitk::DataNode::Pointer> &nodes)
{
  if (!nodes.empty())
  {
    auto node = nodes.front();
    if (auto image = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
    {
      if (auto current = image->GetCurrentIonImageReference())
      {
        QString labelText = str(boost::format("%.2f +/- %.2f Da") % current->mz % current->tol).c_str();

        labelText += "\n";
        if (nodes.size() == 1)
          labelText += node->GetName().c_str();

        this->UpdateTextAnnotations(labelText.toStdString());
      }
    }
  }
}

void m2Data::UpdateLevelWindow(const mitk::DataNode *node)
{
  if (auto msImageBase = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    mitk::LevelWindow lw;
    node->GetLevelWindow(lw);
    lw.SetAuto(msImageBase);
    const_cast<mitk::DataNode *>(node)->SetLevelWindow(lw);
  }
}

void m2Data::NodeAdded(const mitk::DataNode *node)
{
  if (dynamic_cast<m2::OpenSlideImageIOHelperObject *>(node->GetData()))
  {
    OpenSlideImageNodeAdded(node);
  }
  else if (auto data = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    //    if (dynamic_cast<m2::ImzMLSpectrumImage *>(data))
    //      ImzMLImageNodeAdded(node);
    //    else if (dynamic_cast<m2::FsmSpectrumImage *>(data))
    //      FsmImageNodeAdded(node);

    this->ApplySettingsToImage(data);
    data->InitializeImageAccess();
    this->RequestRenderWindowUpdate();

    SpectrumImageNodeAdded(node);
    emit m2::CommunicationService::Instance()->SpectrumImageNodeAdded(node);
  }
}

void m2Data::OpenSlideImageNodeAdded(const mitk::DataNode *node)
{
  if (auto openSlideIOHelper = dynamic_cast<m2::OpenSlideImageIOHelperObject *>(node->GetData()))
  {
    auto dialog = new m2OpenSlideImageIOHelperDialog(m_Parent);
    dialog->SetOpenSlideImageIOHelperObject(openSlideIOHelper);
    auto result = dialog->exec();
    if (result == QDialog::Accepted)
    {
      try
      {
        auto data = dialog->GetData();
        // auto preview = dialog->GetPreviwData();
        // mitk::DataNode::Pointer parent = nullptr;
        // if (preview)
        // {
        // parent = mitk::DataNode::New();
        // parent->SetName(
        //   itksys::SystemTools::GetFilenameWithoutExtension(openSlideIOHelper->GetOpenSlideIO()->GetFileName()));
        // parent->SetData(dialog->GetPreviwData());
        // this->GetDataStorage()->Add(parent);
        // }
        if (data.size() == 1)
        {
          auto filter = m2::SubdivideImage2DFilter::New();
          filter->SetInput(data.back());
          filter->SetTileHeight((unsigned int)(1) << 13);
          filter->SetTileWidth((unsigned int)(1) << 13);
          filter->Update();

          const auto nX = filter->GetNumberOfTilesInX();
          const auto nY = filter->GetNumberOfTilesInY();
          MITK_INFO << nX << " " << nY;

          unsigned int k = 0;
          for (auto I : filter->GetOutputs())
          {
            auto node = mitk::DataNode::New();
            node->SetData(I);
            node->SetName("tile_" + std::to_string(k));
            this->GetDataStorage()->Add(node);
            ++k;
          }
        }
        else
        {
          unsigned int i = 0;
          for (auto &I : data)
          {
            auto node = mitk::DataNode::New();
            node->SetData(I);
            node->SetName(
              itksys::SystemTools::GetFilenameWithoutExtension(openSlideIOHelper->GetOpenSlideIO()->GetFileName()) +
              "_" + std::to_string(i++));
            this->GetDataStorage()->Add(node);
          }
        }
      }
      catch (std::exception &e)
      {
        MITK_ERROR << "Rendering Error" << e.what();
      }
    }
    // remove IO helper object from DS
    GetDataStorage()->Remove(node);
    delete dialog;
  }
}

void m2Data::ImzMLImageNodeAdded(const mitk::DataNode *)
{
  //
}

void m2Data::FsmImageNodeAdded(const mitk::DataNode *)
{
  //
}

void m2Data::SpectrumImageNodeAdded(const mitk::DataNode *node)
{
  auto lut = mitk::LookupTable::New();
  lut->SetType(mitk::LookupTable::LookupTableType::CIVIDS_TRANSPARENT);
  const_cast<mitk::DataNode *>(node)->SetProperty("LookupTable", mitk::LookupTableProperty::New(lut));
  const_cast<mitk::DataNode *>(node)->SetBoolProperty("binary", false);
  mitk::LevelWindow lw;
  const_cast<mitk::DataNode *>(node)->GetLevelWindow(lw);
  lw.SetToMaxWindowSize();
  const_cast<mitk::DataNode *>(node)->SetLevelWindow(lw);

  //  if (auto spectrumImage = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  //  {
  //    // and add as child node
  //    if (spectrumImage->GetImageArtifacts().find("references") != std::end(spectrumImage->GetImageArtifacts()))
  //    {
  //      auto helperNode = mitk::DataNode::New();
  //      helperNode->SetName(node->GetName());
  //      helperNode->SetVisibility(false);
  //      helperNode->SetData(spectrumImage->GetImageArtifacts()["references"]);
  //      helperNode->SetFloatProperty("point 2D size",
  //                                   spectrumImage->GetGeometry()->GetSpacing()[0] * 3);              // x spacing
  //      helperNode->SetFloatProperty("pointsize", spectrumImage->GetGeometry()->GetSpacing()[0] * 3); // x spacing
  //      this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
  //    }

  // -------------- add Mask to datastorage --------------
  //   if(spectrumImage->GetMaskImage()){
  //     auto helperNode = mitk::DataNode::New();
  //     helperNode->SetName(node->GetName() + "_mask");
  //     helperNode->SetVisibility(true);
  //     helperNode->SetData(spectrumImage->GetMaskImage());

  //     this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
  //   }

  //   // -------------- add Index to datastorage --------------
  //   if(spectrumImage->GetIndexImage()){
  //     auto helperNode = mitk::DataNode::New();
  //     helperNode->SetName(node->GetName() + "_index");
  //     helperNode->SetVisibility(false);
  //     helperNode->SetData(spectrumImage->GetIndexImage());

  //     this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
  //   }

  //   // -------------- add Normalization to datastorage --------------
  //   if(spectrumImage->GetNormalizationImage()){
  //     auto helperNode = mitk::DataNode::New();
  //     helperNode->SetName(node->GetName() + "_normalization");
  //     helperNode->SetVisibility(false);
  //     helperNode->SetData(spectrumImage->GetNormalizationImage());

  //     this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
  //   }
  //  }
}

void m2Data::NodeRemoved(const mitk::DataNode *node)
{
  if (dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    auto derivations = this->GetDataStorage()->GetDerivations(node);
    for (auto &&d : *derivations)
    {
      this->GetDataStorage()->Remove(d);
    }
  }
}
