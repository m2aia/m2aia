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

const std::string OpticalImageRegistration::VIEW_ID = "org.mitk.views.opticalimageregistration";

void OpticalImageRegistration::SetFocus() {}

void OpticalImageRegistration::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);

  //  connect(m_Controls.btnSetOrigin, &QPushButton::clicked, this, &OpticalImageRegistration::DoSetOrigin);
  connect(m_Controls.btnStartProcess, &QPushButton::clicked, this, &OpticalImageRegistration::DoImageProcessing);

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

  m_MovingImageSingleNodeSelection = new QmitkSingleNodeSelectionWidget();
  m_MovingImageSingleNodeSelection->SetDataStorage(GetDataStorage());
  m_MovingImageSingleNodeSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::Image>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_MovingImageSingleNodeSelection->SetSelectionIsOptional(true);
  m_MovingImageSingleNodeSelection->SetEmptyInfo(QString("Please select a moving image"));
  m_MovingImageSingleNodeSelection->SetPopUpTitel(QString("Select moving image node"));

  m_Controls.movingImageData->insertWidget(0, m_MovingImageSingleNodeSelection);

  m_MovingPointSetSingleNodeSelection = new QmitkSingleNodeSelectionWidget();
  m_MovingPointSetSingleNodeSelection->SetDataStorage(GetDataStorage());
  m_MovingPointSetSingleNodeSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::PointSet>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_MovingPointSetSingleNodeSelection->SetSelectionIsOptional(true);
  m_MovingPointSetSingleNodeSelection->SetEmptyInfo(QString("Please select a moving point set"));
  m_MovingPointSetSingleNodeSelection->SetPopUpTitel(QString("Select moving point set node"));

  m_Controls.movingPointSetData->insertWidget(1, m_MovingPointSetSingleNodeSelection);

  m_Controls.addFixedPointSet->setToolTip("Add point set for the fixed image reference points");
  m_Controls.addFixedPointSet->setToolTip("Add point set for the moving image reference points");
  connect(m_Controls.addFixedPointSet, &QPushButton::clicked, this, [&]() {
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
  connect(m_Controls.addMovingPointSet, &QPushButton::clicked, this, [&]() {
    auto node = mitk::DataNode::New();
    node->SetData(mitk::PointSet::New());
    node->SetName("MovingPointSet");
    auto movingNode = m_MovingImageSingleNodeSelection->GetSelectedNode();
    if (movingNode)
      this->GetDataStorage()->Add(node, movingNode);
    else
      this->GetDataStorage()->Add(node);
    node->SetFloatProperty("point 2D size", 0.5);
    node->SetProperty("color", mitk::ColorProperty::New(0.0f, 1.0f, 1.0f));
    this->m_MovingPointSetSingleNodeSelection->SetCurrentSelection({node});
  });

  connect(m_Controls.btnSelectDir, &QPushButton::clicked, this, [&, this]() {
    auto result = QFileDialog::getExistingDirectory(nullptr, "Select the working directory for image registration");
    m_Controls.workingDirectory->setText(result);
  });
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

void OpticalImageRegistration::DoSetOrigin()
{
  auto fixedNode = m_FixedImageSingleNodeSelection->GetSelectedNode();
  auto movingNode = m_MovingImageSingleNodeSelection->GetSelectedNode();
  if (fixedNode.IsNull() || movingNode.IsNull())
  {
    QMessageBox::information(nullptr, "Warning", "Both, fixed and moving image, must be specified!");
    return;
  }

  mitk::Point3D p;
  p.Fill(0);
  if (auto image = dynamic_cast<mitk::Image *>(fixedNode->GetData()))
    image->SetOrigin(p);

  if (auto image = dynamic_cast<mitk::Image *>(movingNode->GetData()))
    image->SetOrigin(p);

  mitk::DataStorage::SetOfObjects::Pointer nodes = mitk::DataStorage::SetOfObjects::New();
  nodes->InsertElement(nodes->Size(), fixedNode);

  auto boundingGeometry = GetDataStorage()->ComputeBoundingGeometry3D(nodes, "visible", nullptr);
  mitk::RenderingManager::GetInstance()->InitializeViews(boundingGeometry);

  fixedNode->SetVisibility(true);
  movingNode->SetVisibility(false);
}

void OpticalImageRegistration::DoImageProcessing()
{
  auto elastix = GetElastixPath();
  if (elastix.isEmpty())
  {
    QMessageBox::information(nullptr, "Error", "No elastix executable specified in the MITK properties!");
    return;
  }

  auto fixedNode = m_FixedImageSingleNodeSelection->GetSelectedNode();
  auto movingNode = m_MovingImageSingleNodeSelection->GetSelectedNode();
  if (fixedNode.IsNull() || movingNode.IsNull())
  {
    QMessageBox::information(nullptr, "Error", "Both, fixed and moving image, must be specified!");
    return;
  }

  using itksys::SystemTools;

  auto path = m_Controls.workingDirectory->text().toStdString();
  QStringList cmd;

  auto rigidPath = SystemTools::ConvertToOutputPath(SystemTools::JoinPath({path, "/", "rigid.txt"}));
  {
    std::ofstream o(rigidPath);
    o << m_Controls.rigidText->toPlainText().toStdString();
    o.close();
  }

  auto deformablePath = SystemTools::ConvertToOutputPath(SystemTools::JoinPath({path, "/", "deformable.txt"}));
  {
    std::ofstream o(deformablePath);
    o << m_Controls.deformableText->toPlainText().toStdString();
    o.close();
  }

  cmd << elastix;
  cmd << "-out" << path.c_str();
  cmd << "-p" << rigidPath.c_str();
  cmd << "-p" << deformablePath.c_str();

  if (auto image = dynamic_cast<mitk::Image *>(fixedNode->GetData()))
  {
    auto fixedPath = SystemTools::ConvertToOutputPath(SystemTools::JoinPath({path, "/", "fixed.nrrd"}));
    auto filter = mitk::Image3DSliceToImage2DFilter::New();
    filter->SetInput(image);
    filter->Update();
    mitk::IOUtil::Save(filter->GetOutput(), fixedPath);
    cmd << "-f" << (fixedPath.c_str());
  }

  if (auto image = dynamic_cast<mitk::Image *>(movingNode->GetData()))
  {
    auto movingPath = SystemTools::ConvertToOutputPath(SystemTools::JoinPath({path, "/", "moving.nrrd"}));
    auto filter = mitk::Image3DSliceToImage2DFilter::New();
    filter->SetInput(image);
    filter->Update();

    mitk::IOUtil::Save(filter->GetOutput(), movingPath);
    cmd << "-m" << QString(movingPath.c_str());
  }

  if (auto pntsNode = m_FixedPointSetSingleNodeSelection->GetSelectedNode())
  {
    if (auto pnts = dynamic_cast<mitk::PointSet *>(pntsNode->GetData()))
    {
      mitk::IOUtil::Save(pnts, SystemTools::ConvertToOutputPath(SystemTools::JoinPath({path, "/", "fixed.mps"})));
      auto pntsPath = SystemTools::ConvertToOutputPath(SystemTools::JoinPath({path, "/", "fixed.txt"}));
      Save(pnts, pntsPath);
      cmd << "-fp" << QString(pntsPath.c_str());
    }
  }

  if (auto pntsNode = m_MovingPointSetSingleNodeSelection->GetSelectedNode())
  {
    if (auto pnts = dynamic_cast<mitk::PointSet *>(pntsNode->GetData()))
    {
      mitk::IOUtil::Save(pnts, SystemTools::ConvertToOutputPath(SystemTools::JoinPath({path, "/", "moving.mps"})));
      auto pntsPath = SystemTools::ConvertToOutputPath(SystemTools::JoinPath({path, "/", "moving.txt"}));
      Save(pnts, pntsPath);
      cmd << "-mp" << QString(pntsPath.c_str());
    }
  }

  QProcess call;
  call.execute(cmd.join(" "));

  auto resultPath = SystemTools::ConvertToOutputPath(SystemTools::JoinPath({path, "/", "result.1.nrrd"}));
  MITK_INFO << "Result image path: " << resultPath;
  if (SystemTools::FileExists(resultPath))
  {
    auto vData = mitk::IOUtil::Load(resultPath);
    mitk::Image::Pointer image = dynamic_cast<mitk::Image *>(vData.back().GetPointer());
    auto node = mitk::DataNode::New();
    node->SetName(movingNode->GetName() + "_result");
    node->SetData(image);
    this->GetDataStorage()->Add(node, movingNode);
  }

  // filesystem::remove(path);
}

void OpticalImageRegistration::Save(mitk::PointSet::Pointer pnts, std::string path) const
{
  std::ofstream f(path);
  f << "point\n";
  f << std::to_string(pnts->GetSize()) << "\n";
  for (auto it = pnts->Begin(); it != pnts->End(); ++it)
  {
    const auto &p = it->Value();
    f << p[0] << " " << p[1] << "\n";
  }

  f.close();
}
