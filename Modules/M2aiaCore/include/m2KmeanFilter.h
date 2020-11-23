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
#include <m2MassSpecVisualizationFilter.h>
#include <mitkImage.h>

namespace mitk
{
  class MITKM2AIACORE_EXPORT m2KmeanFilter : public m2::MassSpecVisualizationFilter
  {
  public:
    mitkClassMacro(m2KmeanFilter, m2::MassSpecVisualizationFilter);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);
    itkSetMacro(NumberOfCluster, unsigned int);

  protected:
    using ImageType = itk::Image<double, 3>;

    void GenerateData() override;
    void initializeItkImage(itk::Image<unsigned char, 3>::Pointer);
    void ZTransformation(itk::Image<double, 3>::Pointer);
    unsigned int m_NumberOfCluster = 2;
    std::vector<double> m_PixelValues;

    m2KmeanFilter() = default;
  };
} // namespace mitk
