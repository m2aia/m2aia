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
#include <m2RunningMedian.h>
#include <functional>
#include <m2Morphology.h>
#include <m2Normalization.h>
#include <m2SignalCommon.h>

namespace m2
{
  namespace Signal
  {
    template <class ContainerValueType>
    using BaslineSubstractionFunction = std::function<void(std::vector<ContainerValueType> &)>;

    template <class ContainerValueType>
    BaslineSubstractionFunction<ContainerValueType> CreateSubstractBaselineConverter(BaselineCorrectionType strategy,
                                                                                     int hws)
    {
      using namespace std;
      std::vector<ContainerValueType> baseline;
      switch (strategy)
      {
        case m2::BaselineCorrectionType::TopHat:

          return [baseline, hws](std::vector<ContainerValueType> &ints) mutable {
            if (baseline.size() != ints.size())
              baseline.resize(ints.size());
            m2::Signal::Erosion(ints, hws, baseline);
            m2::Signal::Dilation(baseline, hws, baseline);
            transform(begin(ints), end(ints), begin(baseline), begin(ints), minus<>());
          };

        case m2::BaselineCorrectionType::Median:
          return [baseline, hws](std::vector<ContainerValueType> &ints) mutable {
            if (baseline.size() != ints.size())
              baseline.resize(ints.size());
            m2::RunMedian::apply(ints, hws, baseline);
            transform(begin(ints), end(ints), begin(baseline), begin(ints), minus<>());
          };

        default:
          break;
      }

      return [](std::vector<ContainerValueType> &) mutable {};
    }
  } // namespace Signal
} // namespace m2
