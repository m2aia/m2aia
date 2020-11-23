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
#include "m2CombineImagesView.h"

// Qt
#include <QMessageBox>

// mitk image
#include <mitkImage.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>

// m2
#include <m2ImzMLMassSpecImage.h>

const std::string m2CombineImagesView::VIEW_ID = "org.mitk.views.combineimagesview";

void m2CombineImagesView::SetFocus()
{
  m_Controls.buttonPerformImageProcessing->setFocus();
}

void m2CombineImagesView::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);

  {
    auto predicate =
      mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::ImzMLMassSpecImage>::New(),
                                  mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object")));
    m_SelectA = new QmitkMultiNodeSelectionWidget();
    m_SelectA->SetDataStorage(GetDataStorage());
    m_SelectA->SetNodePredicate(predicate);
    m_SelectA->SetSelectionIsOptional(true);
    m_SelectA->SetEmptyInfo(QString("Image A"));
    m_SelectA->SetPopUpTitel(QString("Select a mass spec. image node"));
    // m_SelectA->SetAutoSelectNewNodes(true);

    ((QVBoxLayout *)(parent->layout()))->insertWidget(0, m_SelectA);
  }

  connect(m_Controls.buttonPerformImageProcessing, &QPushButton::clicked, this, &m2CombineImagesView::CombineImages);
}

static const char CombineImageAxis[3] = {'x', 'y', 'z'};
void m2CombineImagesView::CombineImages()
{
  auto nodes = m_SelectA->GetSelectedNodes();

  if (!nodes.empty() && nodes.size() >= 2)
  {
    auto index = m_Controls.cbAxis->currentIndex();

    auto imageA = dynamic_cast<m2::ImzMLMassSpecImage *>(nodes[0]->GetData());
    auto imageB = dynamic_cast<m2::ImzMLMassSpecImage *>(nodes[1]->GetData());
    auto imageC = m2::ImzMLMassSpecImage::Combine(imageA, imageB, CombineImageAxis[index]);

    for (int i = 2; i < nodes.size(); ++i)
    {
      auto image = dynamic_cast<const m2::ImzMLMassSpecImage *>(nodes[i]->GetData());
      imageC = m2::ImzMLMassSpecImage::Combine(imageC, image, CombineImageAxis[index]);
    }

    // we want to take over the mask and normalization images of the source images and prevent overriding during
    // initialization
    imageC->PreventMaskImageInitializationOn();
    imageC->PreventNormalizationImageInitializationOn();

	std::string name = "Combines";
    {
      // now copy mask and normalization image content pixel-wise (due to equalization of image dimensions)
      mitk::ImagePixelWriteAccessor<m2::MaskImagePixelType, 3> accMask(imageC->GetMaskImage());
      mitk::ImagePixelWriteAccessor<m2::NormImagePixelType, 3> accNorm(imageC->GetNormalizationImage());

      unsigned sourceIndex = 0;
      for (auto &node : nodes)
      {
		  name += "\n" + node->GetName();
        auto image = dynamic_cast<m2::ImzMLMassSpecImage *>(node->GetData());
        auto source = imageC->GetSourceList()[sourceIndex++];
        auto offset = source._offset;
        mitk::ImagePixelReadAccessor<m2::MaskImagePixelType, 3> accMaskSource(image->GetMaskImage());
        mitk::ImagePixelReadAccessor<m2::NormImagePixelType, 3> accNormSource(image->GetNormalizationImage());
        for (unsigned z = 0; z < image->GetDimensions()[2]; ++z)
          for (unsigned y = 0; y < image->GetDimensions()[1]; ++y)
            for (unsigned x = 0; x < image->GetDimensions()[0]; ++x)
            {
              accMask.SetPixelByIndex(itk::Index<3>{x, y, z} + offset, accMaskSource.GetPixelByIndex({x, y, z}));
              accNorm.SetPixelByIndex(itk::Index<3>{x, y, z} + offset, accNormSource.GetPixelByIndex({x, y, z}));
            }
      }
    }

    auto nodeC = mitk::DataNode::New();
    nodeC->SetData(imageC);
	nodeC->SetName(name);
    GetDataStorage()->Add(nodeC); // DataView node handler (NodeAdded) will initialize the image
  }
}
