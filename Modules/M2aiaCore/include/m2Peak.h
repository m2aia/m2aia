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
#include <array>
#include <vector>

namespace m2
{
  class M2AIACORE_EXPORT Peak
  {
  public:
    Peak() : xValue(0), yValue(0), count(0) {}
    Peak(double m, double i, unsigned int c, unsigned int oIdx)
      : xValue(m), yValue(i), count(c), xIndex(oIdx)
    {
    }

    double xValue = 0;
    double yValue = 0;
    double yValueMax = 0;
    double yValueSum = 0;
    unsigned int count = 0;
    unsigned int xIndex = 0;
    friend bool operator<(const Peak &lhs, const Peak &rhs) { return lhs.xValue < rhs.xValue; }
    friend bool operator==(const Peak &lhs, const Peak &rhs) { return lhs.xValue == rhs.xValue; }
    // PeakType& operator=(PeakType &&lhs) = default;
  };

} // namespace m2
