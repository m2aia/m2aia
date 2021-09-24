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
#include <functional>
#include <mitkExceptionMacro.h>
#include <signal/m2Morphology.h>
#include <signal/m2Normalization.h>
#include <signal/m2RunningMedian.h>
#include <signal/m2SignalCommon.h>

namespace m2
{
  namespace Signal
  {
    template <class ItValueType>
    class BaselineFunctor
    {
    private:
      BaselineCorrectionType m_strategy;
      int m_hws;

    public:
      void Initialize(BaselineCorrectionType strategy, int hws)
      {
        m_strategy = strategy;
        m_hws = hws;
      }

      void operator()(typename std::vector<ItValueType>::iterator start,
                      typename std::vector<ItValueType>::iterator end,
                      typename std::vector<ItValueType>::iterator baseline_start)
      {
        switch (m_strategy)
        {
          case m2::BaselineCorrectionType::TopHat:
            m2::Signal::Erosion(start, end, m_hws, baseline_start);
            m2::Signal::Dilation(
              baseline_start, std::next(baseline_start, distance(start, end)), m_hws, baseline_start);
            std::transform(start, end, baseline_start, start, std::minus<>());
            break;
          case m2::BaselineCorrectionType::Median:
            m2::RunMedian::apply(start, end, m_hws, baseline_start);
            std::transform(start, end, baseline_start, start, std::minus<>());

            break;
          case m2::BaselineCorrectionType::None:
            break;
        }
      }
    };
  } // namespace Signal
} // namespace m2
