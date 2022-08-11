/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#pragma once
#include "ui_m2OpenSlideImageIOHelperDialog.h"
#include <QDialog>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>
#include <m2OpenSlideImageIOHelperObject.h>

class m2OpenSlideImageIOHelperDialog : public QDialog
{
  Q_OBJECT

public:
  explicit m2OpenSlideImageIOHelperDialog(QWidget *parent = nullptr) : QDialog(parent)
  {
    m_Controls.setupUi(this);
    this->setSizeGripEnabled(true);
  }

  void SetOpenSlideImageIOHelperObject(m2::OpenSlideImageIOHelperObject *helper);
  void UpdateImageInformation();

  int exec() override
  {
    UpdateImageInformation();
    return QDialog::exec();
  }

  int GetSelectedLevel() { return m_SelectedLevel; }
  double GetSliceThickness() { return m_SliceThickness; }
  std::vector<mitk::Image::Pointer> GetData();
  mitk::Image::Pointer GetPreviewData();

private:
  m2::OpenSlideImageIOHelperObject::Pointer m_Helper;
  Ui::OpenSlideImageIOHelperDialog m_Controls;
  int m_SelectedLevel = -1;
  int m_TilesX;
  int m_TilesY;
  double m_SliceThickness = 1;

  /*This function casts a itk RGBA Image, to a itk varaible lenght vector image with 3 components. */
  static itk::VectorImage<unsigned char, 3>::Pointer ConvertRGBAToVectorImage(
    itk::Image<itk::RGBAPixel<unsigned char>, 3>::Pointer rgbaImage)
  {
    const auto size = rgbaImage->GetLargestPossibleRegion().GetSize();
	itk::VectorImage<unsigned char, 3>::RegionType region;
    region.SetSize(size);
    region.SetIndex({0,0,0});

    auto vectorImage = itk::VectorImage<unsigned char, 3>::New();
    vectorImage->SetRegions(region);
    vectorImage->SetOrigin(rgbaImage->GetOrigin());
    vectorImage->SetSpacing(rgbaImage->GetSpacing());
    vectorImage->SetDirection(rgbaImage->GetDirection());
    vectorImage->SetVectorLength(4);
    vectorImage->Allocate();

    itk::ImageRegionConstIterator<itk::Image<itk::RGBAPixel<unsigned char>, 3>> rgbaIterator(
      rgbaImage, rgbaImage->GetLargestPossibleRegion());

    itk::ImageRegionIterator<itk::VectorImage<unsigned char, 3>> vectorIterator(
      vectorImage, vectorImage->GetLargestPossibleRegion());

    rgbaIterator.GoToBegin();
    vectorIterator.GoToBegin();

    itk::VariableLengthVector<unsigned char> vec;
    vec.SetSize(4);
    while (!rgbaIterator.IsAtEnd())
    {
      memcpy(const_cast<unsigned char *>(vectorIterator.Get().GetDataPointer()), rgbaIterator.Get().GetDataPointer(), 4 * sizeof(unsigned char));
    
      ++vectorIterator;
      ++rgbaIterator;
    }

    return vectorImage;
  }

  /*This function casts a itk RGBA Image, to a itk varaible lenght vector image with 3 components. */
  static itk::Image<unsigned char, 3>::Pointer ConvertRGBAToImage(
    itk::Image<itk::RGBAPixel<unsigned char>, 3>::Pointer rgbaImage, unsigned int component)
  {
  
    const auto size = rgbaImage->GetLargestPossibleRegion().GetSize();

    itk::Image<unsigned char, 3>::RegionType region;
    region.SetSize(size);
    region.SetIndex({0,0,0});

    auto image = itk::Image<unsigned char, 3>::New();
    image->SetRegions(region);
    image->SetOrigin(rgbaImage->GetOrigin());
    image->SetSpacing(rgbaImage->GetSpacing());
    image->SetDirection(rgbaImage->GetDirection());
    image->Allocate();

    itk::ImageRegionConstIterator<itk::Image<itk::RGBAPixel<unsigned char>, 3>> rgbaIterator(
      rgbaImage, rgbaImage->GetLargestPossibleRegion());

    itk::ImageRegionIterator<itk::Image<unsigned char, 3>> imageIterator(image, image->GetLargestPossibleRegion());

    rgbaIterator.GoToBegin();
    imageIterator.GoToBegin();

    while (!rgbaIterator.IsAtEnd())
    {
      imageIterator.Set(rgbaIterator.Get().GetNthComponent(component));
      ++imageIterator;
      ++rgbaIterator;
    }

    return image;
  }
};
