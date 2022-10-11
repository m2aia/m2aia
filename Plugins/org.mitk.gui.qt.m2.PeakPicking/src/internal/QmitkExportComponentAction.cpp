/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#include "QmitkExportComponentAction.h"

#include "mitkRenderingManager.h"
#include <mitkImageReadAccessor.h>
#include <mitkImageWriteAccessor.h>
#include <mitkImage.h>
#include <mitkImageAccessByItk.h>

//needed for qApp
#include <qcoreapplication.h>

QmitkExportComponentAction::QmitkExportComponentAction()
{
}

QmitkExportComponentAction::~QmitkExportComponentAction()
{
}

void QmitkExportComponentAction::Run(const QList<mitk::DataNode::Pointer> &selectedNodes)
{
  for (const auto &referenceNode : selectedNodes)
  {
    if (referenceNode.IsNotNull())
    {
      mitk::Image *img = dynamic_cast<mitk::Image *>(referenceNode->GetData());
      if (img)
      {
        const auto inputPixelType = img->GetPixelType();
        const auto nComponents = inputPixelType.GetNumberOfComponents();
        const auto ioPixelType = inputPixelType.GetPixelType();
        if (nComponents == 1)
          continue;

        if (m_DataStorage.IsNull())
          continue;

        if (ioPixelType == mitk::PixelType::ItkIOPixelType::RGB)
        {
          const std::string channelNames = "RGB";
          for (unsigned int i = 0; i < channelNames.size(); ++i)
          {
            auto output = this->ExportComponentImage(img, i);
            auto newNode = mitk::DataNode::New();
            newNode->SetData(output);
            newNode->SetName(referenceNode->GetName() + "_" + channelNames.at(i));
            m_DataStorage->Add(newNode, referenceNode);
          }
        }

        else if (ioPixelType == mitk::PixelType::ItkIOPixelType::RGBA)
        {
          const std::string channelNames = "RGBA";
          for (unsigned int i = 0; i < channelNames.size(); ++i)
          {
            auto output = this->ExportComponentImage(img, i);
            auto newNode = mitk::DataNode::New();
            newNode->SetData(output);
            newNode->SetName(referenceNode->GetName() + "_" + channelNames.at(i));
            m_DataStorage->Add(newNode, referenceNode);
          }
        }

        else if (nComponents <= 6)
        {
          for (unsigned int i = 0; i < nComponents; ++i)
          {
            auto output = this->ExportComponentImage(img, i);
            auto newNode = mitk::DataNode::New();
            newNode->SetData(output);
            newNode->SetName(referenceNode->GetName() + "_" + std::to_string(i));
            m_DataStorage->Add(newNode, referenceNode);
          }

        }
        else{
             mitk::IntProperty *componentProperty =
            dynamic_cast<mitk::IntProperty *>(referenceNode->GetProperty("Image.Displayed Component"));
            const auto currentComponent = componentProperty->GetValue();
            auto output = this->ExportComponentImage(img, currentComponent);
            auto newNode = mitk::DataNode::New();
            newNode->SetData(output);
            newNode->SetName(referenceNode->GetName() + "_" + std::to_string(currentComponent));
            m_DataStorage->Add(newNode, referenceNode);
        }
      }
    }
  }
}


mitk::Image::Pointer QmitkExportComponentAction::ExportComponentImage(const mitk::Image * img, unsigned int component){
        const auto inputPixelType = img->GetPixelType();
        const auto nComponents = inputPixelType.GetNumberOfComponents();
        const auto &sVector = inputPixelType.GetSize();
        const auto sElement = sVector / nComponents;
        const auto nPixels =
          std::accumulate(img->GetDimensions(), img->GetDimensions() + img->GetDimension(), 1, std::multiplies<>());
        const auto inGeometry = img->GetSlicedGeometry();

        auto output = mitk::Image::New();
        mitk::PixelType newPixelType = mitk::MakePixelType(inputPixelType,1);
        output->Initialize(newPixelType, 3, img->GetDimensions());
        // auto newPixelType = mitk::MakePixelType(inputPixelType, 1);
        output->GetSlicedGeometry()->SetSpacing(inGeometry->GetSpacing());
        output->GetSlicedGeometry()->SetOrigin(inGeometry->GetOrigin());
        output->GetSlicedGeometry()->SetDirectionVector(inGeometry->GetDirectionVector());

        mitk::ImageReadAccessor acc(img);
        mitk::ImageWriteAccessor occ(output);
        char *oData = (char *)occ.GetData();
        for (char *iData = (char *)acc.GetData(); iData != (char *)acc.GetData() + (sVector * nPixels);
             iData += sVector)
        {
          std::copy(iData + (component * sElement), iData + ((component + 1) * sElement), oData);
          oData += sElement;
        }

        return output;
}

void QmitkExportComponentAction::SetDataStorage(mitk::DataStorage* dataStorage)
{
  m_DataStorage = dataStorage;
}

void QmitkExportComponentAction::SetFunctionality(berry::QtViewPart* /*functionality*/)
{
  //not needed
}

void QmitkExportComponentAction::SetSmoothed(bool)
{
  //not needed
}

void QmitkExportComponentAction::SetDecimated(bool)
{
  //not needed
}