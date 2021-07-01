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

#include <mitkLabelSetImage.h>
#include <type_traits>

namespace m2
{
  enum class SpectrumFormatType : unsigned int
  {
    None = 0,
    ContinuousProfile = 1,
    ProcessedProfile = 2,
    ContinuousCentroid = 4,
    ProcessedCentroid = 8
  };

  enum class OverviewSpectrumType : unsigned int
  {
    None = 0,
    Mean = 1,
    Median = 2,
    Maximum = 3,
    Sum = 4,
    Variance = 5,
    PeakIndicators = 6
  };

  enum class NumericType : unsigned int
  {
    Float = 0,
    Double = 1
  };

  //////////////////////////////////////////////////////////////////////
  /////////////////// ATTENTION ////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  // If you have added a new enum value to the list.
  // Ensure that all enum values are well defined in the m2::CORE_MAPPINGS.
  // m2::CORE_MAPPINGS is defined in <m2CoreCommon.h>.
  // m2::CORE_MAPPINGS is used to transform strings into unsigned int values.
  // A compiler error will be thrown if you enabled Testing and not added the
  // new enum value to the test. <m2CoreMappingsTest.cpp>
  //
  //////////////////////////////////////////////////////////////////////
  /////////////////// ATTENTION ////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  const std::map<const std::string, unsigned int> CORE_MAPPINGS{{"None", 0},
                                                                {"ContinuousProfile", 1},
                                                                {"ProcessedProfile", 2},
                                                                {"ContinuousCentroid", 4},
                                                                {"ProcessedCentroid", 8},
                                                                {"Mean", 1},
                                                                {"Median", 2},
                                                                {"Maximum", 3},
                                                                {"Sum", 4},
                                                                {"Variance", 5},
                                                                {"PeakIndicators", 6},
                                                                {"Float", 0},
                                                                {"Double", 1}};

  using DisplayImagePixelType = double;
  using NormImagePixelType = double;
  using IndexType = unsigned;
  using WorldCoordinateType = float;
  using IndexImagePixelType = IndexType;

  enum class TransformationMethod
  {
    None,
    Transformix
  };

  /// m2Utils

  const auto Find = [](const auto &str, const auto &searchString, auto defaultValue, auto & map) {
    auto p = str.find(searchString);
    decltype(defaultValue) converted;

    if (p != std::string::npos)
    {
      char begin = '"';
      char end = '"';
      auto s = p;
      s = str.find(begin, p);
      if (s == std::string::npos)
      {
        begin = ' ';
        end = ')';
        s = str.find(begin, p);
      }

      
      auto e = str.find(end, p);
      auto val = str.substr(s + 1, e - s - 1);
      std::stringstream buffer(val);
      
      
      if (std::is_same<decltype(defaultValue), bool>::value)
        buffer >> std::boolalpha >> converted;      
      else
        buffer >> converted;
      
      MITK_INFO << searchString << " " << converted;
      return converted;
    }

    std::stringstream buffer;
    if (std::is_same<decltype(defaultValue), bool>::value)
        buffer << std::boolalpha << defaultValue;      
      else
        buffer << defaultValue;

    
    map[searchString] = buffer.str();
    return defaultValue;
  };

} // namespace m2

inline m2::SpectrumFormatType operator|(m2::SpectrumFormatType lhs, m2::SpectrumFormatType rhs)
{
  return static_cast<m2::SpectrumFormatType>(static_cast<std::underlying_type<m2::SpectrumFormatType>::type>(lhs) |
                                             static_cast<std::underlying_type<m2::SpectrumFormatType>::type>(rhs));
}

inline m2::SpectrumFormatType operator|=(m2::SpectrumFormatType lhs, m2::SpectrumFormatType rhs)
{
  return lhs | rhs;
}

inline m2::SpectrumFormatType operator&(m2::SpectrumFormatType lhs, m2::SpectrumFormatType rhs)
{
  return static_cast<m2::SpectrumFormatType>(static_cast<std::underlying_type<m2::SpectrumFormatType>::type>(lhs) &
                                             static_cast<std::underlying_type<m2::SpectrumFormatType>::type>(rhs));
}

inline bool any(m2::SpectrumFormatType lhs)
{
  return static_cast<std::underlying_type<m2::SpectrumFormatType>::type>(lhs) != 0;
}
