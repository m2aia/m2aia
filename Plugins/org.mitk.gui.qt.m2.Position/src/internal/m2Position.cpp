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
#include "m2Position.h"

// Qt
#include <QMessageBox>

// mitk image
#include <mitkImage.h>
#include <mitkImageAccessByItk.h>
#include <mitkNodePredicateDataType.h>
#include <mitkPointSet.h>
#include <mitkRotationOperation.h>
#include <mitkScaleOperation.h>
//#include <mitk>
#include <m2ImzMLMassSpecImage.h>
#include <mitkApplyTransformMatrixOperation.h>
#include <mitkInteractionConst.h>
#include <qpushbutton.h>

const std::string m2Position::VIEW_ID = "org.mitk.views.m2.Position";

void m2Position::SetFocus() {}

void m2Position::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  connect(m_Controls.btnPlus5, &QPushButton::clicked, [this]() { this->Rotate(5); });
  connect(m_Controls.btnPlus15, &QPushButton::clicked, [this]() { this->Rotate(15); });
  connect(m_Controls.btnPlus45, &QPushButton::clicked, [this]() { this->Rotate(45); });
  connect(m_Controls.btnPlus90, &QPushButton::clicked, [this]() { this->Rotate(90); });
  connect(m_Controls.btnMinus5, &QPushButton::clicked, [this]() { this->Rotate(-5); });
  connect(m_Controls.btnMinus15, &QPushButton::clicked, [this]() { this->Rotate(-15); });
  connect(m_Controls.btnMinus45, &QPushButton::clicked, [this]() { this->Rotate(-45); });
  connect(m_Controls.btnMinus90, &QPushButton::clicked, [this]() { this->Rotate(-90); });
  // connect(m_Controls.mirrorH, &QPushButton::clicked, [this]() {this->Mirror(0); });
  // connect(m_Controls.mirrorV, &QPushButton::clicked, [this]() {this->Mirror(1); });

  connect(m_Controls.btnLeft, &QPushButton::clicked, [this]() { this->Move({-m_Controls.spnBxStepWidth->value(), 0}); });
  connect(m_Controls.btnRight, &QPushButton::clicked, [this]() { this->Move({m_Controls.spnBxStepWidth->value(), 0}); });
  connect(m_Controls.btnUp, &QPushButton::clicked, [this]() { this->Move({0, -m_Controls.spnBxStepWidth->value()}); });
  connect(m_Controls.btnDown, &QPushButton::clicked, [this]() { this->Move({0, m_Controls.spnBxStepWidth->value()}); });
}

void m2Position::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/,
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

void m2Position::Move(std::array<int, 2> &&vec)
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  for(auto node : nodes){
  mitk::BaseData *data = node->GetData();
  if (data)
  {
    // test if this data item is an image or not (could also be a surface or something totally different)
    
    if (m2::SpectrumImageBase *image = dynamic_cast<m2::SpectrumImageBase *>(data))
    {
      image->ApplyMoveOriginOperation(vec);
    }
    else if (mitk::Image *image = dynamic_cast<mitk::Image *>(data))
    {
      auto geometry = image->GetGeometry();
      auto pos = geometry->GetOrigin();
      auto space = geometry->GetSpacing();
      pos[0] = pos[0] + vec.at(0) * space[0];
      pos[1] = pos[1] + vec.at(1) * space[1];
      geometry->SetOrigin(pos);
	}

    RequestRenderWindowUpdate();
    auto deriv = GetDataStorage()->GetDerivations(node, mitk::TNodePredicateDataType<mitk::PointSet>::New());
    if (deriv->Size())
    {
      for (auto p : *deriv)
      {
        auto geometry = p->GetData()->GetGeometry();
        auto pos = geometry->GetOrigin();
        auto space = geometry->GetSpacing();
        pos[0] = pos[0] + vec.at(0) * space[0];
        pos[1] = pos[1] + vec.at(1) * space[1];
        geometry->SetOrigin(pos);
      }
      RequestRenderWindowUpdate();
    }
  }
  }
}

void m2Position::Mirror(int w)
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  for(auto node : nodes){
  mitk::BaseData *data = node->GetData();
  if (data)
  {
    // test if this data item is an image or not (could also be a surface or something totally different)
    m2::SpectrumImageBase *image = dynamic_cast<m2::SpectrumImageBase *>(data);
    if (image)
    {
      std::unique_ptr<mitk::ApplyTransformMatrixOperation> op;

      vtkSmartPointer<vtkMatrix4x4> m = vtkMatrix4x4::New();
      m->Identity();
      m->SetElement(w, w, -1.0);
      op.reset(new mitk::ApplyTransformMatrixOperation(
        mitk::EOperations::OpAPPLYTRANSFORMMATRIX, m, image->GetGeometry()->GetCenter()));

      image->ApplyGeometryOperation(op.get());
      RequestRenderWindowUpdate();
    }
  }
  }
}

void m2Position::Rotate(int angleDeg)
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  for(auto node : nodes){

  mitk::BaseData *data = node->GetData();
  if (data)
  {
    // test if this data item is an image or not (could also be a surface or something totally different)
    m2::SpectrumImageBase *image = dynamic_cast<m2::SpectrumImageBase *>(data);
    std::unique_ptr<mitk::RotationOperation> op;
    mitk::ScalarType rotAx[3] = {0, 0, 1};
    op.reset(new mitk::RotationOperation(
      mitk::EOperations::OpROTATE, image->GetGeometry()->GetCenter(), mitk::Vector3D(rotAx), angleDeg));
    if (image)
    {
      image->ApplyGeometryOperation(op.get());
      RequestRenderWindowUpdate();
      // actually do something here...
    }
    auto deriv = GetDataStorage()->GetDerivations(node, mitk::TNodePredicateDataType<mitk::PointSet>::New());
    if (deriv->Size())
    {
      for (auto p : *deriv)
      {
		  auto manipulatedGeometry = p->GetData()->GetGeometry()->Clone();
		  manipulatedGeometry->ExecuteOperation(op.get());
		  p->GetData()->GetGeometry()->SetIdentity();
		  p->GetData()->GetGeometry()->Compose(manipulatedGeometry->GetIndexToWorldTransform());
      }
      RequestRenderWindowUpdate();
    }
  }
    }
}
