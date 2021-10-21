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
#include <algorithm>
#include <random>
#include <signal/m2PeakDetection.h>
#include <signal/m2Smoothing.h>

namespace m2
{
  namespace Signal
  {
    template <class XItFirst, class XItLast, class YItFirst, class YItLast>
    std::vector<m2::Peak> EstimateFwhm(XItFirst xsStart, XItLast, YItFirst ysStart, YItLast ysEnd)
    {
      unsigned int sampleSize = 1000;
      std::vector<m2::Peak> peaksSample;
      peaksSample.reserve(sampleSize);

      { // detect peaks
        std::vector<m2::Peak> peaks;
        auto noise = m2::Signal::mad(std::vector<typename YItFirst::value_type>(ysStart, ysEnd));
        m2::Signal::localMaxima(ysStart, ysEnd, xsStart, std::back_inserter(peaks), 50, 3 * noise);
        if(peaks.size() < sampleSize){
          std::random_device rd;
          std::mt19937 g(rd());
          std::shuffle(std::begin(peaks), std::end(peaks), g);
          std::copy(peaks.data(), peaks.data() + sampleSize, std::back_inserter(peaksSample));
          std::sort(std::begin(peaksSample), std::end(peaksSample));
        }else{
          peaksSample = std::move(peaks);
        }
      }

      // compute fwhm at every peak
      for (auto &p : peaksSample)
      {
        const auto peakHm = p.GetY() * 0.5;
        // auto peakIndex = p.xIndex;

        const auto interpolateX = [](auto x1, auto x2, auto y1, auto y2, auto t) -> double
        {
          const auto q = (t - y1) / (y2 - y1);
          return x1 * (1 - q) + x2 * q;
        };

        double left;
        double right;

        { // find left
          auto xsPeakIt = std::next(xsStart, p.GetIndex());
          auto ysPeakIt = std::next(ysStart, p.GetIndex());

          while (*ysPeakIt > peakHm)
          {
            --xsPeakIt;
            --ysPeakIt;
          }

          const auto xLo = *xsPeakIt;
          const auto xUp = *(++xsPeakIt);
          const auto yLo = *ysPeakIt;
          const auto yUp = *(++ysPeakIt);
          left = interpolateX(xLo, xUp, yLo, yUp, peakHm);
          // MITK_INFO << "left " << xLo << " [" << left << "] " << xUp << " : " << peakHm;
        }

        { // find right
          auto xsPeakIt = std::next(xsStart, p.GetIndex());
          auto ysPeakIt = std::next(ysStart, p.GetIndex());

          while (*ysPeakIt > peakHm)
          {
            ++xsPeakIt;
            ++ysPeakIt;
          }

          const auto xLo = *xsPeakIt;
          const auto xUp = *(--xsPeakIt);
          const auto yLo = *ysPeakIt;
          const auto yUp = *(--ysPeakIt);
          right = interpolateX(xLo, xUp, yLo, yUp, peakHm);
          // MITK_INFO << "right " << xLo << " [" << right << "] " << xUp << " : " << peakHm;
        }

        auto distanceToCenterLeft = p.GetX() - left;
        auto distanceToCenterRight = right - p.GetX();

        if (distanceToCenterLeft > distanceToCenterRight * 2)
        {
          p.InsertFwhm(distanceToCenterRight * 2);
        }
        else if (distanceToCenterRight > distanceToCenterLeft * 2)
        {
          p.InsertFwhm(distanceToCenterLeft * 2);
        }
        else
          p.InsertFwhm(std::abs(left - right));
        // MITK_INFO << "FWHM " << fwhmVec.back();
      }

      // m2::Signal::SmoothingFunctor<double> smoother;
      // smoother.Initialize(m2::SmoothingType::SavitzkyGolay, 2);
      // smoother.InitializeKernel();
      // smoother(std::begin(fwhmVec), std::end(fwhmVec));

      return peaksSample;

      // create the linear interpolation function
      // fwhmFun = approxfun(x = sm$x, y = sm$y, rule = 2);
    }

  } // namespace Signal
} // namespace m2