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

#include <regex>

#include <QtConcurrent>
#include <QInputDialog>
#include <QColorDialog>

#include <boost/format.hpp>

#include <itksys/SystemTools.hxx>
#include <itkRescaleIntensityImageFilter.h>

#include <berryPlatform.h>
#include <mitkCoreServices.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>

#include <QmitkRenderWindow.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkImageCast.h>
#include <mitkLookupTableProperty.h>
#include <mitkLayoutAnnotationRenderer.h>
#include <mitkColorProperty.h>

#include <m2SubdivideImage2DFilter.h>
#include <m2SpectrumImageBase.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2UIUtils.h>

#include "m2Data.h"
#include "m2OpenSlideImageIOHelperDialog.h"


const std::string m2Data::VIEW_ID = "org.mitk.views.m2.Data";

void m2Data::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  m_Parent = parent;

  m_M2aiaPreferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");

  InitBaselineCorrectionStrategyComboBox();
  InitNormalizationStrategyComboBox();
  InitRangePoolingStrategyComboBox();
  InitSmoothingStrategyComboBox();
  InitIntensityTransformationStrategyComboBox();

  auto serviceRef = m2::UIUtils::Instance();
  connect(serviceRef, SIGNAL(UpdateImage(qreal, qreal)), this, SLOT(OnGenerateImageData(qreal, qreal)));

  connect(m_Controls.btnCreateImage,
          &QAbstractButton::clicked,
          this,
          [&] { OnGenerateImageData(m_Controls.spnBxMz->value(), FROM_GUI); });

 
  // step through
  // signals are triggered by key strokes (arrows) from spectrum view (Spectrum.ccp)
  auto UIUtilsObject = m2::UIUtils::Instance();
  connect(UIUtilsObject, SIGNAL(PreviousImage()), this, SLOT(OnCreatePrevImage()));
  connect(UIUtilsObject, SIGNAL(NextImage()), this, SLOT(OnCreateNextImage()));
  connect(UIUtilsObject, SIGNAL(IncreaseTolerance()), this, SLOT(OnIncreaseTolerance()));
  connect(UIUtilsObject, SIGNAL(DecreaseTolerance()), this, SLOT(OnDecreaseTolerance()));

  // Make sure, that data nodes added before this view
  // is initialized are handled correctly!!
  auto nodes = this->GetDataStorage()->GetAll();
  unsigned int count = 0;
  for (auto n : *nodes)
  {
    if (auto I = dynamic_cast<m2::ImzMLSpectrumImage *>(n->GetData()))
    {
      if (I->GetImageAccessInitialized())
      {
        n->SetVisibility(false);
      }
      else
      {
        count++;
        n->SetStringProperty("UpdateRequired", "uninitialized imzML");
      }
    }
  }

  if (count)
  {
    auto future = QtConcurrent::run(
      [parent, this]()
      {
        QThread::currentThread()->sleep(1);
      });

    auto watcher = std::make_shared<QFutureWatcher<void>>();
    watcher->setFuture(future);
    connect(watcher.get(),
            &QFutureWatcher<void>::finished,
            [watcher, this]() mutable
            {
              auto nodes = this->GetDataStorage()->GetAll();

              for (auto n : *nodes)
              {
                if (n->GetProperty("UpdateRequired"))
                {
                  auto I = dynamic_cast<m2::SpectrumImageBase *>(n->GetData());
                  this->ApplySettingsToImage(I);
                  this->GetDataStorage()->Remove(n);
                  this->GetDataStorage()->Add(n);
                }
              }
              mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
              this->RequestRenderWindowUpdate();
              watcher->disconnect();
              watcher.reset();
            });
  }

  


  m_Controls.multiLabelImageSelection->SetDataStorage(GetDataStorage());
  m_Controls.multiLabelImageSelection->SetAutoSelectNewNodes(false);
  m_Controls.multiLabelImageSelection->SetNodePredicate(
  
  mitk::NodePredicateAnd::New(mitk::NodePredicateDataType::New("LabelSetImage"), 
                              mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.multiLabelImageSelection->SetSelectionIsOptional(true);
  m_Controls.multiLabelImageSelection->SetEmptyInfo(QString("LabelSetImage selection"));
  m_Controls.multiLabelImageSelection->SetPopUpTitel(QString("LabelSetImage"));


}

void m2Data::InitNormalizationStrategyComboBox()
{
  auto preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");
  auto defaultValue =
    preferences->GetInt("NormalizationStrategy", static_cast<unsigned int>(m2::NormalizationStrategyType::None));
  auto cb = Controls()->CBNormalization;
  for (unsigned int i = 0; i < m2::NormalizationStrategyTypeNames.size(); ++i)
    cb->addItem(m2::NormalizationStrategyTypeNames[i].c_str(), {i});
  cb->setCurrentIndex(defaultValue);
}

m2::NormalizationStrategyType m2Data::GuiToNormalizationStrategyType()
{
  auto preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");
  auto value = this->Controls()->CBNormalization->currentData().toUInt();
  preferences->PutInt("NormalizationStrategy", value);
  return static_cast<m2::NormalizationStrategyType>(value);
}

void m2Data::InitIntensityTransformationStrategyComboBox()
{
  auto preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");
  auto defaultValue = preferences->GetInt("IntensityTransformationStrategy",
                                          static_cast<unsigned int>(m2::IntensityTransformationType::None));
  auto cb = Controls()->CBTransformation;
  for (unsigned int i = 0; i < m2::IntensityTransformationTypeNames.size(); ++i)
    cb->addItem(m2::IntensityTransformationTypeNames[i].c_str(), {i});
  cb->setCurrentIndex(defaultValue);
}

m2::IntensityTransformationType m2Data::GuiToIntensityTransformationStrategyType()
{
  auto preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");
  auto value = this->Controls()->CBTransformation->currentData().toUInt();
  preferences->PutInt("IntensityTransformationStrategy", value);
  return static_cast<m2::IntensityTransformationType>(value);
}

void m2Data::InitRangePoolingStrategyComboBox()
{
  auto preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");
  auto defaultValue =
    preferences->GetInt("RangePoolingStrategy", static_cast<unsigned int>(m2::RangePoolingStrategyType::Maximum));
  auto cb = Controls()->CBImagingStrategy;
  for (unsigned int i = 0; i < m2::RangePoolingStrategyTypeNames.size(); ++i)
    cb->addItem(m2::RangePoolingStrategyTypeNames[i].c_str(), {i}); // add i as data
  cb->setCurrentIndex(defaultValue);
}

m2::RangePoolingStrategyType m2Data::GuiToRangePoolingStrategyType()
{
  auto preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");
  auto value = this->Controls()->CBImagingStrategy->currentData().toUInt();
  preferences->PutInt("RangePoolingStrategy", value);
  return static_cast<m2::RangePoolingStrategyType>(value);
}

void m2Data::InitSmoothingStrategyComboBox()
{
  auto preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");
  auto defaultValue = preferences->GetInt("SmoothingStrategy", static_cast<unsigned int>(m2::SmoothingType::None));
  auto cb = Controls()->CBSmoothing;
  for (unsigned int i = 0; i < m2::SmoothingTypeNames.size(); ++i)
    cb->addItem(m2::SmoothingTypeNames[i].c_str(), {i});
  cb->setCurrentIndex(defaultValue);
}

m2::SmoothingType m2Data::GuiToSmoothingStrategyType()
{
  auto preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");
  auto value = this->Controls()->CBSmoothing->currentData().toUInt();
  preferences->PutInt("SmoothingStrategy", value);
  return static_cast<m2::SmoothingType>(value);
}

void m2Data::InitBaselineCorrectionStrategyComboBox()
{
  auto preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");
  auto defaultValue =
    preferences->GetInt("BaselineCorrectionStrategy", static_cast<unsigned int>(m2::BaselineCorrectionType::None));
  auto cb = Controls()->CBBaselineCorrection;
  for (unsigned int i = 0; i < m2::BaselineCorrectionTypeNames.size(); ++i)
    cb->addItem(m2::BaselineCorrectionTypeNames[i].c_str(), {i});
  cb->setCurrentIndex(defaultValue);
}

m2::BaselineCorrectionType m2Data::GuiToBaselineCorrectionStrategyType()
{
  auto preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");
  auto value = this->Controls()->CBBaselineCorrection->currentData().toUInt();
  preferences->PutInt("BaselineCorrectionStrategy", value);
  return static_cast<m2::BaselineCorrectionType>(value);
}

void m2Data::OnDecreaseTolerance()
{
  m_Controls.spnBxTol->setValue(m_Controls.spnBxTol->value() * 0.9);
  OnGenerateImageData(m_Controls.spnBxMz->value(), FROM_GUI);
}

void m2Data::OnIncreaseTolerance()
{
  m_Controls.spnBxTol->setValue(m_Controls.spnBxTol->value() * 1.1);
  OnGenerateImageData(m_Controls.spnBxMz->value(), FROM_GUI);
}

void m2Data::OnCreateNextImage()
{
  auto center = m_Controls.spnBxMz->value();
  auto offset = m_Controls.spnBxTol->value();
  if (m_Controls.rbtnTolPPM->isChecked())
  {
    offset = m2::PartPerMillionToFactor(offset) * center;
  }
  this->OnGenerateImageData(center + offset, FROM_GUI);
}

void m2Data::OnCreatePrevImage()
{
  auto center = m_Controls.spnBxMz->value();
  auto offset = m_Controls.spnBxTol->value();
  if (m_Controls.rbtnTolPPM->isChecked())
  {
    offset = m2::PartPerMillionToFactor(offset) * center;
  }
  this->OnGenerateImageData(center - offset, FROM_GUI);
}

void m2Data::ApplySettingsToNodes(m2::UIUtils::NodesVectorType::Pointer v)
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
    data->SetIntensityTransformationStrategy(GuiToIntensityTransformationStrategyType());

    if (data->GetSmoothingStrategy() == m2::SmoothingType::Gaussian)
    {
      auto d = int(m_Controls.spnBxSigma->value() * 4 + 0.5);
      m_Controls.spnBxSmoothing->setValue(d);
    }
    data->SetSmoothingHalfWindowSize(m_Controls.spnBxSmoothing->value());
    data->SetBaseLineCorrectionHalfWindowSize(m_Controls.spnBxBaseline->value());
    data->SetUseToleranceInPPM(m_Controls.rbtnTolPPM->isChecked());

    auto preferences = mitk::CoreServices::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");

    data->SetNumberOfBins(std::stoi(preferences->Get("bins", "10000")));

    // data->SetBinningTolerance(m_Controls.spnBxPeakBinning->value());
  }
}

void m2Data::OnGenerateImageData(qreal xRangeCenter, qreal xRangeTol)
{
  // get the selection
  auto nodesToProcess = m2::UIUtils::AllNodes(GetDataStorage());
  if (nodesToProcess->empty())
    return;

  if (xRangeTol < 0)
  {
    xRangeTol = Controls()->spnBxTol->value();
    bool isPpm = Controls()->rbtnTolPPM->isChecked();
    xRangeTol = isPpm ? m2::PartPerMillionToFactor(xRangeTol) * xRangeCenter : xRangeTol;
  }
  emit m2::UIUtils::Instance()->RangeChanged(xRangeCenter, xRangeTol);

  auto flag = std::make_shared<std::atomic<bool>>(0);
  QString labelText = str(boost::format("%.4f +/- %.2f") % xRangeCenter % xRangeTol).c_str();
  if (nodesToProcess->size() == 1)
  {
    auto node = nodesToProcess->front();

    if (auto image = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
    {
      std::string xLabel = image->GetSpectrumType().XAxisLabel;
      labelText = "[" + QString(xLabel.c_str()) + "]" + labelText;
    }
    labelText = QString(node->GetName().c_str()) + "\n" + labelText;
  }

  // this->m_Controls.labelIonImageInfo->setWordWrap(true);
  // this->m_Controls.labelIonImageInfo->setText(labelText);
  this->m_Controls.spnBxMz->setValue(xRangeCenter);

  this->UpdateTextAnnotations(labelText.toStdString());
  this->ApplySettingsToNodes(nodesToProcess);

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
      if(auto node = m_Controls.multiLabelImageSelection->GetSelectedNode()){
        if(auto image = dynamic_cast<mitk::LabelSetImage*>(node->GetData())){
          maskImage = image;
        }
      }

      
      // The smartpointer will stay alive until all captured copies are relesed. Additional
      // all connected signals must be disconnected to make sure that the future is not kept
      // alive after the 'finished-callback' is processed.
      auto future = std::make_shared<QFutureWatcher<mitk::Image::Pointer>>();

      //*************** Worker Finished Callback ******************//
      // capture holds a copy of the smartpointer, so it will stay alive. Make the lambda mutable to
      // allow the manipulation of captured varaibles that are copied by '='.
      const auto futureFinished = [future, dataNode, nodesToProcess, labelText, this]() mutable
      {
        auto image = future->result();
        UpdateLevelWindow(dataNode);
        UpdateSpectrumImageTable(dataNode);
        dataNode->SetProperty("x_range_center", image->GetProperty("x_range_center"));
        dataNode->SetProperty("x_range_tol", image->GetProperty("x_range_tol"));
        this->RequestRenderWindowUpdate();
        future->disconnect();
      };

      //*************** Worker Block******************//
      const auto futureWorker = [xRangeCenter, xRangeTol, data, maskImage, this]()
      {
        // m2::Timer t("Create image @[" + std::to_string(xRangeCenter) + " " + std::to_string(xRangeTol) + "]");
        if (m_InitializeNewNode)
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
    }
  }
}

void m2Data::UpdateSpectrumImageTable(const mitk::DataNode *node)
{
  if (dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    // auto widgets = m_Parent->findChildren<Qm2ImageColorWidget *>("Qm2ImageColorWidget");
    // MITK_INFO << widgets.count();
    //  if(!widgets.count() || widgets.front()->visibilityCheckBox()->isChecked()){
    //    MITK_INFO << widgets.back();
    //    auto c = m_Controls.verticalLayout->count();
    //    m_Controls.verticalLayout->insertWidget(c-1-widgets.count(), new Qm2ImageColorWidget(m_Parent));

    //  }
  }
  //   auto item = m_Controls.tableWidget->item(0, 1);
  //   std::string xLabel = image->GetPropertyValue<std::string>("x_label");
  //   double center = image->GetPropertyValue<double>("x_range_center");
  //   double tol = image->GetPropertyValue<double>("x_range_tol");
  //   QString labelText = str(boost::format(xLabel + " %.2f +/- %.2f Da") % center % tol).c_str();
  //   item->setText(labelText);

  // }

  // item = m_Controls.tableWidget->item(0, 0)->setData()
}



void m2Data::OnRenderSpectrumImages(double min, double max)
{
  Q_UNUSED(min);
  Q_UNUSED(max);
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

mitk::DataNode::Pointer m2Data::FindChildNodeRegex(mitk::DataNode::Pointer &parent, std::string regexString)
{
  auto deriv = this->GetDataStorage()->GetDerivations(parent.GetPointer(), nullptr, true);
  try
  {
    for (auto p : *deriv)
    {
      if (std::regex_match(p->GetName(), std::regex{regexString.c_str()}))
        return p;
    }
  }
  catch (std::exception &e)
  {
    MITK_WARN << e.what();
  }
  return nullptr;
}

void m2Data::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*part*/, const QList<mitk::DataNode::Pointer> &nodes)
{
  if (!nodes.empty())
  {
    auto node = nodes.front();
    if (auto image = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
    {
      
      QString labelText = str(boost::format("%.2f +/- %.2f Da") % image->GetCurrentX() % image->GetTolerance()).c_str();

        labelText += "\n";
        if (nodes.size() == 1)
          labelText += node->GetName().c_str();

        this->UpdateTextAnnotations(labelText.toStdString());
    
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
    const auto max = lw.GetRangeMax();
    lw.SetWindowBounds(0, max);
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
    emit m2::UIUtils::Instance()->SpectrumImageNodeAdded(node);
  }else if (dynamic_cast<m2::IntervalVector *>(node->GetData()))
  {
    // emit m2::UIUtils::Instance()->SpectrumImageNodeAdded(node);
  }
}

void m2Data::OpenSlideImageNodeAdded(const mitk::DataNode *node)
{
  if (auto openSlideIOHelper = dynamic_cast<m2::OpenSlideImageIOHelperObject *>(node->GetData()))
  {
    const auto name = node->GetName();
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
            node->SetName(name + "_tile_" + std::to_string(k));
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

void m2Data::ImzMLImageNodeAdded(const mitk::DataNode*)
{
 
}

void m2Data::FsmImageNodeAdded(const mitk::DataNode *)
{
  //
}

void m2Data::SpectrumImageNodeAdded(const mitk::DataNode *node)
{
  auto nodes = GetDataStorage()->GetAll();
  if (std::any_of(nodes->begin(),
                  nodes->end(),
                  [node](auto f) { return f != node && f->GetName().compare(node->GetName()) == 0; }))
  {
    bool ok;
    QString text = QInputDialog::getText(
    this->m_Parent, tr("Name conflict"), tr("Postfix:"), QLineEdit::Normal, QDir::home().dirName(), &ok);
    const_cast<mitk::DataNode *>(node)->SetName((node->GetName() + "_" + text.toStdString()).c_str());
  }

  auto lut = mitk::LookupTable::New();
  lut->SetType(mitk::LookupTable::LookupTableType::CIVIDS_TRANSPARENT);
  const_cast<mitk::DataNode *>(node)->SetProperty("LookupTable", mitk::LookupTableProperty::New(lut));
  const_cast<mitk::DataNode *>(node)->SetBoolProperty("binary", false);
  mitk::LevelWindow lw;
  const_cast<mitk::DataNode *>(node)->GetLevelWindow(lw);
  lw.SetToMaxWindowSize();
  const_cast<mitk::DataNode *>(node)->SetLevelWindow(lw);

  if (auto spectrumImage = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
  {
    //    // and add as child node
    //    if (spectrumImage->GetImageArtifacts().find("references") != std::end(spectrumImage->GetImageArtifacts()))
    //    {
    //      auto helperNode = mitk::DataNode::New();
    //      helperNode->SetName(node->GetName());
    //      helperNode->SetVisibility(false);
    //      helperNode->SetData(spectrumImage->GetImageArtifacts()["references"]);
    //      helperNode->SetFloatProperty("point 2D size",
    //                                   spectrumImage->GetGeometry()->GetSpacing()[0] * 3);              // x
    //                                   spacing
    //      helperNode->SetFloatProperty("pointsize", spectrumImage->GetGeometry()->GetSpacing()[0] * 3); // x
    //      spacing this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
    //    }

    // -------------- add Mask to datastorage --------------
    if (m_M2aiaPreferences->GetBool("showMaskImage", false) && spectrumImage->GetMaskImage())
    {
      auto helperNode = mitk::DataNode::New();
      helperNode->SetName(node->GetName() + "_mask");
      helperNode->SetVisibility(true);
      helperNode->SetData(spectrumImage->GetMaskImage());

      this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
    }

    // -------------- add Index to datastorage --------------
    if (m_M2aiaPreferences->GetBool("showIndexImage", false) && spectrumImage->GetIndexImage())
    {
      auto helperNode = mitk::DataNode::New();
      helperNode->SetName(node->GetName() + "_index");
      helperNode->SetVisibility(false);
      helperNode->SetData(spectrumImage->GetIndexImage());

      this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
    }

    // -------------- add Normalization to datastorage --------------

    if (m_M2aiaPreferences->GetBool("showNormalizationImage", false) && spectrumImage->GetNormalizationImage())
    {
      auto helperNode = mitk::DataNode::New();
      helperNode->SetName(node->GetName() + "_normalization");
      helperNode->SetVisibility(false);
      helperNode->SetData(spectrumImage->GetNormalizationImage());

      this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
    }
  }
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
