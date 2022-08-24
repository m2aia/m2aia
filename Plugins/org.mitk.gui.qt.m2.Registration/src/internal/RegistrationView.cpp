/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <queue>

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>
#include <berryPlatform.h>
#include <berryPlatformUI.h>

// Qmitk
#include "QmitkMultiNodeSelectionWidget.h"
#include "QmitkSingleNodeSelectionWidget.h"
#include "RegistrationView.h"
// Qt
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QProcess>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <qfiledialog.h>

// mitk
#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkImageAccessByItk.h>
#include <mitkImageCast.h>
#include <mitkImageReadAccessor.h>
#include <mitkImageWriteAccessor.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkPointSet.h>
#include <mitkSceneIO.h>
#include <mitkImageCast.h>

// itk
#include <itksys/SystemTools.hxx>
#include <itkInvertIntensityImageFilter.h>

// m2
#include <m2ElxDefaultParameterFiles.h>
#include <m2ElxRegistrationHelper.h>
#include <m2ElxUtil.h>
#include <ui_ParameterFileEditorDialog.h>

const std::string RegistrationView::VIEW_ID = "org.mitk.views.m2.Registration";

void RegistrationView::SetFocus() {}

void RegistrationView::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  m_Parent = parent;

  m_FixedEntity = std::make_shared<RegistrationEntity>();

  m_ParameterFileEditor = new QDialog(parent);
  m_ParameterFileEditorControls.setupUi(m_ParameterFileEditor);

  connect(m_Controls.btnStartRecon, &QAbstractButton::clicked, this, &RegistrationView::PostProcessReconstruction);

  // Initialize defaults
  {
    m_DefaultParameterFiles[0] = {m2::Elx::Rigid(), m2::Elx::Deformable()};

    auto rigid = m2::Elx::Rigid();
    m2::ElxUtil::ReplaceParameter(rigid, "FixedImageDimension", "3");
    m2::ElxUtil::ReplaceParameter(rigid, "MovingImageDimension", "3");
    auto deformable = m2::Elx::Deformable();
    m2::ElxUtil::ReplaceParameter(deformable, "FixedImageDimension", "3");
    m2::ElxUtil::ReplaceParameter(deformable, "MovingImageDimension", "3");
    m2::ElxUtil::RemoveParameter(deformable, "FinalGridSpacingInPhysicalUnits");
    m2::ElxUtil::RemoveParameter(deformable, "GridSpacingSchedule");
    m_DefaultParameterFiles[1] = {rigid, deformable};
  }

  m_ParameterFiles = m_DefaultParameterFiles;

  connect(
    m_Controls.registrationStrategy,
    qOverload<int>(&QComboBox::currentIndexChanged),
    this,
    [this](auto currentIndex)
    {
      m_ParameterFileEditorControls.rigidText->setText(m_ParameterFiles[currentIndex][0].c_str());
      m_ParameterFileEditorControls.deformableText->setText(m_ParameterFiles[currentIndex][1].c_str());

      switch (currentIndex)
      {
        case 0:
          m_Controls.label->setText("SliceImageData [NxM] or [NxMx1], MuliModal, MultiMetric, Rigid + Deformable");
          break;
        case 1:
          m_Controls.label->setText("VolumeImageData [NxMxD], MultiModal, MultiMetric, Rigid+Deformable");
          break;
      }
    });

  m_Controls.registrationStrategy->setCurrentIndex(1);

  connect(m_ParameterFileEditorControls.buttonBox->button(QDialogButtonBox::StandardButton::RestoreDefaults),
          &QAbstractButton::clicked,
          this,
          [this]()
          {
            auto currentIndex = this->m_Controls.registrationStrategy->currentIndex();
            m_ParameterFileEditorControls.rigidText->setText(m_DefaultParameterFiles[currentIndex][0].c_str());
            m_ParameterFileEditorControls.deformableText->setText(m_DefaultParameterFiles[currentIndex][1].c_str());
          });

  connect(m_ParameterFileEditorControls.buttonBox->button(QDialogButtonBox::StandardButton::Close),
          &QAbstractButton::clicked,
          this,
          [this]()
          {
            auto currentIndex = this->m_Controls.registrationStrategy->currentIndex();
            m_ParameterFiles[currentIndex] = {
              m_ParameterFileEditorControls.rigidText->toPlainText().toStdString(),
              m_ParameterFileEditorControls.deformableText->toPlainText().toStdString()};
          });

  connect(m_Controls.btnStartRegistration, &QPushButton::clicked, this, &RegistrationView::StartRegistration);

  m_FixedEntity->m_ImageSelection = m_Controls.fixedImageSelection;
  m_FixedEntity->m_ImageSelection->SetDataStorage(GetDataStorage());
  m_FixedEntity->m_ImageSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::Image>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_FixedEntity->m_ImageSelection->SetSelectionIsOptional(true);
  m_FixedEntity->m_ImageSelection->SetEmptyInfo(QString("Select fixed image"));
  m_FixedEntity->m_ImageSelection->SetPopUpTitel(QString("Select fixed image node"));

  m_FixedEntity->m_ImageMaskSelection = m_Controls.fixedImageMaskSelection;
  m_FixedEntity->m_ImageMaskSelection->SetDataStorage(GetDataStorage());
  m_FixedEntity->m_ImageMaskSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::Image>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_FixedEntity->m_ImageMaskSelection->SetSelectionIsOptional(true);
  m_FixedEntity->m_ImageMaskSelection->SetEmptyInfo(QString("Select fixed image mask"));
  m_FixedEntity->m_ImageMaskSelection->SetPopUpTitel(QString("Select fixed image mask node"));

  m_FixedEntity->m_PointSetSelection = m_Controls.fixedPointSelection;
  m_FixedEntity->m_PointSetSelection->SetDataStorage(GetDataStorage());
  m_FixedEntity->m_PointSetSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::PointSet>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_FixedEntity->m_PointSetSelection->SetSelectionIsOptional(true);
  m_FixedEntity->m_PointSetSelection->SetEmptyInfo(QString("Select fixed point set"));
  m_FixedEntity->m_PointSetSelection->SetPopUpTitel(QString("Select fixed point set node"));

  m_FixedEntity->m_Index = m_Controls.fixedIndex;

  // m_Controls.movingPointSetData->insertWidget(1, m_MovingPointSetSingleNodeSelection);

  m_Controls.addFixedPointSet->setToolTip("Add point set for the fixed image reference points");
  m_Controls.addFixedPointSet->setToolTip("Add point set for the moving image reference points");

  connect(m_Controls.btnAddModality, &QPushButton::clicked, this, [this]() { AddNewModalityTab(); });

  connect(m_Controls.btnOpenPontSetInteractionView,
          &QPushButton::clicked,
          this,
          []()
          {
            try
            {
              if (auto platform = berry::PlatformUI::GetWorkbench())
                if (auto workbench = platform->GetActiveWorkbenchWindow())
                  if (auto page = workbench->GetActivePage())
                    if (page.IsNotNull())
                      page->ShowView("org.mitk.views.pointsetinteraction", "", 1);
            }
            catch (berry::PartInitException &e)
            {
              BERRY_ERROR << "Error: " << e.what() << std::endl;
            }
          });

  connect(m_Controls.addFixedPointSet,
          &QPushButton::clicked,
          this,
          [&]()
          {
            auto node = mitk::DataNode::New();
            node->SetData(mitk::PointSet::New());
            node->SetName("FixedPointSet");
            auto fixedNode = m_FixedEntity->m_ImageSelection->GetSelectedNode();
            if (fixedNode)
              this->GetDataStorage()->Add(node, fixedNode);
            else
              this->GetDataStorage()->Add(node);
            node->SetFloatProperty("point 2D size", 0.5);
            node->SetProperty("color", mitk::ColorProperty::New(1.0f, 0.0f, 0.0f));
            this->m_FixedEntity->m_PointSetSelection->SetCurrentSelection({node});
          });

  connect(m_Controls.btnEditParameterFiles,
          &QPushButton::clicked,
          this,
          [this]()
          {
            m_ParameterFileEditor->exec();
            auto currentIndex = this->m_Controls.registrationStrategy->currentIndex();
            m_ParameterFiles[currentIndex] = {
              m_ParameterFileEditorControls.rigidText->toPlainText().toStdString(),
              m_ParameterFileEditorControls.deformableText->toPlainText().toStdString()};
          });
}

void RegistrationView::AddNewModalityTab()
{
  auto widget = new QWidget();
  Ui::RegistrationEntityWidgetControls controls;
  controls.setupUi(widget);
  const auto modalityId = m_ModalityId;
  const auto tabId = m_Controls.tabWidget->addTab(widget, QString(m_ModalityId));
  auto page = m_Controls.tabWidget->widget(tabId);
  page->setObjectName(QString(m_ModalityId));
  m_Controls.tabWidget->setCurrentIndex(tabId);
  m_MovingEntities[modalityId] = std::make_shared<RegistrationEntity>();

  connect(controls.btnApplyTransforms,
          &QAbstractButton::clicked,
          [=]() { Registration(m_FixedEntity.get(), m_MovingEntities[modalityId].get()); });

  auto filter = tr("Elastix Parameterfile (*.txt)");
  connect(controls.btnLoadPreset,
          &QAbstractButton::clicked,
          [=]()
          {
            auto paths = QFileDialog::getOpenFileNames(m_Parent, "Load elastix transform parameter files.", "", filter);
            for (auto p : paths)
            {
              std::string s;
              std::ifstream(paths[0].toStdString()) >> s;
              m_MovingEntities[modalityId]->m_Transformations.push_back(s);
            }
          });
  connect(controls.btnSaveTransforms,
          &QAbstractButton::clicked,
          [=]()
          {
            QString path;
            unsigned int i = 0;
            for (auto t : m_MovingEntities[modalityId]->m_Transformations)
            {
              auto name = m_MovingEntities[modalityId]->m_ImageSelection->GetSelectedNode()->GetName() + "_transform" +
                          std::to_string(i) + ".txt";
              auto spath = path.split('/');
              spath.pop_back();
              spath.push_back(name.c_str());
              path = spath.join('/');
              path = QFileDialog::getSaveFileName(
                m_Parent, tr("Save transformation ") + std::to_string(i++).c_str() + " file", path, filter);
              std::ofstream(path.toStdString()) << t;
            }
          });

  QWidget::setTabOrder(m_Controls.btnAddModality, controls.lineEditName);

  controls.lineEditName->setPlaceholderText(QString(modalityId) + ": new name for this modality.");
  connect(controls.lineEditName,
          &QLineEdit::textChanged,
          this,
          [tabId, this](const QString text) { m_Controls.tabWidget->setTabText(tabId, text); });

  auto imageSelection = new QmitkSingleNodeSelectionWidget();
  imageSelection->SetDataStorage(GetDataStorage());
  imageSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::Image>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  imageSelection->SetSelectionIsOptional(true);
  imageSelection->SetEmptyInfo(QString("Select image"));
  imageSelection->SetPopUpTitel(QString("Select image node"));
  controls.widgetList->insertWidget(1, imageSelection);
  m_MovingEntities[modalityId]->m_ImageSelection = imageSelection;

  auto imageMaskSelection = new QmitkSingleNodeSelectionWidget();
  imageMaskSelection->SetDataStorage(GetDataStorage());
  imageMaskSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::Image>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  imageMaskSelection->SetSelectionIsOptional(true);
  imageMaskSelection->SetEmptyInfo(QString("Select image mask"));
  imageMaskSelection->SetPopUpTitel(QString("Select image mask node"));
  controls.widgetList->insertWidget(2, imageMaskSelection);
  m_MovingEntities[modalityId]->m_ImageMaskSelection = imageMaskSelection;

  auto pointSetSelection = new QmitkSingleNodeSelectionWidget();
  pointSetSelection->SetDataStorage(GetDataStorage());
  pointSetSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::PointSet>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  pointSetSelection->SetSelectionIsOptional(true);
  pointSetSelection->SetEmptyInfo(QString("Select point set"));
  pointSetSelection->SetPopUpTitel(QString("Select point set node"));
  controls.referencePointSetData->addWidget(pointSetSelection);
  m_MovingEntities[modalityId]->m_PointSetSelection = pointSetSelection;

  auto attSelection = new QmitkSingleNodeSelectionWidget();
  attSelection->SetDataStorage(GetDataStorage());
  attSelection->SetNodePredicate(mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object")));
  attSelection->SetSelectionIsOptional(true);
  attSelection->SetEmptyInfo(QString("Select attached mask"));
  attSelection->SetPopUpTitel(QString("Select attached mask"));
  controls.attachmentLayout->addWidget(attSelection);
  m_MovingEntities[modalityId]->m_Attachments.push_back(attSelection);
  m_MovingEntities[modalityId]->m_Index = controls.index;
  m_MovingEntities[modalityId]->m_Index->setValue(m_MovingEntities.size());

  connect(controls.addMovingPointSet,
          &QPushButton::clicked,
          this,
          [imageSelection, pointSetSelection, this]()
          {
            auto node = mitk::DataNode::New();
            node->SetData(mitk::PointSet::New());
            node->SetName("MovingPointSet");

            auto movingNode = imageSelection->GetSelectedNode();
            if (movingNode)
            {
              this->GetDataStorage()->Add(node, movingNode);
              node->SetFloatProperty("point 2D size", 0.5);
              node->SetProperty("color", mitk::ColorProperty::New(0.0f, 1.0f, 1.0f));
              pointSetSelection->SetCurrentSelection({node});
            }
          });

  connect(controls.btnRemove,
          &QPushButton::clicked,
          this,
          [this, modalityId]()
          {
            auto widget = this->m_Controls.tabWidget->findChild<QWidget *>(QString(modalityId));
            auto id = m_Controls.tabWidget->indexOf(widget);
            this->m_Controls.tabWidget->removeTab(id);
          });

  ++m_ModalityId;
}

void RegistrationView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*part*/,
                                          const QList<mitk::DataNode::Pointer> &nodes)
{
  if (m_Controls.chkBxReinitOnSelectionChanged->isChecked())
  {
    if (!nodes.empty())
    {
      auto set = mitk::DataStorage::SetOfObjects::New();
      set->push_back(nodes.back());
      auto data = nodes.back();
      if (auto image = dynamic_cast<mitk::Image *>(data->GetData()))
      {
        mitk::RenderingManager::GetInstance()->InitializeViews(
          image->GetTimeGeometry(), mitk::RenderingManager::REQUEST_UPDATE_ALL, true);
        // auto boundingGeometry = GetDataStorage()->ComputeBoundingGeometry3D(set, "visible", nullptr);

        // mitk::RenderingManager::GetInstance()->InitializeViews(boundingGeometry);
      }
    }
  }
}

QString RegistrationView::GetElastixPathFromPreferences() const
{
  berry::IPreferences::Pointer preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");

  return preferences.IsNotNull() ? preferences->Get("elastix", "") : "";
}

void RegistrationView::PostProcessReconstruction()
{
  // create volume node

  {
    MITK_INFO << "***** Initialize new volume *****";
    auto image = dynamic_cast<mitk::Image *>(m_FixedEntity->m_ImageSelection->GetSelectedNode()->GetData());
    unsigned int dims[3] = {0, 0, 0};
    auto newVolume = mitk::Image::New();

    dims[0] = image->GetDimensions()[0];
    dims[1] = image->GetDimensions()[1];
    dims[2] = m_MovingEntities.size() + 1; // +1 for the fixed entity

    newVolume->Initialize(image->GetPixelType(), 3, dims);
    auto spacing = image->GetGeometry()->GetSpacing();
    spacing[2] = m2::MicroMeterToMilliMeter(m_Controls.spinBoxZSpacing->value());
    newVolume->SetSpacing(spacing);

    MITK_INFO << newVolume;
    
    MITK_INFO << "***** Init ordered container and set fixe image at position *****";
    std::vector<mitk::Image::Pointer> orderedData(dims[2]);
    MITK_INFO << "***** init ordered data vector *****";
    auto fixIndex = m_Controls.fixedIndex->value();
    orderedData[fixIndex] = image;
    MITK_INFO << "***** access index and assign fixed image *****";
    

    for (auto kv : m_MovingEntities)
    {
      auto idx = kv.second->m_Index->value();
      auto movingImage = dynamic_cast<mitk::Image *>(kv.second->m_ImageSelection->GetSelectedNode()->GetData());
      
      if(kv.second->m_Transformations.size()){
        m2::ElxRegistrationHelper helper;
        helper.SetTransformations(kv.second->m_Transformations);
        orderedData[idx] = helper.WarpImage(movingImage);
      }else{
        orderedData[idx] = movingImage;
      }
    }
    
    MITK_INFO << "***** Copy data to volume *****";
    AccessByItk(newVolume,
                (
                  [&](auto itkImage)
                  {
                                        
                    for (unsigned int i = 0; i < orderedData.size(); ++i)
                    {
                      AccessByItk(orderedData[i],
                                  (
                                    [&](auto itkInput)
                                    {
                                      auto td = itkImage->GetBufferPointer();
                                      auto sd = itkInput->GetBufferPointer(); // sourcedata
                                      std::copy(sd, sd + (dims[0] * dims[1]), td + (i * (dims[0] * dims[1])));
                                    }));
                    }
                  }));

    {
      MITK_INFO << "***** Add volume to data storage *****";

      auto r = mitk::DataNode::New();
      r->SetData(newVolume);
      r->SetName("Reconstruction");
      GetDataStorage()->Add(r);
    }
  }
}



void RegistrationView::Registration(RegistrationEntity *fixed, RegistrationEntity *moving)
{
  const auto invert = [](mitk::Image::Pointer image){
    mitk::Image::Pointer rImage;
    AccessByItk(image, ([&](auto imageItk) mutable{
      using ItkImageType = typename std::remove_pointer<decltype(imageItk)>::type;
      auto filter = itk::InvertIntensityImageFilter<ItkImageType>::New();
      filter->SetInput(imageItk);
      filter->Update();
      typename ItkImageType::Pointer fout = filter->GetOutput();
      mitk::CastToMitkImage(fout, rImage);
    }));
    return rImage;
  };

  // check if
  auto elastix = m2::ElxUtil::Executable("elastix");
  if (elastix.empty())
  {
    elastix = GetElastixPathFromPreferences().toStdString();
    if (elastix.empty())
    {
      QMessageBox::information(nullptr, "Error", "No elastix executable specified in the MITK properties!");
      return;
    }
  }

  // Images
  mitk::Image::Pointer fixedImage;
  mitk::Image::Pointer movingImage;
  auto fixedImageNode = fixed->m_ImageSelection->GetSelectedNode();
  if (fixedImageNode){
    fixedImage = dynamic_cast<mitk::Image *>(fixedImageNode->GetData());
    
    if(m_Controls.chkBxInvertIntensities->isChecked())
      fixedImage = invert(fixedImage);
    
    if(!fixed->m_Transformations.empty()){
      MITK_INFO << "***** Warp Fixed Image *****";
      m2::ElxRegistrationHelper warpingHelper;
      warpingHelper.SetTransformations(fixed->m_Transformations);
      fixedImage = warpingHelper.WarpImage(fixedImage);
    }
  }


  auto movingImageNode = moving->m_ImageSelection->GetSelectedNode();
  if (movingImageNode){
    movingImage = dynamic_cast<mitk::Image *>(movingImageNode->GetData());
    if(m_Controls.chkBxInvertIntensities->isChecked())
      movingImage = invert(movingImage);
  }

  // Fixed image mask
  mitk::Image::Pointer fixedImageMask;
  auto fixedImageMaskNode = fixed->m_ImageMaskSelection->GetSelectedNode();
  if (fixedImageMaskNode){
    fixedImageMask = dynamic_cast<mitk::Image *>(fixedImageMaskNode->GetData());
    if(!fixed->m_Transformations.empty()){
      m2::ElxRegistrationHelper warpingHelper;
      warpingHelper.SetTransformations(fixed->m_Transformations);
      fixedImageMask = warpingHelper.WarpImage(fixedImageMask, "short", 1);
    }
  }

  // PointSets
  mitk::PointSet::Pointer fixedPointSet;
  mitk::PointSet::Pointer movingPointSet;

  if (auto node = fixed->m_PointSetSelection->GetSelectedNode())
    fixedPointSet = dynamic_cast<mitk::PointSet *>(node->GetData());
  if (auto movingPointSetNode = moving->m_PointSetSelection->GetSelectedNode())
    movingPointSet = dynamic_cast<mitk::PointSet *>(movingPointSetNode->GetData());

  try
  {
    auto currentIndex = m_Controls.registrationStrategy->currentIndex();
    std::list<std::string> queue;

    std::function<void(std::string)> statusCallback = [=](std::string v) mutable
    {
      if (queue.size() > 15)
        queue.pop_front();
      queue.push_back(v);
      QString s;
      for (auto line : queue)
        s = s + line.c_str() + "\n";

      m_Controls.labelStatus->setText(s);
    };

    {
      m2::ElxRegistrationHelper helper;
      std::vector<std::string> parameterFiles;

      // copy valid and discard empty parameterfiles
      for (auto &p : m_ParameterFiles[currentIndex])
        parameterFiles.push_back(p);

      if (!m_Controls.chkBxUseDeformableRegistration->isChecked() || parameterFiles.back().empty())
      {
        parameterFiles.pop_back();
      }

      MITK_INFO << "***** Start Registration *****";
      // setup and run
      helper.SetAdditionalBinarySearchPath(itksys::SystemTools::GetParentDirectory(elastix));
      helper.SetImageData(fixedImage, movingImage);
      helper.SetFixedImageMaskData(fixedImageMask);
      helper.SetPointData(fixedPointSet, movingPointSet);
      helper.SetRegistrationParameters(parameterFiles);
      helper.SetRemoveWorkingDirectory(true);
      helper.UseMovingImageSpacing(m_Controls.keepSpacings->isChecked());
      helper.SetStatusCallback(statusCallback);
      helper.GetRegistration();
      // warp original data
      movingImage = dynamic_cast<mitk::Image *>(movingImageNode->GetData());
      auto warpedImage = helper.WarpImage(movingImage);

      auto timeString = itksys::SystemTools::GetCurrentDateTime("%H%M");
      auto node = mitk::DataNode::New();
      node->SetData(warpedImage);
      node->SetName(movingImageNode->GetName() + "_warped" + timeString);
      moving->m_ResultNode = node;
      moving->m_Transformations = helper.GetTransformation();

      MITK_INFO << "Attachments: " << moving->m_Attachments.size();
      for (const QmitkSingleNodeSelectionWidget *attSel : moving->m_Attachments)
      {
        if (auto node = attSel->GetSelectedNode())
        {
          if (auto labelSetImage = dynamic_cast<mitk::LabelSetImage *>(node->GetData()))
          {
            auto wapred = helper.WarpImage(labelSetImage, "short", 1);
            auto resNode = mitk::DataNode::New();
            resNode->SetData(wapred);
            resNode->SetName(node->GetName() + "_warped_mask" + timeString);
            moving->m_ResultAttachments.push_back(resNode);
          }
          else if (auto image = dynamic_cast<mitk::Image *>(node->GetData()))
          {
            auto warped = helper.WarpImage(image, "float", 1);
            auto resNode = mitk::DataNode::New();
            resNode->SetData(warped);
            resNode->SetName(node->GetName() + "_warped_att_" + timeString);
            moving->m_ResultAttachments.push_back(resNode);
          }
        }
      }
    }

    mitk::DataNode *parentNode = movingImageNode;
    if (m_Controls.chkBxAttachToFixedImage->isChecked())
    {
      parentNode = fixedImageNode;
    }

    MITK_INFO << "Add moving image node";
    auto node = moving->m_ResultNode;
    this->GetDataStorage()->Add(node, parentNode);

    MITK_INFO << "Add attachments";
    for (auto node : moving->m_ResultAttachments)
    {
      this->GetDataStorage()->Add(node, parentNode);
      MITK_INFO << node->GetName();
    }

    m_Controls.btnStartRegistration->setEnabled(true);
    statusCallback("Completed");
  }
  catch (std::exception &e)
  {
    MITK_ERROR << e.what();
  }
}

void RegistrationView::StartRegistration()
{
  if (m_Controls.rbAllToOne->isChecked())
  {
    MITK_INFO << "m_Controls.rbAllToOne->isChecked()";
    for (auto &kv : m_MovingEntities)
      Registration(m_FixedEntity.get(), kv.second.get());
  }

  if (m_Controls.rbSubsequent->isChecked())
  {

    MITK_INFO << "m_Controls.rbSubsequent->isChecked()";
    const auto fixIndex = m_Controls.fixedIndex->value();
    std::list<std::shared_ptr<RegistrationEntity>> upperEntities, lowerEntities;

    upperEntities.push_back(m_FixedEntity);
    lowerEntities.push_back(m_FixedEntity);

    for (auto &kv : m_MovingEntities)
    {
      if (kv.second->m_Index->value() < fixIndex)
        upperEntities.push_back(kv.second);

      if (kv.second->m_Index->value() > fixIndex)
        lowerEntities.push_back(kv.second);
    }

    upperEntities.sort([](auto a, auto b) { return a->m_Index->value() < b->m_Index->value(); });   
    lowerEntities.sort([](auto a, auto b) { return a->m_Index->value() < b->m_Index->value(); });

    {
      auto a = upperEntities.rbegin();
      auto b = std::next(upperEntities.rbegin());
      MITK_INFO << "start upper stack registration!"; 
      for (; b != upperEntities.rend(); ++a, ++b)
      {
        MITK_INFO << (*a)->m_Index->value() << "<-" << (*b)->m_Index->value();
        Registration(a->get(),b->get());
      }
    }
    {
      auto a = lowerEntities.begin();
      auto b = std::next(lowerEntities.begin());
      MITK_INFO << "start lower stack registration!";
      for (; b != lowerEntities.end(); ++a, ++b)
      {
        MITK_INFO << (*a)->m_Index->value() << "<-" << (*b)->m_Index->value();
        Registration(a->get(),b->get());
      }
    }
  }

  // auto resultPath = SystemTools::ConvertToOutputPath(SystemTools::JoinPath({path, "/", "result.1.nrrd"}));
  // MITK_INFO << "Result image path: " << resultPath;
  // if (SystemTools::FileExists(resultPath))
  // {
  //   auto vData = mitk::IOUtil::Load(resultPath);
  //   mitk::Image::Pointer image = dynamic_cast<mitk::Image *>(vData.back().GetPointer());
  //   auto node = mitk::DataNode::New();
  //   node->SetName(movingNode->GetName() + "_result");
  //   node->SetData(image);
  //   this->GetDataStorage()->Add(node, movingNode);
  // }

  // filesystem::remove(path);
}
