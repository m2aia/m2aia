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

#include "m2DataToolsView.h"

#include <QmitkRenderWindow.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2SpectrumImage.h>
#include <m2UIUtils.h>
#include <mitkLabelSetImage.h>
#include <mitkLayoutAnnotationRenderer.h>
#include <mitkLookupTableProperty.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>

const std::string m2DataToolsView::VIEW_ID = "org.mitk.views.m2.DataTools";

namespace
{
  /**
   * @brief Move a single data object to a new origin.
   *
   * mitk::MultiLabelSegmentation is derived from mitk::SlicedData and not from mitk::Image, so
   * neither a dynamic_cast to mitk::Image nor a TNodePredicateDataType<mitk::Image> reaches a
   * segmentation. Its own geometry and the geometry of every group image have to be moved here.
   */
  void SetOriginOfData(mitk::BaseData *data, const mitk::Point3D &origin)
  {
    if (nullptr == data || nullptr == data->GetGeometry())
      return;

    data->GetGeometry()->SetOrigin(origin);

    if (auto segmentation = dynamic_cast<mitk::MultiLabelSegmentation *>(data))
      for (unsigned int group = 0; group < segmentation->GetNumberOfGroups(); ++group)
        if (auto groupImage = segmentation->GetGroupImage(group))
          if (auto groupGeometry = groupImage->GetGeometry())
            groupGeometry->SetOrigin(origin);
  }

  /**
   * @brief Move a node and everything that belongs to it to a new origin.
   *
   * This covers the data of the node itself, the images a spectrum image carries internally and
   * are therefore not part of the data storage (index image, segmentation, points, normalization
   * images), and all direct child nodes. Child images and segmentations are moved to the same
   * origin, child point sets are shifted by the same delta the node itself was moved by.
   */
  void MoveNodeToOrigin(mitk::DataStorage *storage, const mitk::DataNode *node, const mitk::Point3D &origin)
  {
    auto data = node->GetData();
    if (nullptr == data || nullptr == data->GetGeometry())
      return;

    const mitk::Point3D prevOrigin = data->GetGeometry()->GetOrigin();
    SetOriginOfData(data, origin);

    if (auto spectrumImage = dynamic_cast<m2::SpectrumImage *>(data))
    {
      std::vector<mitk::BaseData *> ownedData{spectrumImage->GetIndexImage(),
                                              spectrumImage->GetMultilabelSegmentation(),
                                              spectrumImage->GetPoints()};
      for (auto kv : spectrumImage->GetNormalizationImages())
        ownedData.push_back(kv.second);

      for (auto owned : ownedData)
        SetOriginOfData(owned, origin);
    }

    const double dx = origin[0] - prevOrigin[0];
    const double dy = origin[1] - prevOrigin[1];

    // Keep the returned smart pointer alive. Iterating over *GetDerivations(...) directly would
    // free the set before the first iteration, the lifetime of the temporary is not extended.
    auto childNodes = storage->GetDerivations(node);
    for (auto child : *childNodes)
    {
      auto childData = child->GetData();

      if (auto pointSet = dynamic_cast<mitk::PointSet *>(childData))
      {
        for (auto p = pointSet->Begin(); p != pointSet->End(); ++p)
        {
          auto &point = p->Value();
          point[0] += dx;
          point[1] += dy;
        }
      }
      else if (auto image = dynamic_cast<mitk::Image *>(childData))
      {
        if (image->IsInitialized())
          SetOriginOfData(image, origin);
      }
      else if (nullptr != dynamic_cast<mitk::MultiLabelSegmentation *>(childData))
      {
        SetOriginOfData(childData, origin);
      }
    }
  }

  /**
   * @brief Restore the origin of every spectrum image from its absolute position offset properties.
   *
   * Shared by the reset buttons of the tiling and the alignment section, both restore the very
   * same state.
   */
  void ResetOriginsFromProperties(mitk::DataStorage *storage)
  {
    auto allNodes = m2::UIUtils::AllNodes(storage);
    for (auto node : *allNodes)
    {
      auto image = dynamic_cast<m2::SpectrumImage *>(node->GetData());
      if (nullptr == image)
        continue;

      mitk::Point3D origin;
      origin[0] = image->GetPropertyValue<double>("[IMS:1000053] absolute position offset x", 0);
      origin[1] = image->GetPropertyValue<double>("[IMS:1000054] absolute position offset y", 0);
      origin[2] = image->GetPropertyValue<double>("absolute position offset z", 0);

      MoveNodeToOrigin(storage, node, origin);
    }
  }
} // namespace

void m2DataToolsView::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  m_Parent = parent;

  {
    m_Controls.ReferenceLevelWindowSelection->SetDataStorage(GetDataStorage());
    m_Controls.ReferenceLevelWindowSelection->SetNodePredicate(
      mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::Image>::New(),
                                  mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
    m_Controls.ReferenceLevelWindowSelection->SetSelectionIsOptional(true);
    m_Controls.ReferenceLevelWindowSelection->SetEmptyInfo(QString("Reference image selection"));
    m_Controls.ReferenceLevelWindowSelection->SetPopUpTitel(QString("Image"));

    m_Controls.ReferenceSelectionForScaleBar->SetDataStorage(GetDataStorage());
    m_Controls.ReferenceSelectionForScaleBar->SetNodePredicate(
      mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::Image>::New(),
                                  mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
    m_Controls.ReferenceSelectionForScaleBar->SetSelectionIsOptional(true);
    m_Controls.ReferenceSelectionForScaleBar->SetEmptyInfo(QString("Reference image selection"));
    m_Controls.ReferenceSelectionForScaleBar->SetPopUpTitel(QString("Image"));
  }

  // disable reference point set
  m_Controls.refPointSetGroup->setVisible(false);

  connect(m_Controls.btnEqualizeLW, &QAbstractButton::clicked, this, &m2DataToolsView::OnEqualizeLW);
  connect(m_Controls.resetTiling, &QAbstractButton::clicked, this, &m2DataToolsView::OnResetTiling);
  connect(m_Controls.applyTiling, &QAbstractButton::clicked, this, &m2DataToolsView::OnApplyTiling);
  connect(m_Controls.alignImages, &QAbstractButton::clicked, this, &m2DataToolsView::OnAlignImages);
  connect(m_Controls.resetAlignImages, &QAbstractButton::clicked, this, &m2DataToolsView::OnResetAlignment);
  m_Controls.ReferenceSelectionForScaleBar->setEnabled(true);

  connect(m_Controls.ScaleBar,
          &QGroupBox::toggled,
          this,
          [this](bool state)
          {
            if (state && !m_Controls.ReferenceSelectionForScaleBar->GetSelectedNode())
            {
              m_Controls.ScaleBar->setChecked(false);
              return;
            }
            // m_Controls.sclaeBarLabel->setEnabled(true);
            m_Controls.ReferenceSelectionForScaleBar->setEnabled(true);
            m_ColorBarAnnotations[0]->SetVisibility(state);
            UpdateColorBarAndRenderWindows();
            RequestRenderWindowUpdate();
          });

  this->m_ColorBarAnnotations.clear();
  for (int i = 0; i < 2; ++i)
  {
    auto cbAnnotation = mitk::ColorBarAnnotation::New();
    this->m_ColorBarAnnotations.push_back(cbAnnotation);
    cbAnnotation->SetFontSize(20);
    cbAnnotation->SetOrientation(1);
    cbAnnotation->SetVisibility(0);

    m_Controls.scaleBarFontSize->setValue(cbAnnotation->GetFontSize());
    m_Controls.scaleBarOrientation->setCurrentIndex(cbAnnotation->GetOrientation());
    // m_Controls.scaleBarLenght->setValue(cbAnnotation->GetLenght());
    // m_Controls.scaleBarWidth->setValue(cbAnnotation->GetWidth());

    connect(m_Controls.scaleBarFontSize,
            &QSlider::sliderMoved,
            this,
            [this, i](int pos)
            {
              m_ColorBarAnnotations[i]->SetFontSize(pos);
              UpdateColorBarAndRenderWindows();
              RequestRenderWindowUpdate();
            });

    connect(m_Controls.scaleBarLenght,
            &QSlider::sliderMoved,
            this,
            [this, i](int pos)
            {
              Q_UNUSED(pos)
              // m_ColorBarAnnotations[i]->SetLength(pos);
              UpdateColorBarAndRenderWindows();
              RequestRenderWindowUpdate();
            });

    connect(m_Controls.scaleBarWidth,
            &QSlider::sliderMoved,
            this,
            [this, i](int pos)
            {
              Q_UNUSED(pos)
              // m_ColorBarAnnotations[i]->SetWidth(pos);
              UpdateColorBarAndRenderWindows();
              RequestRenderWindowUpdate();
            });

    connect(m_Controls.scaleBarOrientation,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this, i](int pos)
            {
              m_ColorBarAnnotations[i]->SetOrientation(pos);
              UpdateColorBarAndRenderWindows();
              RequestRenderWindowUpdate();
            });
  }
}

void m2DataToolsView::OnAlignImages()
{
  // Get all visible image nodes
  std::vector<mitk::DataNode::Pointer> nodes;
  auto allNode = GetDataStorage()->GetAll();
  
  for (auto node : *allNode)
  {
    if (dynamic_cast<mitk::Image *>(node->GetData()))
    {
      if (!node->IsVisible(nullptr))
        continue;
      nodes.push_back(node);
    }
  }

  if (nodes.empty())
    return;

  // Find the image with the greatest extent
  mitk::DataNode::Pointer referenceNode = nullptr;
  double maxExtent = 0.0;
  
  for (auto node : nodes)
  {
    if (auto image = dynamic_cast<mitk::Image *>(node->GetData()))
    {
      auto geometry = image->GetGeometry();
      auto bounds = geometry->GetBounds();
      
      // Calculate extent as the diagonal of the bounding box
      double width = bounds[1] - bounds[0];
      double height = bounds[3] - bounds[2];
      double depth = bounds[5] - bounds[4];
      double extent = std::sqrt(width * width + height * height + depth * depth);
      
      if (extent > maxExtent)
      {
        maxExtent = extent;
        referenceNode = node;
      }
    }
  }
  
  if (!referenceNode)
    return;
  
  // Move the reference image origin to coordinate system origin (0,0,0)
  mitk::Point3D newRefOrigin;
  newRefOrigin.Fill(0.0);
  MoveNodeToOrigin(GetDataStorage(), referenceNode, newRefOrigin);

  // Calculate the geometric center of the reference image (now with origin at 0,0,0)
  mitk::Point3D referenceCenter = referenceNode->GetData()->GetGeometry()->GetCenter();

  // Align all other images to the reference center
  for (auto node : nodes)
  {
    if (!node->IsVisible(nullptr))
      continue;
    
    // Skip the reference node as it's already positioned
    if (node == referenceNode)
      continue;

    auto image = dynamic_cast<mitk::Image *>(node->GetData());
    if (nullptr == image)
      continue;

    auto geometry = image->GetGeometry();
    mitk::Point3D imageCenter = geometry->GetCenter();

    // Shift the origin so that the image center ends up on the reference center
    mitk::Point3D origin = geometry->GetOrigin();
    for (unsigned int i = 0; i < 3; ++i)
      origin[i] += referenceCenter[i] - imageCenter[i];

    MoveNodeToOrigin(GetDataStorage(), node, origin);
  }
  
  mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
}

void m2DataToolsView::OnResetAlignment()
{
  ResetOriginsFromProperties(GetDataStorage());
  mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
}

void m2DataToolsView::OnResetTiling()
{
  ResetOriginsFromProperties(GetDataStorage());
  mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
}

void m2DataToolsView::OnEqualizeLW()
{
  
  auto allNodes = GetDataStorage()->GetSubset(m_Controls.ReferenceLevelWindowSelection->GetNodePredicate());
  // m2::UIUtils::AllNodes(GetDataStorage());

  if (auto node = this->m_Controls.ReferenceLevelWindowSelection->GetSelectedNode())
  {
    mitk::LevelWindow lw_ref;
    node->GetLevelWindow(lw_ref);

    for (auto &n : *allNodes)
      n->SetLevelWindow(lw_ref);
  }

  RequestRenderWindowUpdate();
}

void m2DataToolsView::OnApplyTiling()
{
  auto rows = m_Controls.mosaicRows->value();
  unsigned int maxWidth = 0, maxHeight = 0;

  // Spectrum Image Base nodes should never be child nodes!
  std::vector<mitk::DataNode::Pointer> nodes;
  auto allNode = GetDataStorage()->GetAll();
  for (auto node : *allNode)
    if (auto image = dynamic_cast<mitk::Image *>(node->GetData()))
    {
      if(!node->IsVisible(nullptr))
        continue;

      maxWidth = std::max(maxWidth, image->GetDimensions()[0]);
      maxHeight = std::max(maxHeight, image->GetDimensions()[1]);
      nodes.push_back(node);
    }

  int nodesInRow = std::ceil(nodes.size() / double(rows));
  MITK_INFO << "NodesInRow: " << nodesInRow;
  if (nodesInRow < 1)
    return;

  std::sort(nodes.begin(),
            nodes.end(),
            [](mitk::DataNode::Pointer &a, mitk::DataNode::Pointer &b) -> bool
            { return a->GetName().compare(b->GetName()) < 0; });

  int i = 0;
  for (auto node : nodes)
  {
    if(!node->IsVisible(nullptr))
      continue;

    auto image = dynamic_cast<mitk::Image *>(node->GetData());
    if (nullptr == image)
      continue;

    auto spacing = image->GetGeometry()->GetSpacing();
    mitk::Point3D origin;
    origin[0] = maxWidth * int(i % nodesInRow) * spacing[0];
    origin[1] = maxHeight * int(i / nodesInRow) * spacing[1];
    origin[2] = 0.0;

    MoveNodeToOrigin(GetDataStorage(), node, origin);

    ++i;
  }
  mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
  // this->RequestRenderWindowUpdate();
}

void m2DataToolsView::UpdateColorBarAndRenderWindows()
{
  mitk::ColorBarAnnotation::Pointer cbAnnotation;
  auto lookuptabel = mitk::LookupTableProperty::New();

  cbAnnotation = m_ColorBarAnnotations[0];

  auto renderer = GetRenderWindowPart()->GetQmitkRenderWindow("axial")->GetRenderer();
  mitk::LayoutAnnotationRenderer::AddAnnotation(cbAnnotation, renderer);

  auto node = this->m_Controls.ReferenceSelectionForScaleBar->GetSelectedNode();
  if (node)
  {
    if (node->GetProperty(lookuptabel, "LookupTable"))
      cbAnnotation->SetLookupTable(lookuptabel->GetValue()->GetVtkLookupTable());
  }
}

void m2DataToolsView::UpdateLevelWindow(const mitk::DataNode *node)
{
  if (auto msImageBase = dynamic_cast<mitk::Image *>(node->GetData()))
  {
    mitk::LevelWindow lw;
    node->GetLevelWindow(lw);
    lw.SetAuto(msImageBase);
    const_cast<mitk::DataNode *>(node)->SetLevelWindow(lw);
  }
}
