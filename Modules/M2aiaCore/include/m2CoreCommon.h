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

namespace m2
{
  enum class SpectrumFormatType : unsigned
  {
    NotSet = 0,
    ContinuousProfile = 1,
    ProcessedProfile = 2,  
    ContinuousCentroid = 4,
    ProcessedCentroid = 8
  };





  enum class OverviewSpectrumType
  {
    Sum,
    Mean,
    Maximum,
    Variance,
    Median,
    PeakIndicators,    
    None
  };



  enum class NumericType
  {
    Float,
    Double
  };

  using DisplayImagePixelType = double;
  using NormImagePixelType = double;
  using MaskImagePixelType = unsigned short;
  using IndexType = unsigned;
  using WorldCoordinateType = float;
  using IndexImagePixelType = IndexType;

  enum class TransformationMethod
  {
    None,
    Transformix
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
