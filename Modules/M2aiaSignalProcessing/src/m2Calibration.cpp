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

#include <m2Calibration.h>
#include <algorithm>
#include <cstring>
#include <vector>

template <class MzItFirst, class MzItLast, class IntItFirst>
static inline double m2::Calibration::TotalIonCurrent(MzItFirst mIt0, MzItLast mItEnd, IntItFirst iIt0) noexcept
{
  double TIC = 0;
  auto mIt1 = std::next(mIt0);
  auto iIt1 = std::next(iIt0);
  for (; mIt1 != mItEnd; ++mIt0, ++mIt1, ++iIt0, ++iIt1)
  {
    TIC += ((*iIt0) + (*iIt1)) * 0.5 * ((*mIt1) - (*mIt0));
  }
  return TIC;
}
