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
#include <QShortcut>
#include <m2ImzMLSpectrumImage.h>
#include <mitkApplyTransformMatrixOperation.h>
#include <mitkInteractionConst.h>
#include <qpushbutton.h>
#include <mitkRenderingManager.h>

const std::string m2Position::VIEW_ID = "org.mitk.views.m2.Position";

void m2Position::SetFocus() {}

void m2Position::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);

  QShortcut *scRotateLeft = new QShortcut(QKeySequence(Qt::Key_Q), parent);
  QShortcut *scRotateRight = new QShortcut(QKeySequence(Qt::Key_E), parent);
  QShortcut *scUp = new QShortcut(QKeySequence(Qt::Key_W), parent);
  QShortcut *scDown = new QShortcut(QKeySequence(Qt::Key_S), parent);
  QShortcut *scLeft = new QShortcut(QKeySequence(Qt::Key_A), parent);
  QShortcut *scRight = new QShortcut(QKeySequence(Qt::Key_D), parent);
  QShortcut *scRotatePlus = new QShortcut(QKeySequence(Qt::Key_Plus), parent);
  QShortcut *scRotateMinus = new QShortcut(QKeySequence(Qt::Key_Minus), parent);
  QShortcut *scStepPlus = new QShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_Plus), parent);
  QShortcut *scStepMinus = new QShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_Minus), parent);
  QShortcut *scUpArrow = new QShortcut(QKeySequence(Qt::Key_Up), parent);
  QShortcut *scDownArrow = new QShortcut(QKeySequence(Qt::Key_Down), parent);
  QShortcut *scLeftArrow = new QShortcut(QKeySequence(Qt::Key_Left), parent);
  QShortcut *scRightArrow = new QShortcut(QKeySequence(Qt::Key_Right), parent);
  QShortcut *scRotateLeftArrow = new QShortcut(QKeySequence(Qt::Key_L), parent);
  QShortcut *scRotateRightArrow = new QShortcut(QKeySequence(Qt::Key_R), parent);

  connect(m_Controls.btnRotatePlus, &QPushButton::clicked, [this]() { this->Rotate(m_Controls.rotationBox->value()); });
  connect(m_Controls.btnRotateMinus, &QPushButton::clicked, [this]() { this->Rotate(-m_Controls.rotationBox->value()); });
  
  connect(m_Controls.mirrorH, &QPushButton::clicked, [this]() {this->Mirror(0); });
  connect(m_Controls.mirrorV, &QPushButton::clicked, [this]() {this->Mirror(1); });
  
  connect(scRotateLeft, &QShortcut::activated, [this]() { this->Rotate(-m_Controls.rotationBox->value()); });
  connect(scRotateRight, &QShortcut::activated, [this]() { this->Rotate(m_Controls.rotationBox->value()); });
  connect(scRotateLeftArrow, &QShortcut::activated, [this]() { this->Rotate(-m_Controls.rotationBox->value()); });
  connect(scRotateRightArrow, &QShortcut::activated, [this]() { this->Rotate(m_Controls.rotationBox->value()); });

  auto left = [this]() { this->Move({float(-m_Controls.spnBxStepWidth->value()* 10e-4), 0.0f}); };
  auto right = [this]() { this->Move({float(m_Controls.spnBxStepWidth->value()* 10e-4), 0.0f}); };
  auto up = [this]() { this->Move({0.0f, float(-m_Controls.spnBxStepWidth->value()* 10e-4)}); };
  auto down = [this]() { this->Move({0.0f, float(m_Controls.spnBxStepWidth->value()* 10e-4)}); };

  connect(m_Controls.btnLeft, &QPushButton::clicked, left);
  connect(m_Controls.btnRight, &QPushButton::clicked, right);
  connect(m_Controls.btnUp, &QPushButton::clicked, up);
  connect(m_Controls.btnDown, &QPushButton::clicked, down);

  connect(scStepPlus, &QShortcut::activated, [this]() { m_Controls.spnBxStepWidth->setValue(m_Controls.spnBxStepWidth->value() + 5); });
  connect(scStepMinus, &QShortcut::activated, [this]() { m_Controls.spnBxStepWidth->setValue(m_Controls.spnBxStepWidth->value() - 5); });
  connect(scRotatePlus, &QShortcut::activated, [this]() { m_Controls.rotationBox->setValue(m_Controls.rotationBox->value()*1.1); });
  connect(scRotateMinus, &QShortcut::activated, [this]() { m_Controls.rotationBox->setValue(m_Controls.rotationBox->value()*0.9); });

  connect(scUpArrow, &QShortcut::activated, up);
  connect(scDownArrow, &QShortcut::activated, down);
  connect(scLeftArrow, &QShortcut::activated, left);
  connect(scRightArrow, &QShortcut::activated, right);
  
  connect(scUp, &QShortcut::activated, up);
  connect(scDown, &QShortcut::activated, down);
  connect(scLeft, &QShortcut::activated, left);
  connect(scRight, &QShortcut::activated, right);
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

void m2Position::MoveImage(mitk::Image *image, std::array<float, 2> vec)
{
  mitk::Vector3D v;
  v[0] = vec[0];
  v[1] = vec[1];
  v[2] = 0;
  image->GetGeometry()->Translate(v);  
}

void m2Position::Move(std::array<float, 2> vec)
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  for (auto node : nodes)
  {
    mitk::BaseData *data = node->GetData();
    if (data)
    {
      if (mitk::Image *image = dynamic_cast<mitk::Image *>(data))
      {
        MoveImage(image, std::move(vec));

        auto derivedNodes = GetDataStorage()->GetDerivations(node, mitk::TNodePredicateDataType<mitk::Image>::New());
        for (auto derivedNode : *derivedNodes)
        {
          if (auto derivedImage = dynamic_cast<mitk::Image *>(derivedNode->GetData()))
          {
            MoveImage(derivedImage, vec);
          }
        }
        if (m_Controls.chkAutoFitView->isChecked())
          mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
        RequestRenderWindowUpdate();
      }
    }
  }
}

void m2Position::MirrorImage(mitk::Image * image, int w){
    auto geometry = image->GetGeometry();
    auto t = geometry->GetVtkTransform();
    auto m = t->GetMatrix();
    auto m0 = m->GetElement(0, 0);
    auto m1 = m->GetElement(1, 1);

    m->SetElement(0, 0, (w == 0) ? -m0 : m0); // flip x-axis
    m->SetElement(1, 1, (w == 1) ? -m1 : m1); // flip y-axis
    image->GetGeometry()->SetIndexToWorldTransformByVtkMatrixWithoutChangingSpacing(m);
    
    
    auto spacing = geometry->GetSpacing();
    if (w == 0) // mirrored along x-axis
    {
     if(m0 < 0) // if the x-axis was flipped
        MoveImage(image, {-1*float(spacing[0] * (image->GetDimensions()[0])), 0.0f});
      else if (m0 > 0) // if the x-axis was not flipped
        MoveImage(image, {1*float(spacing[0] * (image->GetDimensions()[0])), 0.0f});
    }
    else if (w == 1) // mirrored along y-axis
    {
      if(m1 < 0) // if the y-axis was flipped
        MoveImage(image, { 0.0f, -1*  float(spacing[1] * (image->GetDimensions()[1]))});
      else if (m1 > 0) // if the y-axis was not flipped
        MoveImage(image, { 0.0f, 1*float(spacing[1] * (image->GetDimensions()[1]))});
    }
    
}

void m2Position::Mirror(int w)
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  for (auto node : nodes)
  {
    mitk::BaseData *data = node->GetData();
    if (data)
    {
      // test if this data item is an image or not (could also be a surface or something totally different)
      
      if (auto image = dynamic_cast<mitk::Image *>(data))
      {
        MirrorImage(image, w);
        auto derivedNodes = GetDataStorage()->GetDerivations(node, mitk::TNodePredicateDataType<mitk::Image>::New());
        for (auto derivedNode : *derivedNodes)
        {
          if (auto derivedImage = dynamic_cast<mitk::Image *>(derivedNode->GetData()))
          {
            MirrorImage(derivedImage, w);
          }
        }
        if (m_Controls.chkAutoFitView->isChecked())
          mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
        RequestRenderWindowUpdate();
      }
      
      else
      {
        QMessageBox::warning(nullptr, "Warning", "This operation is only supported for images.");
      }
    }
  }
}


void m2Position::RotateImage(mitk::Image *image, float angleDeg)
{
  std::unique_ptr<mitk::RotationOperation> op;
  mitk::ScalarType rotAx[3] = {0, 0, 1};

  op.reset(new mitk::RotationOperation(
    mitk::EOperations::OpROTATE, image->GetGeometry()->GetCenter(), mitk::Vector3D(rotAx), angleDeg));
  auto manipulated = image->GetGeometry()->Clone();
  manipulated->ExecuteOperation(op.get());
  image->GetGeometry()->SetIdentity();
  image->GetGeometry()->Compose(manipulated->GetIndexToWorldTransform());


}

void m2Position::Rotate(float angleDeg)
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  for (auto node : nodes)
  {
    mitk::BaseData *data = node->GetData();
    if (data)
    {
      if(auto image = dynamic_cast<mitk::Image *>(data))
      {
        // Rotate the image
        RotateImage(image, angleDeg);
        auto derivedNodes = GetDataStorage()->GetDerivations(node, mitk::TNodePredicateDataType<mitk::Image>::New());
        for (auto derivedNode : *derivedNodes)
        {
          if (auto derivedImage = dynamic_cast<mitk::Image *>(derivedNode->GetData()))
          {
            RotateImage(derivedImage, angleDeg);
          }
        }
        // mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
        RequestRenderWindowUpdate();
      }
    } 
 
  }
}
