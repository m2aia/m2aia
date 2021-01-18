/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#pragma once

#include <MitkM2aiaCoreExports.h>
#include <itkImageRegionIterator.h>
#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImageToImageFilter.h>
namespace m2
{
  class MITKM2AIACORE_EXPORT MassSpecVisualizationFilter : public mitk::ImageToImageFilter
  {
  public:
    mitkClassMacro(MassSpecVisualizationFilter, ImageToImageFilter);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);
    itkSetMacro(NumberOfComponents, unsigned int);

    void SetMaskImage(mitk::Image::Pointer);
    void GetValidIndices();
    std::vector<itk::Index<3>> m_ValidIndices = {};

    static mitk::Image::Pointer ConvertMitkVectorImageToRGB(mitk::Image::Pointer vImage)
    {
      mitk::Image::Pointer result;
      itk::VectorImage<unsigned char, 3>::Pointer vectorImage;
      mitk::CastToItkImage(vImage, vectorImage);
      mitk::CastToMitkImage(ConvertVectorImageToRGB(vectorImage), result);
      return result;
    }

    /*This function casts a itk vector Image with vector length of 3, to a RGB itk Image. The buffer of the
      vector image is copied to the RGB image.*/
    static itk::Image<itk::RGBPixel<unsigned char>, 3>::Pointer ConvertVectorImageToRGB(
      itk::VectorImage<unsigned char, 3>::Pointer vectorImage)
    {
      itk::Image<RGBPixel, 3>::Pointer rgbImage = itk::Image<RGBPixel, 3>::New();

      itk::Image<RGBPixel, 3>::IndexType start;
      start[0] = 0;
      start[1] = 0;
      start[2] = 0;

      itk::Image<RGBPixel, 3>::SizeType size;
      auto dimensions = vectorImage->GetLargestPossibleRegion().GetSize();
      size[0] = dimensions[0];
      size[1] = dimensions[1];
      size[2] = dimensions[2];
      itk::Image<RGBPixel, 3>::RegionType region;
      region.SetSize(size);
      region.SetIndex(start);

      rgbImage->SetRegions(region);
      rgbImage->Allocate();

      rgbImage->SetOrigin(vectorImage->GetOrigin());
      rgbImage->SetSpacing(vectorImage->GetSpacing());
      rgbImage->SetDirection(vectorImage->GetDirection());

      itk::ImageRegionIterator<itk::Image<RGBPixel, 3>> imageIterator(rgbImage, rgbImage->GetRequestedRegion());
      imageIterator.GoToBegin();
      auto pixel = RGBPixel();
      pixel.SetRed(0);
      pixel.SetBlue(0);
      pixel.SetGreen(0);

      while (!imageIterator.IsAtEnd())
      {
        imageIterator.Set(pixel);
        ++imageIterator;
      }

      memcpy(
        rgbImage->GetBufferPointer(), vectorImage->GetBufferPointer(), sizeof(RGBPixel) * size[0] * size[1] * size[2]);

      return rgbImage;
    }

  protected:
    using RGBPixel = itk::RGBPixel<unsigned char>;
    using PixelType = double;
    mitk::Image::Pointer m_MaskImage;
    unsigned int m_NumberOfComponents = 3;

    void initializeItkImage(itk::Image<RGBPixel, 3>::Pointer);
    void initializeItkVectorImage(itk::VectorImage<PixelType, 3>::Pointer);
  };
} // namespace m2
