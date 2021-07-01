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
#include <berryPlatform.h>
#include <berryPlatformUI.h>

// Qmitk
#include "OpticalImageRegistration.h"
#include "QmitkMultiNodeSelectionWidget.h"
#include "QmitkSingleNodeSelectionWidget.h"
// Qt
#include <QHBoxLayout>
#include <QMessageBox>
#include <QProcess>
#include <QVBoxLayout>
#include <qfiledialog.h>
// mitk image
#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkImage3DSliceToImage2DFilter.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkPointSet.h>

// itk
#include <itksys/SystemTools.hxx>
#include <m2ElxRegistrationHelper.h>
#include <m2ElxUtil.h>

const std::string OpticalImageRegistration::VIEW_ID = "org.mitk.views.opticalimageregistration";

void OpticalImageRegistration::SetFocus() {}

void OpticalImageRegistration::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);

  connect(m_Controls.btnStartRegistration, &QPushButton::clicked, this, &OpticalImageRegistration::StartRegistration);

  m_FixedImageSingleNodeSelection = new QmitkSingleNodeSelectionWidget();
  m_FixedImageSingleNodeSelection->SetDataStorage(GetDataStorage());
  m_FixedImageSingleNodeSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::Image>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_FixedImageSingleNodeSelection->SetSelectionIsOptional(true);
  m_FixedImageSingleNodeSelection->SetEmptyInfo(QString("Please select a fixed image"));
  m_FixedImageSingleNodeSelection->SetPopUpTitel(QString("Select fixed image node"));

  m_Controls.fixedImageData->insertWidget(0, m_FixedImageSingleNodeSelection);

  m_FixedPointSetSingleNodeSelection = new QmitkSingleNodeSelectionWidget();
  m_FixedPointSetSingleNodeSelection->SetDataStorage(GetDataStorage());
  m_FixedPointSetSingleNodeSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::PointSet>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_FixedPointSetSingleNodeSelection->SetSelectionIsOptional(true);
  m_FixedPointSetSingleNodeSelection->SetEmptyInfo(QString("Please select a fixed point set"));
  m_FixedPointSetSingleNodeSelection->SetPopUpTitel(QString("Select fixed point set node"));

  m_Controls.fixedPointSetData->insertWidget(1, m_FixedPointSetSingleNodeSelection);

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
            auto fixedNode = m_FixedImageSingleNodeSelection->GetSelectedNode();
            if (fixedNode)
              this->GetDataStorage()->Add(node, fixedNode);
            else
              this->GetDataStorage()->Add(node);
            node->SetFloatProperty("point 2D size", 0.5);
            node->SetProperty("color", mitk::ColorProperty::New(1.0f, 0.0f, 0.0f));
            this->m_FixedPointSetSingleNodeSelection->SetCurrentSelection({node});
          });
}

void OpticalImageRegistration::AddNewModalityTab()
{
  auto widget = new QWidget();
  m_MovingModalitiesControls[m_ModalityId].setupUi(widget);
  const auto modalityId = m_ModalityId;
  const auto tabId = m_Controls.tabWidget->addTab(widget, QString(m_ModalityId));
  m_Controls.tabWidget->setCurrentIndex(tabId);
  auto &controls = m_MovingModalitiesControls[m_ModalityId];

  controls.lineEditName->setPlaceholderText("Type in a new name for this a modality here.");
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
  imageSelection->SetEmptyInfo(QString("Please select a moving image"));
  imageSelection->SetPopUpTitel(QString("Select moving image node"));
  // m_Controls.movingImageData->insertWidget(0, m_MovingImageSingleNodeSelection);
  controls.widgetList->insertWidget(1, imageSelection);

  connect(imageSelection,
          &QmitkSingleNodeSelectionWidget::CurrentSelectionChanged,
          this,
          [tabId, modalityId, this](QList<mitk::DataNode::Pointer> nodeList)
          {
            if (nodeList.size() != 1)
            {
              MITK_ERROR << "Not clear what to do!";
              return;
            }
            const auto node = nodeList.back();
            if (auto image = dynamic_cast<mitk::Image *>(node->GetData()))
            {
              m_MovingModalitiesDualRepresentation[modalityId].reset(new m2::DualGeometryImageWrapper(image));
            }
          });

  auto pointSetSelection = new QmitkSingleNodeSelectionWidget();
  pointSetSelection->SetDataStorage(GetDataStorage());
  pointSetSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::PointSet>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  pointSetSelection->SetSelectionIsOptional(true);
  pointSetSelection->SetEmptyInfo(QString("Please select a moving point set"));
  pointSetSelection->SetPopUpTitel(QString("Select moving point set node"));
  controls.movingPointSetData->addWidget(pointSetSelection);

  connect(controls.addMovingPointSet,
          &QPushButton::clicked,
          this,
          [&]()
          {
            auto node = mitk::DataNode::New();
            node->SetData(mitk::PointSet::New());
            node->SetName("MovingPointSet");
            auto movingNode = imageSelection->GetSelectedNode();
            if (movingNode)
              this->GetDataStorage()->Add(node, movingNode);
            else
              this->GetDataStorage()->Add(node);
            node->SetFloatProperty("point 2D size", 0.5);
            node->SetProperty("color", mitk::ColorProperty::New(0.0f, 1.0f, 1.0f));
            pointSetSelection->SetCurrentSelection({node});
          });

  connect(controls.btnRemove,
          &QPushButton::clicked,
          this,
          [this, tabId, modalityId]()
          {
            this->m_Controls.tabWidget->removeTab(tabId);
            // this->m_MovingModalitiesControls[]
          });

  ++m_ModalityId;
}

void OpticalImageRegistration::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*part*/,
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

QString OpticalImageRegistration::GetElastixPath() const
{
  berry::IPreferences::Pointer preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.ext.externalprograms");

  return preferences.IsNotNull() ? preferences->Get("elastix", "") : "";
}

void OpticalImageRegistration::StartRegistration()
{
  // check if
  auto elastix = m2::ElxUtil::Executable("elastix");
  if (elastix.empty())
  {
    elastix = GetElastixPath().toStdString();
    if (elastix.empty())
    {
      QMessageBox::information(nullptr, "Error", "No elastix executable specified in the MITK properties!");
      return;
    }
  }

  mitk::PointSet::Pointer fixedPointSet;
  mitk::Image::Pointer fixedImage;

  if (auto node = m_FixedImageSingleNodeSelection->GetSelectedNode())
    fixedImage = dynamic_cast<mitk::Image *>(node->GetData());

  if (auto node = m_FixedPointSetSingleNodeSelection->GetSelectedNode())
    fixedPointSet = dynamic_cast<mitk::PointSet *>(node->GetData());

  // const auto rigid = m_Controls.rigidText->toPlainText().toStdString();
  // const auto deformable = m_Controls.deformableText->toPlainText().toStdString();

  for (const auto kv : m_MovingModalitiesDualRepresentation)
  {
    MITK_INFO << "Process modality with ID [" << kv.first << "] ";
    auto dualRepresentation = kv.second.get();
    mitk::Image::Pointer movingImage = dualRepresentation->GetWrappedImage();
    mitk::PointSet::Pointer movingPointSet;
    auto movingNode = dynamic_cast<QmitkSingleNodeSelectionWidget *>(
                        m_MovingModalitiesControls[kv.first].widgetList->itemAt(1)->widget())
                        ->GetSelectedNode();

    if (auto pointSetNodeSelection = dynamic_cast<QmitkSingleNodeSelectionWidget *>(
          m_MovingModalitiesControls[kv.first].movingPointSetData->itemAt(1)->widget()))
    {
      if(auto pointSetNode = pointSetNodeSelection->GetSelectedNode())
        movingPointSet = dynamic_cast<mitk::PointSet *>(pointSetNode->GetData());
    }

    try
    {
      m2::ElxRegistrationHelper helper;
      helper.SetAdditionalBinarySearchPath(itksys::SystemTools::GetParentDirectory(elastix));
      helper.SetImageData(fixedImage, movingImage);
      helper.SetPointData(fixedPointSet, movingPointSet);
      // helper.SetRegistrationParameters({rigid, deformable});
      helper.GetRegistration();

      auto warpedImage = helper.WarpImage(movingImage);
      auto node = mitk::DataNode::New();
      node->SetData(warpedImage);
      node->SetName("Warped Image");
      GetDataStorage()->Add(node, movingNode);
    }
    catch (std::exception &e)
    {
      MITK_ERROR << e.what();
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

