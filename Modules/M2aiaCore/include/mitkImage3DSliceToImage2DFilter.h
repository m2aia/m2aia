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
#include <mitkImageToImageFilter.h>
#include <m2MSImageBase.h>

namespace mitk
{
  class MITKM2AIACORE_EXPORT Image3DSliceToImage2DFilter : public mitk::ImageToImageFilter
  {
  public:
    mitkClassMacro(Image3DSliceToImage2DFilter, ImageToImageFilter);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

    void GenerateData() override;

  protected:
    Image3DSliceToImage2DFilter() = default;
    ~Image3DSliceToImage2DFilter() override = default;
  };

} // namespace mitk
