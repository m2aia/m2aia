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
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>
#include <ctkDoubleSpinBox.h>
#include <ctkRangeWidget.h>
#include <m2MSImageBase.h>
#include <m2MultiSliceFilter.h>
#include <mitkColorProperty.h>
#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkLookupTable.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateProperty.h>
#include <qlabel.h>

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
      }

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
