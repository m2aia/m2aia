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

namespace m2
{
  enum class SmoothingType
  {
    None,
    SavitzkyGolay,
    Gaussian
  };

  enum class RangePoolingStrategyType
  {
    None,
    Mean,
    Maximum,
    Median,
    Sum
  };

  enum class NormalizationStrategyType
  {
    None,
    TIC,
    Median,
    InFile,
    Sum
  };

  enum class BaselineCorrectionType
  {
    None,
    TopHat,
    Median
  };
} // namespace m2
