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
#include <map>
#include <array>
#include <string>

namespace m2
{
  enum class SmoothingType : unsigned int
  {
    None = 0,
    SavitzkyGolay = 1,
    Gaussian = 2
  };

  const std::array<std::string, 3>  SmoothingTypeNames = {"None", "SavitzkyGolay", "Gaussian"};

  enum class RangePoolingStrategyType : unsigned int
  {
    None = 0,
    Mean = 1,
    Median = 2,
    Maximum = 3,
    Sum = 4
  };

  const std::array<std::string, 5>  RangePoolingStrategyTypeNames = {"None", "Mean", "Median", "Maximum", "Sum"};

  enum class NormalizationStrategyType : unsigned int
  {
    None = 0,
    TIC = 1,
    Median = 2,
    InFile = 3,
    Sum = 4,
    Mean = 5,
    Max = 6,
    RMS = 7
  };

  const std::array<std::string, 8> NormalizationStrategyTypeNames = {"None", "TIC", "Median", "InFile", "Sum", "Mean", "Max", "RMS"};

  enum class BaselineCorrectionType : unsigned int
  {
    None = 0,
    TopHat = 1,
    Median = 2
  };

  const std::array<std::string, 3> BaselineCorrectionTypeNames = {"None", "TopHat", "Median"};

  enum class IntensityTransformationType : unsigned int
  {
    None = 0,
    Log2 = 1,
    Log10 = 2,
    SquareRoot = 3
  };

  const std::array<std::string, 4> IntensityTransformationTypeNames = {"None", "Log2", "Log10", "SquareRoot"};

  //////////////////////////////////////////////////////////////////////
  /////////////////// ATTENTION ////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  // If you have added a new enum value to the list.
  // Ensure that all enum values are well defined in the m2::SIGNAL_MAPPINGS.
  // m2::SIGNAL_MAPPINGS is defined in <m2SignalCommon.h>.
  // m2::SIGNAL_MAPPINGS is used to transform strings into unsigned int values.
  // A compiler error will be thrown if you enabled Testing and not added the
  // new enum value to the test. <m2SignalMappingsTest.cpp>
  //
  //////////////////////////////////////////////////////////////////////
  /////////////////// ATTENTION ////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  const std::map<const std::string, unsigned int> SMOOTHING_MAPPINGS{
    {"None", 0}, {"SavitzkyGolay", 1}, {"Gaussian", 2}};

  const std::map<const std::string, unsigned int> BASECOR_MAPPINGS{{"None", 0}, {"TopHat", 1}, {"Median", 2}};

  const std::map<const std::string, unsigned int> NORMALIZATION_MAPPINGS{
    {"None", 0}, {"TIC", 1}, {"Median", 2}, {"InFile", 3}, {"Sum", 4}, {"RMS",7}};

  const std::map<const std::string, unsigned int> POOLING_MAPPINGS{
    {"None", 0}, {"Mean", 1}, {"Median", 2}, {"Maximum", 3}, {"Sum", 4}};

  const std::map<const std::string, unsigned int> INTENSITYTRANSFORMATION_MAPPINGS{
    {"None", 0}, {"Log2", 1}, {"Log10", 2}, {"SquareRoot", 3}};

} // namespace m2
