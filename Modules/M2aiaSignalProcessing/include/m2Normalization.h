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
#include <numeric>
#include <functional>
#include <vector>
#include <m2SignalCommon.h>

namespace m2
{


  namespace Signal
  {
    /*!
     * TotalIonCurrent: This function approximates the area under the curve of an given spectrum.
     *
     * \param mIt0 GetXAxis start position
     * \param mItEnd GetXAxis end position
     * \param iIt0 Intensity array start (ImzMLImageSource of intensities is the same size as the source of masses)
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

    template <class ContainerType>
    double Median(ContainerType &ints) noexcept
    {
      return m2::Signal::Median(std::begin(ints), std::end(ints));
    }

    template <class ItFirst, class ItLast>
    double Median(ItFirst first, ItLast last) noexcept
    {
      const size_t n = std::distance(first, last);
      if ((n % 2) == 0)
      {
        std::nth_element(first, first + n / 2, last);
        std::nth_element(first, first + n / 2 + 1, last);
        return 0.5 * (*(first + n / 2) + *(first + (n / 2 + 1)));
      }
      else
      {
        std::nth_element(first, first + (n / 2), last);
        return *(first + n / 2);
      }
    }

    template <class ContainerValueType1, class ContainerValueType2>
    using CalibrationFunctor = std::function<ContainerValueType1(
      std::vector<ContainerValueType1> &, std::vector<ContainerValueType2> &,const ContainerValueType1 &)>;


    template <class ContainerValueType1, class ContainerValueType2>
    inline CalibrationFunctor<ContainerValueType1, ContainerValueType2> CreateNormalizor(
      NormalizationStrategyType strategy)
    {
      switch (strategy)
      {
        case NormalizationStrategyType::InFile:
          return
            [](std::vector<ContainerValueType1> &,
               std::vector<ContainerValueType2> & ints,
               const ContainerValueType1 &constant ) -> ContainerValueType1 {
              ContainerValueType1 val = constant;
              std::transform(
                std::begin(ints), std::end(ints), std::begin(ints), [&val](const auto &a) { return a / val; });
              return val;
            };
        case NormalizationStrategyType::TIC:
          return
            [](std::vector<ContainerValueType1> & pos,
               std::vector<ContainerValueType2> & ints,
               const ContainerValueType1 &) -> ContainerValueType1 {
            ContainerValueType1 val = m2::Signal::TotalIonCurrent(std::begin(pos), std::end(pos), std::begin(ints));
              std::transform(
                std::begin(ints), std::end(ints), std::begin(ints), [&val](const auto &a) { return a / val; });
              return val;
            };
        case NormalizationStrategyType::Sum:
          return
            [](std::vector<ContainerValueType1> & ,
               std::vector<ContainerValueType2> & ints,
               const ContainerValueType1 &) -> ContainerValueType1 {
              ContainerValueType1 val = std::accumulate(std::begin(ints), std::end(ints), (long double)(0));
              std::transform(
                std::begin(ints), std::end(ints), std::begin(ints), [&val](const auto &a) { return a / val; });
              return val;
            };
        case NormalizationStrategyType::Median:
          return
            [](std::vector<ContainerValueType1> &,
               std::vector<ContainerValueType2> & ints,
               const ContainerValueType1 &) -> ContainerValueType1 {
              ContainerValueType1 val = m2::Signal::Median(std::begin(ints), std::end(ints));
              std::transform(
                std::begin(ints), std::end(ints), std::begin(ints), [&val](const auto &a) { return a / val; });
              return val;
            };
        case NormalizationStrategyType::None:
          break;
      }

      return [](std::vector<ContainerValueType1> &,
                std::vector<ContainerValueType2> &,
                const ContainerValueType1 &) -> ContainerValueType1 { return 1; };
    }

  }; // namespace Signal
} // namespace m2
