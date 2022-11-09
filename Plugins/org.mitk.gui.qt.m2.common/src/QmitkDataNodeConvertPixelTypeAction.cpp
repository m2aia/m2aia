/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include <QmitkDataNodeConvertPixelTypeAction.h>

#include <mitkImageAccessByItk.h>
#include <mitkImageCast.h>
#include <mitkLookupTable.h>
#include <mitkLookupTableProperty.h>

#include <itkRescaleIntensityImageFilter.h>
#include <itkImageIOBase.h>

#include <QString>
#include <QMenu>

QmitkDataNodeConvertPixelTypeAction::QmitkDataNodeConvertPixelTypeAction(QWidget *parent,
                                                       berry::IWorkbenchPartSite *workbenchPartSite,
                                                       berry::IConfigurationElement *configElement)
  : QAction(parent), QmitkAbstractDataNodeAction(workbenchPartSite, configElement)
{
  this->InitializeAction();
}

void QmitkDataNodeConvertPixelTypeAction::InitializeAction()
{
  setText(tr("Convert pixel type"));
  setToolTip(tr("Copy the image and rescale intensities to the range of the new pixel type."));

  setMenu(new QMenu);
  connect(menu(), &QMenu::aboutToShow, this, &QmitkDataNodeConvertPixelTypeAction::OnMenuAboutShow);
}

void QmitkDataNodeConvertPixelTypeAction::OnMenuAboutShow()
{
  std::vector<itk::IOComponentEnum> types{itk::IOComponentEnum::CHAR,
                                          itk::IOComponentEnum::UCHAR,
                                          itk::IOComponentEnum::SHORT,
                                          itk::IOComponentEnum::USHORT,
                                          itk::IOComponentEnum::INT,
                                          itk::IOComponentEnum::UINT};

  menu()->clear();
  QAction *action;
  for (const auto &type : types)
  {
    QString name = QString::fromStdString(itk::ImageIOBase::GetComponentTypeAsString(type));
    name.replace("_", " ");

    action = menu()->addAction(name);
    action->setCheckable(true);
    connect(action, &QAction::triggered, [type, this]() {
      auto selectedNodes = this->GetSelectedNodes();
      for (auto referenceNode : selectedNodes)
      {
        if (referenceNode.IsNotNull())
        {
          if (mitk::Image::Pointer image = dynamic_cast<mitk::Image *>(referenceNode->GetData()))
          {
            auto newImage = OnApplyCastImage(image, type);
            auto dataNodeNew = mitk::DataNode::New();
            auto lut = mitk::LookupTable::New();
            lut->SetType(mitk::LookupTable::LookupTableType::GRAYSCALE_TRANSPARENT);
            dataNodeNew->SetProperty("LookupTable", mitk::LookupTableProperty::New(lut));
            dataNodeNew->SetData(newImage);
            dataNodeNew->SetVisibility(false);
            dataNodeNew->SetName(referenceNode->GetName() + "_" + itk::ImageIOBase::GetComponentTypeAsString(type));
            this->m_DataStorage.Lock()->Add(dataNodeNew);
            // UpdateLevelWindow(dataNodeNew);
          }
        }
      }
    });
  }
}
namespace m2
{
    template<class T, class ST>
    mitk::Image::Pointer CastTo(ST sourceImage)
    {
      using SourceImageType = typename std::remove_pointer<decltype(sourceImage)>::type;
      using ImageType = typename std::remove_pointer<T>::type;
      using FilterType = itk::RescaleIntensityImageFilter<SourceImageType, ImageType>;
      mitk::Image::Pointer outImage;
      auto filter = FilterType::New();
      filter->SetInput(sourceImage);
      filter->SetOutputMinimum(std::numeric_limits<typename ImageType::PixelType>::min());
      filter->SetOutputMaximum(std::numeric_limits<typename ImageType::PixelType>::max());
      filter->Update();
      mitk::CastToMitkImage(filter->GetOutput(), outImage);
      return outImage;
    };
}

mitk::Image::Pointer QmitkDataNodeConvertPixelTypeAction::OnApplyCastImage(mitk::Image::Pointer image, itk::IOComponentEnum type)
{
  mitk::Image::Pointer outImage;

  AccessByItk(image, ([&](auto itkSourceImage){
    // using ImagePixelType = typename std::remove_pointer<decltype(itkImage)>::type::PixelType;
    constexpr auto ImageDimension = std::remove_pointer<decltype(itkSourceImage)>::type::ImageDimension;
    MITK_INFO << "CONVERT " << static_cast<unsigned int>(type);
    switch (type)
    {
      case itk::IOComponentEnum::CHAR:
      {
        using TargetImageType = itk::Image<char, ImageDimension>;
        outImage = m2::CastTo<TargetImageType>(itkSourceImage);
      }
      break;
      case itk::IOComponentEnum::UCHAR:
      {
        using TargetImageType = itk::Image<unsigned char, ImageDimension>;
        outImage = m2::CastTo<TargetImageType>(itkSourceImage);
      }
      break;
      case itk::IOComponentEnum::INT:
      {
        using TargetImageType = itk::Image<int, ImageDimension>;
        outImage = m2::CastTo<TargetImageType>(itkSourceImage);
      }
      break;
      case itk::IOComponentEnum::UINT:
      {
        using TargetImageType = itk::Image<unsigned int, ImageDimension>;
        outImage = m2::CastTo<TargetImageType>(itkSourceImage);
      }
      break;
      case itk::IOComponentEnum::SHORT:
      {
        using TargetImageType = itk::Image<short, ImageDimension>;
        outImage = m2::CastTo<TargetImageType>(itkSourceImage);
      }
      break;
      case itk::IOComponentEnum::USHORT:
      {
        using TargetImageType = itk::Image<unsigned short, ImageDimension>;
        outImage = m2::CastTo<TargetImageType>(itkSourceImage);
      }
      break;
      default:
        MITK_WARN << "Conversion type not in switch-case!";
      break;
    }
   
  }));

  return outImage;
}


void QmitkDataNodeConvertPixelTypeAction::OnActionTriggered(bool){
 
}