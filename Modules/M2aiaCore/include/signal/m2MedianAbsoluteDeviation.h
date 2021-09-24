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
#include <algorithm>
#include <numeric>
#include <vector>

namespace m2
{
  namespace Signal
  {
    /*!
     * Median absolute deviation (mad)
     *
     * Compute the median absolute deviation, the median of the
     * absolute deviations from the median, and (by default) adjust
     * by a factor for asymptotically normal consistency.
     */

    template <class InContainerType>
    double MedianAbsoluteDeviation(InContainerType intsCpy, const double consant = 1.4826)
    {
      std::nth_element(std::begin(intsCpy), std::next(std::begin(intsCpy), intsCpy.size() / 2), std::end(intsCpy));
      auto median = intsCpy[intsCpy.size() / 2];
      std::vector<double> absDiffOfMedian;

      std::transform(std::cbegin(intsCpy), std::cend(intsCpy), std::back_inserter(absDiffOfMedian), [median](auto &a) {
        return std::abs(a - median);
      });
      
	  std::nth_element(std::begin(absDiffOfMedian),
                       std::next(std::begin(absDiffOfMedian), absDiffOfMedian.size() / 2),
                       std::end(absDiffOfMedian));

      return consant * absDiffOfMedian[absDiffOfMedian.size() / 2];
    };

    /*!
     * Median absolute deviation (mad)
     *
     * Compute the median absolute deviation, the median of the
     * absolute deviations from the median, and (by default) adjust
     * by a factor for asymptotically normal consistency.
     */
    template <class InContainerType>
    double mad(InContainerType intsCpy, const double consant = 1.4826)
    {
      return MedianAbsoluteDeviation(intsCpy, consant);
    }

  } // namespace Signal
} // namespace m2
