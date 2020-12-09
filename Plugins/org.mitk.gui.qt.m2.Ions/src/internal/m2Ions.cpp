/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "m2Ions.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <qlabel.h>

#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>
#include <ctkDoubleSpinBox.h>
#include <ctkRangeWidget.h>

#include <m2MSImageBase.h>
#include <m2MultiSliceFilter.h>
#include <m2PcaImageFilter.h>
#include <m2TSNEImageFilter.h>

#include <mitkColorProperty.h>
#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkLookupTable.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateProperty.h>

const std::string m2Ions::VIEW_ID = "org.mitk.views.m2.Ions";

void m2Ions::SetFocus() {}

void m2Ions::OnProcessingNodesReceived(const QString & /*id*/, m2::CommunicationService::NodesVectorType::Pointer nodes)
{
  if (nodes->empty())
  {
    return;
  }

  UpdateImageList(nodes);
}

void m2Ions::CreateQtPartControl(QWidget *parent)
{
  m_Controls.setupUi(parent);

  m_Controls.itemsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_Controls.itemsTable->setColumnCount(4);
  m_Controls.itemsTable->setColumnWidth(0, 130);
  m_Controls.itemsTable->setColumnWidth(1, 20);
  m_Controls.itemsTable->setColumnWidth(2, 250);

  m_Controls.itemsTable->setHorizontalHeaderLabels({"m/z value", "color", "scale", "name"});
  // m_Controls.itemsTable->setSelectionMode(QAbstractItemView::NoSelection);
  // m_Controls.itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

  connect(m2::CommunicationService::Instance(),
          &m2::CommunicationService::SendProcessingNodes,
          this,
          &m2Ions::OnProcessingNodesReceived);

  connect(m_Controls.visualization, &QCheckBox::toggled, this, [](auto v) {
    if (v)
    {
      emit m2::CommunicationService::Instance()->RequestProcessingNodes(QString::fromStdString(VIEW_ID));
    }
  });

  m_Menu = new QMenu(parent);
  m_PromoteToAllNodes = new QAction(parent);
  m_PromoteToAllNodes->setText("Promote ion image references");
  m_PromoteToAllNodes->setToolTip("All valid nodes will get a copy of this listing of ion image references.");
  m_Menu->addAction(m_PromoteToAllNodes);

  m_Include = new QWidgetAction(parent);
  auto chkBx = new QCheckBox(parent);
  chkBx->setText("Use this mass for visualization");
  m_Include->setDefaultWidget(chkBx);
  m_Menu->addAction(m_Include);
}

void m2Ions::UpdateImageList(m2::CommunicationService::NodesVectorType::Pointer nodes)
{
  std::set<m2::IonImageReference *, m2::IonImageReference::Comp> tableRefs;
  for (auto n : *nodes)
  {
    auto img = dynamic_cast<m2::MSImageBase *>(n->GetData());
    auto &refs = img->GetIonImageReferenceVector();
    tableRefs.insert(std::begin(refs), std::end(refs));
  }

  auto table = m_Controls.itemsTable;
  table->clearContents();
  table->setRowCount(tableRefs.size());

  unsigned tableRowPosition = 0;
  for (auto *ref : tableRefs)
  {
    // check for which MS image the reference is available
    QString tooltip = "Related MS images:\n";
    for (auto node : *nodes)
    {
      auto msImage = dynamic_cast<m2::MSImageBase *>(node->GetData());
      auto &ionImages = m_ContainerMap[node];
      auto &ionReferences = msImage->GetIonImageReferenceVector();
      if (std::find(std::begin(ionReferences), std::end(ionReferences), ref) != std::end(ionReferences))
      {
        tooltip += QString(node->GetName().c_str()) + "\n";

        // pre grab the image if it not already exist
        // TODO: garbage collection missing
        if (!ionImages[ref])
        {
          auto geom = msImage->GetGeometry()->Clone();
          auto maskImage = msImage->GetMaskImage();
          mitk::Image::Pointer ionImage = mitk::Image::New();
          ionImage->Initialize(mitk::MakeScalarPixelType<m2::IonImagePixelType>(), *geom);
          msImage->GrabIonImage(ref->mz, ref->tol, maskImage, ionImage);
          ionImages[ref] = ionImage;
        }
      }


    }

    auto rangeWidget = new ctkRangeWidget(table);
    rangeWidget->setRange(0, 100);
    rangeWidget->setToolTip(tooltip);
    table->setCellWidget(tableRowPosition, 2, rangeWidget);
    rangeWidget->minimumSpinBox()->hide();
    rangeWidget->maximumSpinBox()->hide();

    if (ref->scale[0] < 0 || ref->scale[1] < 0)
    {
      rangeWidget->setValues(0, 100);
      std::array<float, 2> scaleOptions;
      scaleOptions[0] = rangeWidget->minimum() / 100.0;
      scaleOptions[1] = rangeWidget->maximum() / 100.0;
      ref->scale = scaleOptions;
    }
    else
    {
      rangeWidget->setValues(ref->scale[0] * 100, ref->scale[1] * 100);
    }

    connect(rangeWidget, &ctkRangeWidget::valuesChanged, this, [this, ref, nodes](int min, int max) {
      ref->scale[0] = min / 100.0;
      ref->scale[1] = max / 100.0;
      CalculateVisualization(nodes);
    });

    double rgb[3];
    if (std::accumulate(std::begin(ref->color), std::prev(std::end(ref->color)), 0.0) < 0)
    {
      auto randNumber = double(rand()) / double(RAND_MAX);
      auto lookupTable = mitk::LookupTable::New();
      lookupTable->SetType(mitk::LookupTable::LEGACY_RAINBOW_COLOR);
      lookupTable->GetVtkLookupTable()->SetRange(0, 1);
      lookupTable->GetColor(randNumber, rgb);
      std::copy(rgb, rgb + 3, std::begin(ref->color));
    }
    else
    {
      std::copy(std::begin(ref->color), std::prev(std::end(ref->color)), rgb);
    }

    /*   auto color = QColor();
       color.setRed(rgb[0] * 255);
       color.setGreen(rgb[1] * 255);
       color.setBlue(rgb[2] * 255);*/

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << ref->mz << " m/z +/- " << ref->tol << " Da";
    auto label = new QLabel(QString::fromStdString(ss.str().c_str()), table);
    label->setToolTip(tooltip);
    table->setCellWidget(tableRowPosition, 0, label);
    label->setContextMenuPolicy(Qt::CustomContextMenu);

    auto labelName = new QLabel(ref->name.c_str(), table);
    labelName->setToolTip(tooltip);
    table->setCellWidget(tableRowPosition, 3, labelName);
    labelName->setContextMenuPolicy(Qt::CustomContextMenu);

    auto labelContextMenuCallback = [=](const QPoint &pos) {
      QMenu menu;

      auto remove = new QAction(&menu);
      remove->setText("Remove");
      remove->setToolTip("Remove ion image reference in all valid nodes.");
      menu.addAction(remove);

      connect(remove, &QAction::triggered, this, [=]() {
        for (auto node : *nodes)
        {
          if (auto image = dynamic_cast<m2::MSImageBase *>(node->GetData()))
          {
            auto &refVec = image->GetIonImageReferenceVector();
            if (std::find(std::begin(refVec), std::end(refVec), ref) != std::end(refVec))
              refVec.erase(std::remove(std::begin(refVec), std::end(refVec), ref));
          }
        }

        UpdateImageList(nodes);
      });

      auto promote = new QAction(&menu);
      promote->setText("Promote list");
      promote->setToolTip("All valid nodes will get a copy of this listing of ion image references.");
      menu.addAction(promote);

      auto include = new QWidgetAction(&menu);
      auto chkBx = new QCheckBox(&menu);
      chkBx->setText("Include mass");
      chkBx->setChecked(ref->isActive);
      include->setDefaultWidget(chkBx);
      menu.addAction(include);

      connect(chkBx, &QCheckBox::toggled, this, [=](bool checked) {
        ref->isActive = checked;
        CalculateVisualization(nodes);
      });

      int minLabelMinHeight = 25;
      int subMenuMinWidth = 165;
      std::string labelBackgrndStyle = "background-color: #252525";

      QMenu pcaSubMenu;
      auto pcaAction = new QWidgetAction(&pcaSubMenu);
      pcaAction->setText("Perform pca");

      auto slider = new QSlider(Qt::Horizontal);
      slider->setMinimum(1);
      slider->setMaximum(10);
      slider->setValue(3);
      slider->setStyleSheet("padding: 10px");

      auto sliderAction = new QWidgetAction(&pcaSubMenu);
      sliderAction->setDefaultWidget(slider);

      auto componentLabel = new QLabel();
      componentLabel->setText(("Number of Components: " + std::to_string(slider->value())).c_str());
      componentLabel->setMinimumHeight(minLabelMinHeight);
      componentLabel->setStyleSheet(labelBackgrndStyle.c_str());

      auto actionLabel = new QWidgetAction(&pcaSubMenu);
      actionLabel->setDefaultWidget(componentLabel);

      connect(slider, &QSlider::valueChanged, this, [slider, componentLabel, this]() {
        componentLabel->setText(("Number of Components: " + std::to_string(slider->value())).c_str());
        m_NumberOfComponents = slider->value();
      });

      pcaSubMenu.addAction(pcaAction);
      pcaSubMenu.addAction(actionLabel);
      pcaSubMenu.addAction(sliderAction);

      std::string pcaMenuTitle = "Pca";
      pcaSubMenu.setTitle(pcaMenuTitle.c_str());
      pcaSubMenu.setMinimumWidth(subMenuMinWidth);

      menu.addMenu(&pcaSubMenu);

      QMenu tsneMenu;
      tsneMenu.setTitle("T-sne");

      auto tsneItSlider = new QSlider(Qt::Horizontal);
      tsneItSlider->setMinimum(255);
      tsneItSlider->setMaximum(2000);
      tsneItSlider->setValue(m_Iterations);
      tsneItSlider->setStyleSheet("padding: 10px");

      auto tsneItSliderAction = new QWidgetAction(&tsneMenu);
      tsneItSliderAction->setDefaultWidget(tsneItSlider);

      auto sliderLabel = new QLabel();
      std::string sliderLabelStr = "Number of iterations: ";
      sliderLabel->setText((sliderLabelStr + std::to_string(tsneItSlider->value())).c_str());
      sliderLabel->setMinimumHeight(minLabelMinHeight);
      sliderLabel->setStyleSheet(labelBackgrndStyle.c_str());

      auto sliderLabelAction = new QWidgetAction(&tsneMenu);
      sliderLabelAction->setDefaultWidget(sliderLabel);

      connect(tsneItSlider, &QSlider::valueChanged, this, [sliderLabel, sliderLabelStr, this](int value) {
        sliderLabel->setText((sliderLabelStr + std::to_string(value)).c_str());
        m_Iterations = value;
      });

      auto perplexityLabelAction = new QWidgetAction(&tsneMenu);
      std::string perplexityLabelStr = "Perplexity: ";
      auto perplexityLabel = new QLabel(perplexityLabelStr.c_str());
      perplexityLabel->setMinimumHeight(minLabelMinHeight);
      perplexityLabel->setStyleSheet(labelBackgrndStyle.c_str());

      perplexityLabelAction->setDefaultWidget(perplexityLabel);

      auto tsnePerplexityInput = new QSpinBox();
      tsnePerplexityInput->setRange(5, 50);
      tsnePerplexityInput->setStyleSheet("padding-left: 10px");

      connect(tsnePerplexityInput, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        m_Perplexity = value;
      });

      auto tsnePerplexityAction = new QWidgetAction(&tsneMenu);
      tsnePerplexityAction->setDefaultWidget(tsnePerplexityInput);

      auto tsneAction = new QWidgetAction(&tsneMenu);
      tsneAction->setText("Perform t-sne");

      connect(tsneAction, &QAction::triggered, this, [tableRefs, this]() { PerformTsne(tableRefs); });

      tsneMenu.addAction(tsneAction);
      tsneMenu.addAction(perplexityLabelAction);
      tsneMenu.addAction(tsnePerplexityAction);
      tsneMenu.addAction(sliderLabelAction);
      tsneMenu.addAction(tsneItSliderAction);
      tsneMenu.setMinimumWidth(subMenuMinWidth);

      menu.addMenu(&tsneMenu);

      connect(pcaAction, &QAction::triggered, this, [tableRefs, this]() { PerformPCA(tableRefs); });
      menu.exec(label->mapToGlobal(pos));
    };

    connect(label, &QWidget::customContextMenuRequested, this, labelContextMenuCallback);
    connect(labelName, &QWidget::customContextMenuRequested, this, labelContextMenuCallback);

    QPushButton *pButton = new QPushButton(table);
    pButton->setAutoFillBackground(true);
    auto style = QString("background-color: rgb(%1,%2,%3); border: none;")
                   .arg(int(rgb[0] * 255))
                   .arg(int(rgb[1] * 255))
                   .arg(int(rgb[2] * 255));

    pButton->setStyleSheet(style);
    pButton->setToolTip(tooltip);
    table->setCellWidget(tableRowPosition, 1, pButton);

    connect(pButton, &QPushButton::clicked, this, [this, ref, nodes, pButton]() {
      auto color = QColorDialog::getColor();
      int r, g, b;
      color.getRgb(&r, &g, &b);
      auto style = QString("background-color: rgb(%1,%2,%3); border: none;").arg(r).arg(g).arg(b);
      pButton->setStyleSheet(style);

      ref->color[0] = r / 255.0;
      ref->color[1] = g / 255.0;
      ref->color[2] = b / 255.0;

      this->CalculateVisualization(nodes);
    });

    ++tableRowPosition;
  }
  CalculateVisualization(nodes);
}

int m2Ions::SetM2FilterImages(m2::MassSpecVisualizationFilter::Pointer imageFilter,
                              std::set<m2::IonImageReference *, m2::IonImageReference::Comp> tableRefs)
{
  auto select = m_Controls.itemsTable->selectionModel();
  std::vector<m2::IonImageReference *> selectedReferences;
  auto it = tableRefs.begin();

  const auto selectedRows = select->selectedRows();

  if (selectedRows.size() < 3)
  {
    QMessageBox::warning(NULL, "Too few images", "Pick at least three images for the calculation of the pca");
    return 0;
  }

  for (const auto &selectedRow : selectedRows)
  {
    auto reference = *std::next(it, selectedRow.row());
    MITK_INFO << reference->mz << " " << selectedRow.row();
    selectedReferences.push_back(reference);
  }

  int key = 0;
  mitk::Image::Pointer maskImage;
  for (const auto &ref : selectedReferences)
  {
    for (const auto &nodeImageReferencePair : m_ContainerMap)
    {
      if (!maskImage)
      {
        maskImage = dynamic_cast<m2::MSImageBase *>(nodeImageReferencePair.first->GetData())->GetMaskImage();
      }
      for (const auto &imageReferenceImagePair : nodeImageReferencePair.second)
      {
        if (imageReferenceImagePair.first->operator==(*ref))
        {
          mitk::Image::Pointer image = nodeImageReferencePair.second.at(ref);
          imageFilter->SetInput(key, image);
          ++key;
        }
      }
    }
  }
  imageFilter->SetMaskImage(maskImage);

  return 1;
}

void m2Ions::PerformTsne(std::set<m2::IonImageReference *, m2::IonImageReference::Comp> tableRefs) {
  m2::TSNEImageFilter::Pointer tsneFilter = m2::TSNEImageFilter::New();
  tsneFilter->SetIterations(m_Iterations);
  tsneFilter->SetPerplexity(m_Perplexity);

  if (!this->SetM2FilterImages(dynamic_cast<m2::MassSpecVisualizationFilter *>(tsneFilter.GetPointer()), tableRefs))
  {
    return;
  }

  tsneFilter->Update();

  auto outputImage = tsneFilter->GetOutput();
  auto outputNode = mitk::DataNode::New();
  outputNode->SetData(outputImage);
  outputNode->SetName("T-sne");
  this->GetDataStorage()->Add(outputNode);

}

void m2Ions::PerformPCA(std::set<m2::IonImageReference *, m2::IonImageReference::Comp> tableRefs)

{
  m2::PcaImageFilter::Pointer pcaFilter = m2::PcaImageFilter::New();
  pcaFilter->SetNumberOfComponents(m_NumberOfComponents);

  if (!this->SetM2FilterImages(dynamic_cast<m2::MassSpecVisualizationFilter *>(pcaFilter.GetPointer()), tableRefs))
  {
    return;
  }

  pcaFilter->Update();

  auto outputImage = pcaFilter->GetOutput();
  auto outputNode = mitk::DataNode::New();
  outputNode->SetData(outputImage);
  outputNode->SetName("pca");
  this->GetDataStorage()->Add(outputNode);
}

void m2Ions::CalculateVisualization(m2::CommunicationService::NodesVectorType::Pointer nodes)
{
  if (m_Controls.visualization->isChecked())
  {
    for (auto node : *nodes)
    {
      if (auto msImage = dynamic_cast<m2::MSImageBase *>(node->GetData()))
      {
        auto &ionImages = m_ContainerMap[node];
        if (ionImages.size() > 0)
        {
          mitk::Image::Pointer maskImage;
          int counter = 0;

          auto multiSliceFilter = m2::MultiSliceFilter::New();
          multiSliceFilter->SetMaskImage(msImage->GetMaskImage());

          std::vector<std::array<float, 2>> scaleOptions;
          std::vector<std::array<float, 4>> colors;

          for (auto &kv : ionImages)
          {
            auto ref = kv.first;

            if (ref->isActive)
            {
              auto image = kv.second;
              multiSliceFilter->SetInput(counter, image);
              counter++;
              scaleOptions.push_back(ref->scale);
              colors.push_back(ref->color);
            }
          }

          if (counter == 0)
            return; // no images added

          multiSliceFilter->SetVisualizationOption(m2::MultiSliceFilter::MIXCOLOR);

          multiSliceFilter->SetScaleOptions(scaleOptions);
          multiSliceFilter->SetColorVector(colors);
          multiSliceFilter->Update();

          auto parentNode = node;
          auto nameProperty = mitk::StringProperty::New(parentNode->GetName() + "_MultiSlice");
          auto namePropertyPredicate = mitk::NodePredicateProperty::New("name", nameProperty);
          mitk::Image::Pointer outputImage = multiSliceFilter->GetOutput();
          auto derivations = this->GetDataStorage()->GetDerivations(node, namePropertyPredicate);

          if (!derivations->empty())
          {
            auto multiSliceNode = derivations->front();
            multiSliceNode->SetData(outputImage);
          }
          else
          {
            auto outputNode = mitk::DataNode::New();
            outputNode->SetData(outputImage);
            outputNode->SetProperty("name", nameProperty);
            this->GetDataStorage()->Add(outputNode, parentNode);
          }

          this->RequestRenderWindowUpdate();
        }
      }
    }
  }
}
