/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#pragma once
#include "QmitkAbstractDataNodeAction.h"

// mitk core
#include <type_traits>

#include <itkImageRegionIterator.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkVectorIndexSelectionCastImageFilter.h>

#include <mitkDataNode.h>
#include <mitkImage.h>
#include <itkComposeImageFilter.h>
#include <mitkImageCast.h>
#include <mitkImageAccessByItk.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkITKImageImport.h>
// qt
#include <QAction>


class QmitkDataNodeConvertToRGBImageAction : public QAction, public QmitkAbstractDataNodeAction
{
  Q_OBJECT

public:

  QmitkDataNodeConvertToRGBImageAction(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchPartSite);
  QmitkDataNodeConvertToRGBImageAction(QWidget* parent = nullptr, berry::IWorkbenchPartSite* workbenchPartSite = nullptr);
  

protected:

  void InitializeAction() override;

  using RGBPixel = itk::RGBPixel<unsigned char>;

  template <typename TPixel, unsigned int VDimensions>
static void _ConvertToRGB(const itk::VectorImage<TPixel, VDimensions>* image, double min, double max, std::vector<mitk::Image::Pointer>& result)
  {
    typedef itk::VectorImage<TPixel, VDimensions> VectorImageType;
    typedef itk::Image<TPixel, VDimensions> ImageType;
    typedef itk::Image<unsigned char, VDimensions> RGBChannelImageType;
    typedef itk::VectorIndexSelectionCastImageFilter<VectorImageType, ImageType> VectorIndexSelectorType;

    std::vector<typename RGBChannelImageType::Pointer> rgbChannelImages;
    auto numberOfLayers = image->GetVectorLength();
    for (decltype(numberOfLayers) layer = 0; layer < numberOfLayers; ++layer)
    {
      auto layerSelector = VectorIndexSelectorType::New();
      layerSelector->SetInput(image);
      layerSelector->SetIndex(layer);
      layerSelector->Update();

      typedef itk::RescaleIntensityImageFilter<ImageType, RGBChannelImageType> RescaleFilterType;
      auto rescaler = RescaleFilterType::New();
      rescaler->SetInput(layerSelector->GetOutput());
      rescaler->SetOutputMinimum(min);
      rescaler->SetOutputMaximum(max);
      rescaler->Update();
      
      rgbChannelImages.push_back(rescaler->GetOutput());
      mitk::Image::Pointer layerImage;
      mitk::CastToMitkImage(rescaler->GetOutput(), layerImage);
      result.push_back(layerImage);
    }

    typedef itk::RGBPixel<unsigned char>         RGBPixelType;
    typedef itk::Image<RGBPixelType, VDimensions>    RGBImageType;

    typedef itk::ComposeImageFilter<RGBChannelImageType, RGBImageType> ComposeFilterType;
    auto composer = ComposeFilterType::New();
    composer->SetInput1(rgbChannelImages[0]);
    composer->SetInput2(rgbChannelImages[1]);
    composer->SetInput3(rgbChannelImages[2]);
    composer->Update();

    mitk::Image::Pointer rgbImageMitk;
    mitk::CastToMitkImage(composer->GetOutput(), rgbImageMitk);
    
    result.push_back(rgbImageMitk);
  }

 static std::vector<mitk::Image::Pointer> ConvertToRGB(const mitk::Image* vecImage)
  {
    if (nullptr == vecImage)
    {
      mitkThrow() << "Invalid usage; nullptr passed to SplitVectorImage.";
    }

    if (vecImage->GetChannelDescriptor().GetPixelType().GetPixelType() != itk::IOPixelEnum::VECTOR)
    {
      mitkThrow() << "Invalid usage of SplitVectorImage; passed image is not a vector image. Present pixel type: "<< vecImage->GetChannelDescriptor().GetPixelType().GetPixelTypeAsString();
    }

    std::vector<mitk::Image::Pointer> result;
    AccessVectorPixelTypeByItk_n(vecImage, _ConvertToRGB, (0.0, 255.0, result));
    

    for (auto image : result)
    {
      image->SetTimeGeometry(vecImage->GetTimeGeometry()->Clone());
    }

    return result;
  }

  static mitk::Image::Pointer ConvertMitkVectorImageToRGB(mitk::Image::Pointer vImage)
    {

      auto results = ConvertToRGB(vImage.GetPointer());
      return results[3];
    }

};
