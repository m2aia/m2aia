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
#include <array>
#include <vector>

namespace m2
{
  class MITKM2AIACORE_EXPORT MassValue
  {
  public:
    MassValue() : mass(0), intensity(0), count(0) {}
    MassValue(double m, double i, unsigned int c, unsigned int oIdx)
      : mass(m), intensity(i), count(c), massAxisIndex(oIdx)
    {
    }
    // PeakType(PeakType &&) = default;
    double mass = 0;
    double intensity = 0;
    double intensityMax = 0;
    double intensitySum = 0;
    unsigned int count = 0;
    unsigned int massAxisIndex = 0;
    friend bool operator<(const MassValue &lhs, const MassValue &rhs) { return lhs.mass < rhs.mass; }
    friend bool operator==(const MassValue &lhs, const MassValue &rhs) { return lhs.mass == rhs.mass; }
    // PeakType& operator=(PeakType &&lhs) = default;
  };

} // namespace m2
