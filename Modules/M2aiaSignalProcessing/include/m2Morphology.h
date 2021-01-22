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
#include <vector>

namespace m2
{
  template <class T>
  class M2AIASIGNALPROCESSING_EXPORT Morphology
  {
  protected:
    template <class U>
    static void apply(std::vector<T> &y, unsigned int s, std::vector<T> &output, U cmp = U()) noexcept;

  public:
    static void dilation(std::vector<T> &y, unsigned int s, std::vector<T> &output) noexcept;
    static void erosion(std::vector<T> &y, unsigned int s, std::vector<T> &output) noexcept;

  }; // namespace Morphology
} // namespace m2

template m2::Morphology<double>;
template m2::Morphology<float>;
template m2::Morphology<long>;
template m2::Morphology<__int64>;
