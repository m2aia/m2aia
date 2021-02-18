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

#include <M2aiaCoreExports.h>
#include <mitkImageToImageFilter.h>
#include <m2MSImageBase.h>

namespace mitk
{
  class M2AIACORE_EXPORT Image2DToImage3DSliceFilter : public mitk::ImageToImageFilter
  {
  public:
    mitkClassMacro(Image2DToImage3DSliceFilter, ImageToImageFilter);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

    void GenerateData() override;

  protected:
    Image2DToImage3DSliceFilter() = default;
    ~Image2DToImage3DSliceFilter() override = default;
  };

} // namespace mitk
