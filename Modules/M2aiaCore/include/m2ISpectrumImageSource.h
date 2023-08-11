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
#include <mitkImage.h>
#include <vector>

namespace m2
{

  class M2AIACORE_EXPORT ISpectrumImageSource
  {
    public:
    virtual void GetYValues(unsigned int /*id*/, std::vector<float> &) {};
    virtual void GetYValues(unsigned int /*id*/, std::vector<double> &){};
    virtual void GetXValues(unsigned int /*id*/, std::vector<float> &) {};
    virtual void GetXValues(unsigned int /*id*/, std::vector<double> &) {};

    virtual void InitializeImageAccess() {};
    virtual void InitializeGeometry() {};
    virtual void GetImagePrivate(double /*x*/ , double  /*tol*/, const mitk::Image * /*mask*/, mitk::Image * /*target*/) {};
  };

} // namespace m2