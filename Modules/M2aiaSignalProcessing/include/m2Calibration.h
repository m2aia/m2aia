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

#include <M2aiaSignalProcessingExports.h>
#include <algorithm>
#include <cstring>
#include <vector>

namespace m2
{
  namespace Signal
  {
    /*!
     * TotalIonCurrent: This function approximates the area under the curve of an given spectrum.
     *
     * \param mIt0 MassAxis start position
     * \param mItEnd MassAxis end position
     * \param iIt0 Intensity array start (Source of intensities is the same size as the source of masses)
     * \return tic Value of the TotalIonCount
     */
    template <class MzItFirst, class MzItLast, class IntItFirst>
    double TotalIonCurrent(MzItFirst mIt0, MzItLast mItEnd, IntItFirst iIt0) noexcept
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

  }; // namespace Calibration
} // namespace m2
