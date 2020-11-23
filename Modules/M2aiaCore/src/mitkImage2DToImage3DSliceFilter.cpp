/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#include <mitkImage2DToImage3DSliceFilter.h>
#include <mitkImageAccessByItk.h>
#include <mitkImageCast.h>

void mitk::Image2DToImage3DSliceFilter::GenerateData()
{
  auto input = this->GetInput();
  mitk::Image::Pointer output = this->GetOutput();

  auto slice2Dto3DImageSlice = [](auto in, auto out) {
    if (out == nullptr)
      mitkThrow() << "Make sure that output is an Image::Pointer and already allocated";

    using PType = typename std::remove_pointer<decltype(in)>::type::PixelType;
    using Image3DType = itk::Image<PType, 3>;
    auto outItk = Image3DType::New();
    auto region = outItk->GetLargestPossibleRegion();
    auto s = region.GetSize();
    s[0] = in->GetLargestPossibleRegion().GetSize()[0];
    s[1] = in->GetLargestPossibleRegion().GetSize()[1];
    s[2] = 1;
    region.SetSize(s);
    outItk->SetRegions(region);

    auto spacing = outItk->GetSpacing();
    spacing[0] = in->GetSpacing()[0];
    spacing[1] = in->GetSpacing()[1];
    spacing[2] = 1;

    outItk->SetSpacing(spacing);

    auto origin = outItk->GetOrigin();
    origin[0] = in->GetOrigin()[0];
    origin[1] = in->GetOrigin()[1];
    origin[2] = 0;
    outItk->SetOrigin(origin);

    auto dir = outItk->GetDirection();
    dir[0][0] = in->GetDirection()[0][0];
    dir[0][1] = in->GetDirection()[0][1];
    dir[1][0] = in->GetDirection()[1][0];
    dir[1][1] = in->GetDirection()[1][1];
    outItk->SetDirection(dir);

    outItk->Allocate();
    memcpy(outItk->GetBufferPointer(), in->GetBufferPointer(), s[0] * s[1] * sizeof(PType));

    mitk::CastToMitkImage(outItk, out);
  };

  AccessByItk_1(input, slice2Dto3DImageSlice, output);
}
