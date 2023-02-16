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
#include <m2IntervalVector.h>
#include <numeric>

namespace m2
{
  namespace Signal
  {

    template <class PeakItFirst, class PeakItLast, class OutIterType>
    inline void binPeaks(
      PeakItFirst s, PeakItLast e, OutIterType output, double tolerance, bool absoluteDistance = false)
    {
      if (s == e)
        return;

      unsigned int size = 0;
      const auto accPeak = std::accumulate(s, e, m2::Interval());
      const auto meanX = accPeak.GetX();

      bool dirty = false;
      decltype(s) i = s;
      while (i != e)
      {
        if (std::abs(i->GetX() - meanX) >= ((meanX * tolerance) * (!absoluteDistance) + (absoluteDistance * tolerance)))
        {
          dirty = true;
          break;
        }
        ++i;
      }

      if (dirty && size >= 2)
      {
        decltype(s) pivot;
        {
          decltype(s) s0 = s;
          decltype(s) s1 = std::next(s);
          pivot = s1;
          double max = 0; // max distance of mz neighbors is split position
          while (s1 != e)
          {
            if (s1->GetX() - s0->GetX() > max)
            {
              max = s1->GetX() - s0->GetX();
              pivot = s1;
            }
            ++s1;
            ++s0;
          }
        }

        binPeaks(s, pivot, output, tolerance, absoluteDistance);
        binPeaks(pivot, e, output, tolerance, absoluteDistance);
      }
      else
      {
        // output a new peak
        m2::Interval newPeak = *s;
        ++s;

        std::for_each(s, e, [&newPeak](const m2::Interval &p) { newPeak += p; });

        (*output) = newPeak;
        ++output;
      }
    }

    template <class XIt, class YIt, class OutIterType>
    inline void binning(
      XIt xFirst, XIt xEnd, YIt yFirst, OutIterType output, unsigned int bins)
    {
      if (xFirst == xEnd)
        return;

      //bin size
      double range = *(xEnd-1) - *xFirst;
      double binSize = range/double(bins); 

      unsigned int currentBin = 0;
      unsigned int currentIndex = 0;

      auto yIt = yFirst;
      auto xIt = xFirst;
      auto oIt = output;
      while(xIt!=xEnd){
        m2::Interval p;
        
        // accumulate
        while(xIt!=xEnd && *xIt < (currentBin+1)*binSize){
          p.index(currentIndex);
          p.x(*xIt);
          p.y(*yIt);
          ++xIt;
          ++yIt;
          ++currentIndex;
        }

        if(p.x.count()){ // avoid empty peaks
          *oIt = p; // assign peak
          ++output;
        }
        ++currentBin;
      }
    }
  }
}