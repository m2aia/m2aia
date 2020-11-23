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

#include <algorithm>
#include <numeric>
#include <vector>

namespace m2
{
  namespace Noise
  {
    /*!
     * Median absolute deviation (mad)
     *
     * Compute the median absolute deviation, the median of the
     * absolute deviations from the median, and (by default) adjust
     * by a factor for asymptotically normal consistency.
     *
     * The actual value calculated is
     * constant * cMedian(abs(x - center))
     * with the default value of center being median(x)
     *
     * \param intsCpy
     * \param consant
     * \return actual calculated mad value
     */
    template <class InContainerType>
    double mad(InContainerType intsCpy, const double consant = 1.4826)
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

  } // namespace Noise
} // namespace m2
