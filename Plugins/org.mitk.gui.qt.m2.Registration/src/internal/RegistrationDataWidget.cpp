/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "RegistrationDataWidget.h"

#include <QFileDialog>
#include <QMessageBox>

#include <m2ElxRegistrationHelper.h>
#include <mitkDataStorage.h>
#include <mitkImage.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkPointSet.h>

RegistrationDataWidget::RegistrationDataWidget(QWidget *parent, mitk::DataStorage::Pointer storage)
  : QWidget(parent), m_Parent(parent), m_DataStorage(storage)
{
  m_Controls.setupUi(this);
  m_Controls.imageSelection->SetDataStorage(m_DataStorage);
  
  m_RegistrationData = std::make_shared<RegistrationData>();

  // configure image selection
  m_Controls.imageSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::Image>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.imageSelection->SetSelectionIsOptional(true);
  m_Controls.imageSelection->SetEmptyInfo(QString("Select image"));
  m_Controls.imageSelection->SetPopUpTitel(QString("Select image node"));

  // configure image mask selection
  m_Controls.imageMaskSelection->SetDataStorage(m_DataStorage);
  m_Controls.imageMaskSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::Image>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.imageMaskSelection->SetSelectionIsOptional(true);
  m_Controls.imageMaskSelection->SetEmptyInfo(QString("Select image mask"));
  m_Controls.imageMaskSelection->SetPopUpTitel(QString("Select image mask node"));

  // configure image mask selection
  m_Controls.pointSetSelection->SetDataStorage(m_DataStorage);
  m_Controls.pointSetSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::PointSet>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.pointSetSelection->SetSelectionIsOptional(true);
  m_Controls.pointSetSelection->SetEmptyInfo(QString("Select point set"));
  m_Controls.pointSetSelection->SetPopUpTitel(QString("Select point set node"));

  connect(m_Controls.btnLoadPreset, SIGNAL(clicked()), this, SLOT(OnLoadTransformations()));
  connect(m_Controls.btnSaveTransforms, SIGNAL(clicked()), this, SLOT(OnSaveTransformations()));
  connect(m_Controls.btnApplyTransforms, SIGNAL(clicked()), this, SLOT(OnApplyTransformations()));
  connect(m_Controls.btnRemove, SIGNAL(clicked()), this, SIGNAL(RemoveSelf()));
  connect(m_Controls.addMovingPointSet, SIGNAL(clicked()), this, SLOT(OnAddPointSet()));
}

RegistrationDataWidget::~RegistrationDataWidget() {}

void RegistrationDataWidget::SetDataStorage(mitk::DataStorage::Pointer storage){
  m_DataStorage = storage;
}

void RegistrationDataWidget::OnLoadTransformations()
{
  const QString filter = tr("Elastix Parameterfile (*.txt)");
  const auto paths = QFileDialog::getOpenFileNames(m_Parent, "Load elastix transform parameter files.", "", filter);
  m_RegistrationData->m_Transformations.clear();
  for (auto p : paths)
  {
    std::ifstream reader(paths[0].toStdString());
    std::string s((std::istreambuf_iterator<char>(reader)), std::istreambuf_iterator<char>());
    m_RegistrationData->m_Transformations.push_back(s);
  }
}

void RegistrationDataWidget::OnSaveTransformations()
{
  const QString filter = tr("Elastix Parameterfile (*.txt)");
  QString path;
  unsigned int i = 0;
  for (auto t : m_RegistrationData->m_Transformations)
  {
    const auto name = m_RegistrationData->m_Name + "_transform" + std::to_string(i) + ".txt";
    auto spath = path.split('/');
    spath.pop_back();
    spath.push_back(name.c_str());
    path = spath.join('/');
    path = QFileDialog::getSaveFileName(
      m_Parent, tr("Save transformation ") + std::to_string(i++).c_str() + " file", path, filter);
    std::ofstream(path.toStdString()) << t;
  }
}

void RegistrationDataWidget::OnApplyTransformations()
{
  UpdateRegistrationDataFromUI();
  m2::ElxRegistrationHelper warpingHelper;
  const auto node = m_Controls.imageSelection->GetSelectedNode();
  const auto image = dynamic_cast<const mitk::Image *>(node->GetData());
  warpingHelper.SetTransformations(m_RegistrationData->m_Transformations);
  auto result = warpingHelper.WarpImage(image);
  auto newNode = mitk::DataNode::New();
  newNode->SetData(result);
  newNode->SetName(node->GetName() + "(warped)");
  m_DataStorage->Add(newNode, node);
}

void RegistrationDataWidget::OnAddPointSet()
{
  if (HasImage())
  {
    auto imageNode = GetImageNode();
    auto node = mitk::DataNode::New();
    node->SetData(mitk::PointSet::New());
    node->SetName(imageNode->GetName() + " (points)");
    auto s = GetImage()->GetGeometry()->GetSpacing()[0];
    node->SetFloatProperty("point 2D size", s);
    node->SetProperty("color", mitk::ColorProperty::New(0.0f, 1.0f, 1.0f));
    m_DataStorage->Add(node, imageNode);
    m_Controls.pointSetSelection->SetCurrentSelection({node});
  }else{
    QMessageBox::information(m_Parent, "Missing image selection!","Please select an image before proceeding. No image has been selected currently.");
  }

}

void RegistrationDataWidget::UpdateRegistrationDataFromUI()
{
  
  m_RegistrationData->m_Image = GetImage();
  if(HasImage()) m_RegistrationData->m_Name = GetImageNode()->GetName();
  m_RegistrationData->m_Mask = GetMask();
  m_RegistrationData->m_PointSet = GetPointSet();
  
}

std::shared_ptr<RegistrationData> RegistrationDataWidget::GetRegistrationData()
{
  UpdateRegistrationDataFromUI();
  return m_RegistrationData;
}

const RegistrationData* RegistrationDataWidget::GetRegistrationData() const
{
  return m_RegistrationData.get();
}

bool RegistrationDataWidget::HasImage() const
{
  return m_Controls.imageSelection->GetSelectedNode().IsNotNull();
}

bool RegistrationDataWidget::HasMask() const
{
  return m_Controls.imageMaskSelection->GetSelectedNode().IsNotNull();
}

bool RegistrationDataWidget::HasPointSet() const
{
  return m_Controls.pointSetSelection->GetSelectedNode().IsNotNull();
}

mitk::DataNode::Pointer RegistrationDataWidget::GetImageNode() const
{
  return m_Controls.imageSelection->GetSelectedNode();
}

mitk::DataNode::Pointer RegistrationDataWidget::GetMaskNode() const
{
  return m_Controls.imageMaskSelection->GetSelectedNode();
}

mitk::DataNode::Pointer RegistrationDataWidget::GetPointSetNode() const
{
  return m_Controls.pointSetSelection->GetSelectedNode();
}

int RegistrationDataWidget::GetIndex() const{
  return m_Controls.index->value();
}


mitk::Image::Pointer RegistrationDataWidget::GetImage() const
{
  if(HasImage())
    return dynamic_cast<mitk::Image *>(GetImageNode()->GetData());
  return nullptr;
}

mitk::Image::Pointer RegistrationDataWidget::GetMask() const
{
  if(HasMask())
    return dynamic_cast<mitk::Image *>(GetMaskNode()->GetData());
  return nullptr;
}


mitk::PointSet::Pointer RegistrationDataWidget::GetPointSet() const {
  if(HasPointSet())
    return dynamic_cast<mitk::PointSet *>(GetPointSetNode()->GetData());
  return nullptr;
}

bool RegistrationDataWidget::HasTransformations() const{
  return !GetRegistrationData()->m_Transformations.empty();
}

void RegistrationDataWidget::SetTransformations(const std::vector<std::string> & data){
  GetRegistrationData()->m_Transformations = data;
}

std::vector<std::string> RegistrationDataWidget::GetTransformations() const{
  return GetRegistrationData()->m_Transformations;
}

void RegistrationDataWidget::EnableButtons(bool enable){  
    m_Controls.btnApplyTransforms->setVisible(enable);
    m_Controls.btnLoadPreset->setVisible(enable);
    m_Controls.btnRemove->setVisible(enable);
    m_Controls.btnSaveTransforms->setVisible(enable);
}