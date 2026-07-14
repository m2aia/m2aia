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
#include <QTableWidgetItem>
#include <vnl/vnl_matrix_fixed.h>
#include <mitkLabelSetImage.h>
#include <mitkNodePredicateOr.h>

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

  // Spacing / Origin / Direction
  m_Controls.tblDirection->horizontalHeader()->setStretchLastSection(true);
  m_Controls.tblDirection->verticalHeader()->setDefaultSectionSize(22);

  connect(m_Controls.btnApplySpacing, &QPushButton::clicked, this, &m2Position::ApplySpacing);
  connect(m_Controls.btnApplyOrigin,  &QPushButton::clicked, this, &m2Position::ApplyOrigin);
  connect(m_Controls.btnRefreshGeometry, &QPushButton::clicked, this, &m2Position::RefreshGeometryInfo);
}

void m2Position::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/,
                                    const QList<mitk::DataNode::Pointer> &nodes)
{
  foreach (mitk::DataNode::Pointer node, nodes)
  {
    if (node.IsNotNull())
    {
      auto *d = node->GetData();
      if (dynamic_cast<mitk::Image *>(d) || dynamic_cast<mitk::MultiLabelSegmentation *>(d))
      {
        RefreshGeometryInfo();
        return;
      }
    }
  }
}

// ─── Spacing ─────────────────────────────────────────────────────────────────

void m2Position::ApplySpacingToData(mitk::BaseData *data, const mitk::Vector3D &spacing)
{
  data->GetGeometry()->SetSpacing(spacing);
}

void m2Position::ApplySpacing()
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  mitk::Vector3D spacing;
  spacing[0] = m_Controls.spnSpacingX->value();
  spacing[1] = m_Controls.spnSpacingY->value();
  spacing[2] = m_Controls.spnSpacingZ->value();

  auto pred = mitk::NodePredicateOr::New(
    mitk::TNodePredicateDataType<mitk::Image>::New(),
    mitk::TNodePredicateDataType<mitk::MultiLabelSegmentation>::New());

  for (auto node : nodes)
  {
    auto *d = node->GetData();
    if (dynamic_cast<mitk::Image *>(d) || dynamic_cast<mitk::MultiLabelSegmentation *>(d))
    {
      ApplySpacingToData(d, spacing);
      auto derived = GetDataStorage()->GetDerivations(node, pred);
      for (auto dd : *derived)
        ApplySpacingToData(dd->GetData(), spacing);
    }
    node->GetData()->Modified();
  }

  if (m_Controls.chkAutoFitView->isChecked())
    mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
  RequestRenderWindowUpdate();
  RefreshGeometryInfo();
}

// ─── Origin ──────────────────────────────────────────────────────────────────

void m2Position::ApplyOriginToData(mitk::BaseData *data, const mitk::Point3D &origin)
{
  data->GetGeometry()->SetOrigin(origin);
}

void m2Position::ApplyOrigin()
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  mitk::Point3D origin;
  origin[0] = m_Controls.spnOriginX->value();
  origin[1] = m_Controls.spnOriginY->value();
  origin[2] = m_Controls.spnOriginZ->value();

  auto pred = mitk::NodePredicateOr::New(
    mitk::TNodePredicateDataType<mitk::Image>::New(),
    mitk::TNodePredicateDataType<mitk::MultiLabelSegmentation>::New());

  for (auto node : nodes)
  {
    auto *d = node->GetData();

    if(auto *mls = dynamic_cast<mitk::MultiLabelSegmentation *>(d))
      d= mls->GetGroupImage(0);

    if (dynamic_cast<mitk::Image *>(d))
    {
      ApplyOriginToData(d, origin);
      auto derived = GetDataStorage()->GetDerivations(node, pred);
      for (auto dd : *derived)
        ApplyOriginToData(dd->GetData(), origin);
    }
    node->GetData()->Modified();
  }

  if (m_Controls.chkAutoFitView->isChecked())
    mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
  RequestRenderWindowUpdate();
  RefreshGeometryInfo();
}

// ─── Geometry info refresh ────────────────────────────────────────────────────

void m2Position::RefreshGeometryInfo()
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  mitk::BaseGeometry *geo = nullptr;
  for (auto node : nodes)
  {
    if (node.IsNotNull())
    {
      auto *d = node->GetData();

      if(auto *mls = dynamic_cast<mitk::MultiLabelSegmentation *>(d))
        d = mls->GetGroupImage(0);

      if (dynamic_cast<mitk::Image *>(d))
        geo = d->GetGeometry();
    }
    if (geo)
      break;
  }

  if (!geo)
    return;
  const auto &sp = geo->GetSpacing();
  m_Controls.spnSpacingX->setValue(sp[0]);
  m_Controls.spnSpacingY->setValue(sp[1]);
  m_Controls.spnSpacingZ->setValue(sp[2]);

  const auto &orig = geo->GetOrigin();
  m_Controls.spnOriginX->setValue(orig[0]);
  m_Controls.spnOriginY->setValue(orig[1]);
  m_Controls.spnOriginZ->setValue(orig[2]);

  // Direction matrix: columns are unit world directions of image axes I, J, K
  const auto *mat = geo->GetIndexToWorldTransform()->GetMatrix().GetVnlMatrix().data_block();
  // mat is row-major 3x3: mat[row*3 + col]
  for (int col = 0; col < 3; ++col)
  {
    double len = 0.0;
    for (int row = 0; row < 3; ++row)
      len += mat[row * 3 + col] * mat[row * 3 + col];
    len = std::sqrt(len);
    if (len < 1e-10) len = 1.0;
    for (int row = 0; row < 3; ++row)
    {
      auto *item = new QTableWidgetItem(QString::number(mat[row * 3 + col] / len, 'f', 6));
      item->setTextAlignment(Qt::AlignCenter);
      m_Controls.tblDirection->setItem(row, col, item);
    }
  }
  m_Controls.tblDirection->resizeColumnsToContents();
}

// ─────────────────────────────────────────────────────────────────────────────

void m2Position::MoveData(mitk::BaseData *data, std::array<float, 2> vec)
{
  mitk::Vector3D v;
  v[0] = vec[0];
  v[1] = vec[1];
  v[2] = 0;
  data->GetGeometry()->Translate(v);
}

void m2Position::Move(std::array<float, 2> vec)
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  auto pred = mitk::NodePredicateOr::New(
    mitk::TNodePredicateDataType<mitk::Image>::New(),
    mitk::TNodePredicateDataType<mitk::MultiLabelSegmentation>::New());

  for (auto node : nodes)
  {
    auto *d = node->GetData();

    if(auto *mls = dynamic_cast<mitk::MultiLabelSegmentation *>(d))
      d = mls->GetGroupImage(0);
    
    if (dynamic_cast<mitk::Image *>(d))
    {
      MoveData(d, vec);
      d->Modified();
      auto derivedNodes = GetDataStorage()->GetDerivations(node, pred);
      for (auto derivedNode : *derivedNodes){
        MoveData(derivedNode->GetData(), vec);
        derivedNode->GetData()->Modified();
        derivedNode->Modified();
      }
      
      node->GetData()->Modified();
      node->Modified();
      if (m_Controls.chkAutoFitView->isChecked())
        mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
      RequestRenderWindowUpdate();
    }
  }
}

void m2Position::MirrorData(mitk::BaseData *data, int w)
{
  auto geometry = data->GetGeometry();
  auto t = geometry->GetVtkTransform();
  auto m = t->GetMatrix();
  auto m0 = m->GetElement(0, 0);
  auto m1 = m->GetElement(1, 1);

  m->SetElement(0, 0, (w == 0) ? -m0 : m0);
  m->SetElement(1, 1, (w == 1) ? -m1 : m1);
  geometry->SetIndexToWorldTransformByVtkMatrixWithoutChangingSpacing(m);

  const auto spacing = geometry->GetSpacing();
  // Use GetExtent() so this works for both Image and MultiLabelSegmentation
  const double extX = geometry->GetExtent(0);
  const double extY = geometry->GetExtent(1);

  if (w == 0)
  {
    if (m0 < 0)
      MoveData(data, {-1.0f * float(spacing[0] * extX), 0.0f});
    else if (m0 > 0)
      MoveData(data, {1.0f * float(spacing[0] * extX), 0.0f});
  }
  else if (w == 1)
  {
    if (m1 < 0)
      MoveData(data, {0.0f, -1.0f * float(spacing[1] * extY)});
    else if (m1 > 0)
      MoveData(data, {0.0f, 1.0f * float(spacing[1] * extY)});
  }
}

void m2Position::Mirror(int w)
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  auto pred = mitk::NodePredicateOr::New(
    mitk::TNodePredicateDataType<mitk::Image>::New(),
    mitk::TNodePredicateDataType<mitk::MultiLabelSegmentation>::New());

  for (auto node : nodes)
  {
    auto *d = node->GetData();
    if(auto *mls = dynamic_cast<mitk::MultiLabelSegmentation *>(d))
      d = mls->GetGroupImage(0);

    if (dynamic_cast<mitk::Image *>(d))
    {
      MirrorData(d, w);
      d->Modified();
      auto derivedNodes = GetDataStorage()->GetDerivations(node, pred);
      for (auto derivedNode : *derivedNodes){
        MirrorData(derivedNode->GetData(), w);
        derivedNode->GetData()->Modified();
        derivedNode->Modified();
      }
      

      node->GetData()->Modified();
      node->Modified();
      if (m_Controls.chkAutoFitView->isChecked())
        mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
      RequestRenderWindowUpdate();
    }
  }
}

void m2Position::RotateData(mitk::BaseData *data, float angleDeg, const mitk::Point3D &center)
{
  mitk::ScalarType rotAx[3] = {0, 0, 1};
  std::unique_ptr<mitk::RotationOperation> op(
    new mitk::RotationOperation(mitk::EOperations::OpROTATE, center, mitk::Vector3D(rotAx), angleDeg));
  auto manipulated = data->GetGeometry()->Clone();
  manipulated->ExecuteOperation(op.get());
  data->GetGeometry()->SetIdentity();
  data->GetGeometry()->Compose(manipulated->GetIndexToWorldTransform());
}

void m2Position::Rotate(float angleDeg)
{
  QList<mitk::DataNode::Pointer> nodes = this->GetDataManagerSelection();
  if (nodes.empty())
    return;

  auto pred = mitk::NodePredicateOr::New(
    mitk::TNodePredicateDataType<mitk::Image>::New(),
    mitk::TNodePredicateDataType<mitk::MultiLabelSegmentation>::New());

  // Compute the shared bounding-box centre for global rotation
  mitk::Point3D globalCenter;
  globalCenter.Fill(0.0);
  if (!m_Controls.chkRotateLocally->isChecked())
  {
    int count = 0;
    for (auto node : nodes)
    {
      auto *d = node->GetData();
      if(auto *mls = dynamic_cast<mitk::MultiLabelSegmentation *>(d))
        d = mls->GetGroupImage(0);  

      if (dynamic_cast<mitk::Image *>(d))
      {
        const auto &c = d->GetGeometry()->GetCenter();
        globalCenter[0] += c[0];
        globalCenter[1] += c[1];
        globalCenter[2] += c[2];
        ++count;
      }
    }
    if (count > 0)
    {
      globalCenter[0] /= count;
      globalCenter[1] /= count;
      globalCenter[2] /= count;
    }
  }

  for (auto node : nodes)
  {
    auto *d = node->GetData();
    if(auto *mls = dynamic_cast<mitk::MultiLabelSegmentation *>(d))
      d = mls->GetGroupImage(0);
    if (dynamic_cast<mitk::Image *>(d))
    {
      const mitk::Point3D center =
        m_Controls.chkRotateLocally->isChecked() ? d->GetGeometry()->GetCenter() : globalCenter;
      RotateData(d, angleDeg, center);
      auto derivedNodes = GetDataStorage()->GetDerivations(node, pred);
      for (auto derivedNode : *derivedNodes)
      {
        RotateData(derivedNode->GetData(), angleDeg, center);
        derivedNode->GetData()->Modified();
        derivedNode->Modified();
      }
      node->GetData()->Modified();
      node->Modified();
      if (m_Controls.chkAutoFitView->isChecked())
        mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
      RequestRenderWindowUpdate();
    }
  }
}
