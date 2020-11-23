/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#include <mitkImage3DSliceToImage2DFilter.h>
#include <mitkImageAccessByItk.h>
#include <mitkImageCast.h>

#include <itkConstantPadImageFilter.h>

void mitk::Image3DSliceToImage2DFilter::GenerateData()
{
  auto input = this->GetInput();
  mitk::Image::Pointer output = this->GetOutput();

  auto slice3Dto2DImage = [](auto in, auto out) {
    if (out == nullptr)
      mitkThrow() << "Make sure that output is an Image::Pointer and already allocated";
    using IType = typename std::remove_pointer<decltype(in)>::type;
    using PType = typename IType::PixelType;
    using Image2DType = itk::Image<PType, 2>;
    auto outItk = Image2DType::New();
    auto region = outItk->GetLargestPossibleRegion();
    auto s = region.GetSize();
    s[0] = in->GetLargestPossibleRegion().GetSize()[0];
    s[1] = in->GetLargestPossibleRegion().GetSize()[1];
    region.SetSize(s);
    outItk->SetRegions(region);

    auto spacing = outItk->GetSpacing();
    spacing[0] = in->GetSpacing()[0];
    spacing[1] = in->GetSpacing()[1];
    outItk->SetSpacing(spacing);

    auto origin = outItk->GetOrigin();
    origin[0] = in->GetOrigin()[0];
    origin[1] = in->GetOrigin()[1];
    outItk->SetOrigin(origin);

    auto dir = outItk->GetDirection();
    dir[0][0] = in->GetDirection()[0][0];
    dir[0][1] = in->GetDirection()[0][1];
    dir[1][0] = in->GetDirection()[1][0];
    dir[1][1] = in->GetDirection()[1][1];
    outItk->SetDirection(dir);

    outItk->Allocate();
    memcpy(outItk->GetBufferPointer(), in->GetBufferPointer(), s[0] * s[1] * sizeof(PType));

    typename Image2DType::SizeType lowerExtendRegion;
    lowerExtendRegion.Fill(5);

    typename Image2DType::SizeType upperExtendRegion;
    upperExtendRegion.Fill(5);
    /*{
      using FilterType = itk::ConstantPadImageFilter<Image2DType, Image2DType>;
      FilterType::Pointer filter = FilterType::New();

      filter->SetInput(outItk);
      filter->SetPadLowerBound(lowerExtendRegion);
      filter->SetPadUpperBound(upperExtendRegion);
      filter->SetConstant(0);
      filter->Update();
      mitk::CastToMitkImage(filter->GetOutput(), out);
    }*/

    mitk::CastToMitkImage(outItk, out);
  };

  AccessByItk_1(input, slice3Dto2DImage, output);
}
