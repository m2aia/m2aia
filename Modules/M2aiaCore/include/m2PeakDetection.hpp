/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#ifndef M2_PEAKDETECTION
#define M2_PEAKDETECTION

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <m2MassValue.h>
#include <m2NoiseEstimators.hpp>
#include <numeric>
#include <tuple>
#include <vector>

namespace m2
{
  class Peaks
  {
  public:
    
    template <class FirstIt, class LastIt, class RefFirstIt, class RefLastIt, class OutFirstIt>
    static inline void findMatches(
      FirstIt iIt, LastIt iItEnd, RefFirstIt rIt, RefLastIt rItEnd, OutFirstIt oIt, double tolerance)
    {
      for (; iIt != iItEnd; ++iIt)
      {
        double diff = std::numeric_limits<double>::max();
        decltype(rIt) rItTemp = rItEnd;
        decltype(rIt) rItCp = rIt;
        for (; rItCp != rItEnd; ++rItCp)
        {
          if (std::abs(iIt->mass - rItCp->mass) <= (rItCp->mass * tolerance))
          {
            if (std::abs(iIt->intensity - rItCp->intensity) < diff)
            {
              rItTemp = rItCp;
              diff = std::abs(iIt->intensity - rItCp->intensity);
            }
          }
        }
        if (rItTemp != rItEnd)
        {
          (*oIt) = {*iIt, *rItTemp};
          ++oIt;
        }
      }
    }

    /*!
     * This function looks for similar peaks (mass) across MassPeaks objects and equalizes their mass.
     * Based on MALDIquant 'binPeaks' function.
     *
     * The algorithm is based on the following workflow:
     * Calculate differences between each neighbor.
     * Divide the mass vector at the largest gap (largest difference) and form a left and a right bin.
     * Rerun step 3 for the left and/or the right bin if they don't fulfill the following criteria:
     * All peaks in a bin are near to the mean (abs(mass-meanMass)/meanMass < tolerance).
     *
     * \param s Range start Position
     * \param e Range end Position
     * \param tolerance in parts per million (ppm)
     * \param output List of binned peaks
     */
    template <class InIterType, class OutIterType>
    static inline void binPeaks(
      InIterType s, InIterType e, OutIterType output, double tolerance, bool absoluteDistance = false)
    {
      if (s == e)
        return;

      unsigned int size = 0;
      auto meanMZ = std::accumulate(s, e, double(0), [&size](auto &a, MassValue &b) {
        ++size;
        return a + b.mass;
      });
      meanMZ /= double(size);

      bool dirty = false;
      InIterType i = s;
      while (i != e)
      {
        if (std::abs(i->mass - meanMZ) >= ((meanMZ * tolerance) * (!absoluteDistance) + (absoluteDistance * tolerance)))
        {
          dirty = true;
          break;
        }
        ++i;
      }

      if (dirty && size >= 2)
      {
        InIterType pivot;
        {
          InIterType s0 = s;
          InIterType s1 = std::next(s);
          pivot = s1;
          double max = 0; // max distance of mz neighbors is split position
          while (s1 != e)
          {
            if (s1->mass - s0->mass > max)
            {
              max = s1->mass - s0->mass;
              pivot = s1;
            }
            ++s1;
            ++s0;
          }
        }

        /*std::cout << "[";
        for (auto ss = s; ss != pivot; ++ss)
        {
          std::cout << ss->mass;
          if (ss != std::prev(pivot))
            std::cout << " ";
        }
        std::cout << "]";
        std::cout << "[";
        for (auto ss = pivot; ss != e; ++ss)
        {
          std::cout << ss->mass;
          if (ss != std::prev(e))
            std::cout << " ";
        }
        std::cout << "]\n";*/

        binPeaks(s, pivot, output, tolerance, absoluteDistance);
        binPeaks(pivot, e, output, tolerance, absoluteDistance);
      }
      else
      {
        // output a new peak
        unsigned int countPeak = 0;
        double maxIntensityPeak = 0;
        unsigned int indexPeak = 0;

        std::for_each(s, e, [&maxIntensityPeak, &countPeak, &indexPeak](const MassValue &p) {
          if (p.intensity > maxIntensityPeak)
          {
            maxIntensityPeak = p.intensity;
            indexPeak = p.massAxisIndex;
          }

          countPeak += p.count;
        });

        (*output) = MassValue(meanMZ, maxIntensityPeak, countPeak, indexPeak);
        ++output;
      }
    };

    template <typename IntsItFirst, typename IntsItLast, typename MzsItFirst, typename PeakMzIntDestItFirst>
    static inline auto localMaxima(IntsItFirst intsInFirst,
                                   IntsItLast intsInLast,
                                   MzsItFirst mzsInFirst,
                                   PeakMzIntDestItFirst peaksOutFirst,
                                   unsigned int windowSize,
                                   double threshold,
                                   bool fillWithZeros = false)
    {
      auto upper = std::next(intsInFirst, windowSize);
      auto lower = intsInFirst;

      auto mid = intsInFirst;
      const auto first = intsInFirst;
      const auto last = intsInLast;

      auto max = std::max_element(first, upper + 1);
      unsigned int index = 0;

      for (; mid != last;)
      {
        /* maximum out of window, calculate new maximum in current window */
        if (max < lower)
          max = std::max_element(lower, upper + 1);
        else if (*upper >= *max)
          max = upper;

        if (max == mid && *max > threshold) // maximum is located at mid of the sliding window
          (*peaksOutFirst) = MassValue{(double)(*mzsInFirst), (double)(*max), 1, index};
        else if (fillWithZeros)
        {
          (*peaksOutFirst) = MassValue{(double)(*mzsInFirst), 0.0, 0, index};
        }

        if (std::distance(lower, upper) == (2 * windowSize) /*Number of hops from lower to upper*/
            || (upper + 1) == last)
          ++lower;

        if ((upper + 1) != last)
          ++upper;

        ++mid;
        ++index;
        ++mzsInFirst;
        ++peaksOutFirst;
      }
    }

    //	.pseudoCluster
    //
    //	Find possible isotopic cluster in mass/mz data.
    //
    //	@param x double, mass
    //	@param size integer, cluster size, number of peaks per cluster
    //	@param distance double, distance between isotopes (mass of a neutron; see
    //	Park et al 2008); could be of length > 1 (if > 1: order will affect later
    //	removal in .monoisotopicPattern).
    //	@param tolerance double, mass tolerance
    //	@return a matrix of indices (nrow(x) == n) of potential clusters
    //	@references
    //	K. Park, J.Y. Yoon, S. Lee, E. Paek, H. Park, H.J. Jung, and S.W. Lee. 2008.
    //	Isotopic peak intensity ratio based algorithm for determination of isotopic
    //	clusters and monoisotopic masses of polypeptides from high-resolution
    //	mass spectrometric data.
    //	Analytical Chemistry, 80: 7294-7303.
    //	@noRd
    static inline std::vector<std::vector<unsigned int>> pseudoCluster(const std::vector<double> &x,
                                                                       unsigned int size = 3L,
                                                                       double distance = 1.00235,
                                                                       double tolerance = 1e-4)
    {
      // auto isCloseTo = [](const double &a, const double &b, const double &tol) { return abs(b - a) <= tol; };

      std::vector<std::vector<unsigned int>> r;
      //      auto rr = std::begin(r);

      for (unsigned int i = 0; i < x.size() - size; ++i)
      {
        bool eval = true;
        std::vector<unsigned int> indices;
        indices.push_back(i);

        for (unsigned int s = 1; s < size; ++s)
        {
          std::vector<double> c;
          double minDistance = std::numeric_limits<double>::max();
          unsigned int minElem = 0;
          for (unsigned ik = i + 1; ik < x.size(); ++ik)
          {
            double dist = std::abs(x[ik] - (x[i] + s * distance));
            if (dist < minDistance)
            {
              minDistance = dist;
              minElem = ik;
            }
            else if (dist > minDistance)
            {
              break;
            }
          }

          if (minDistance < (x[i] * tolerance))
          {
            indices.push_back(minElem);
          }
          else
          {
            eval = false;
            break;
          }
        }

        /*for (unsigned k = 0; k < indices.size() - 1; ++k)
          if ((std::abs(x[indices[k]] - x[indices[k + 1]]) > (x[i] * tolerance)))
            eval = false;
*/
        if (eval)
        {
          r.emplace_back(indices);
        }
      }

      return r;
    }

    // Map mass to poisson mean/lambda.
    //
    // @param mass double, mass from experimental peak list
    // @return double suitable to pass to `dpois`
    // @references
    // E.J. Breen, F.G. Hopwood, K.L. Williams, and M.R. Wilkins. 2000.
    // Automatic poisson peak harvesting for high throughput protein identification.
    // Electrophoresis 21 (2000): 2243-2251.
    // @noRd

    //  .P
    //
    //  Model isotopic distribution by poisson distribution.
    //
    //  @param mass double, mass from experimental peak list
    //  @param isotopes integer, which isotopes
    //  @return double, isotopic distribution
    //  @references
    //  E.J. Breen, F.G. Hopwood, K.L. Williams, and M.R. Wilkins. 2000.
    //  Automatic poisson peak harvesting for high throughput protein identification.
    //  Electrophoresis 21 (2000): 2243-2251.
    //  @noRd
    static inline std::vector<double> P(const std::vector<double> &xp, const std::vector<unsigned int> &isotopes)
    {
      std::function<unsigned int(unsigned int)> factorial;
      factorial = [&factorial](unsigned int n) { return n < 2 ? 1 : n * factorial(n - 1); };

      std::vector<double> lambdas;
      std::transform(
        std::begin(xp), std::end(xp), std::back_inserter(lambdas), [](double x) { return 0.000594 * x + 0.03091; });

      std::vector<double> p;

      for (auto itL = std::begin(lambdas); itL != std::end(lambdas);)
        for (auto itI = std::begin(isotopes); itI != std::end(isotopes) && itL != std::end(lambdas); ++itI, ++itL)
          p.push_back((std::pow(*itL, *itI) * std::exp(-*itL)) / factorial(*itI));

      return p;
    }

    // .Psum
    //
    // Model isotopic distribution by poisson distribution and sum to 1 (similar to
    // TIC).
    //
    // @param mass double, mass from experimental peak list
    // @param isotopes integer, which isotopes
    // @return double, isotopic distribution
    // @references
    // E.J. Breen, F.G. Hopwood, K.L. Williams, and M.R. Wilkins. 2000.
    // Automatic poisson peak harvesting for high throughput protein identification.
    // Electrophoresis 21 (2000): 2243-2251.
    // @noRd
    static inline std::vector<std::vector<double>> Psum(const std::vector<double> &x,
                                                        const std::vector<unsigned int> &isotopes)
    {
      const auto ni = isotopes.size();
      const auto nx = x.size();

      std::vector<double> xp;
      for (unsigned int i = 0; i < nx; ++i)
      {
        for (unsigned int j = 0; j < ni; ++j)
        {
          xp.push_back(x[i]);
        }
      }

      auto p = P(xp, isotopes);

      std::vector<std::vector<double>> patterns;
      for (unsigned int i = 0; i < x.size(); ++i)
      {
        std::vector<double> pattern;
        double sum = 0;
        for (unsigned int j = 0; j < ni; ++j)
        {
          sum += p[i * ni + j];
          pattern.push_back(p[i * ni + j]);
        }
        std::transform(std::begin(pattern), std::end(pattern), std::begin(pattern), [&sum](const auto &v) -> double {
          return v / sum;
        });
        patterns.emplace_back(pattern);
      }
      return patterns;
    }

    // .colCors
    //
    // Calculate the correlation for two matrices columnwise.
    //
    // @param x matrix/data.frame
    // @return double
    // @author Sebastian Gibb <mail@@sebastiangibb.de>
    // @noRd
    static inline std::vector<double> colCors(std::vector<std::vector<double>> x,
                                              std::vector<std::vector<double>> y) noexcept
    {
      if (x.size() != y.size())
        return {};

      std::vector<double> colCors;
      for (auto xIt = std::begin(x), yIt = std::begin(y); xIt != std::end(x); ++xIt, ++yIt)
      {
        double N = (double)xIt->size();
        auto meanX = std::accumulate(std::begin(*xIt), std::end(*xIt), double(0)) / N;
        auto meanY = std::accumulate(std::begin(*yIt), std::end(*yIt), double(0)) / N;
        auto meanXY = std::inner_product(std::begin(*xIt), std::end(*xIt), std::begin(*yIt), double(0)) / N;
        auto meanYY = std::inner_product(std::begin(*yIt), std::end(*yIt), std::begin(*yIt), double(0)) / N;
        auto meanXX = std::inner_product(std::begin(*xIt), std::end(*xIt), std::begin(*xIt), double(0)) / N;
        auto p = (meanXY - (meanX * meanY)) / (std::sqrt(meanXX - meanX * meanX) * sqrt(meanYY - meanY * meanY));
        colCors.push_back(p);
      }

      return colCors;
    }

    static inline auto moveVectorsWithUniqueElementsOnly(std::vector<std::vector<unsigned int>> &origin)
      -> std::vector<std::vector<unsigned int>>
    {
      std::remove_reference<decltype(origin)>::type resultPccr;
      if (origin.empty())
        return resultPccr;
      resultPccr.emplace_back(std::move(origin[0]));

      for (unsigned int i = 1; i < origin.size(); ++i)
      { // all pc vecs

        bool dirtyFlag = false;

        for (int k = (int)resultPccr.size() - 1; k > 0 && dirtyFlag == false; --k)
        { // check if any vec before has index duplicates
          if (origin[i].front() > resultPccr[k].back())
            break;

          std::vector<unsigned int> intersection;
          std::set_intersection(std::begin(origin[i]),
                                std::end(origin[i]),
                                std::begin(resultPccr[k]),
                                std::end(resultPccr[k]),
                                std::back_inserter(intersection));
          dirtyFlag |= (bool)intersection.size();
        }

        if (!dirtyFlag)
          resultPccr.emplace_back(std::move(origin[i]));
      }
      return resultPccr;
    }

    //  .monoisotopicPattern
    //
    //  Model isotopic distribution by poisson distribution.
    //
    //  @param x double, mass from experimental peak list
    //  @param y double, intensity from experimental peak list
    //  @param tolerance double, mass tolerance for .pseudoCluster
    //  @param minCor double, minimal correlation between experimental and model
    //  intensities
    //  @param distance double, distance between isotopes (mass of a neutron; see
    //  Park et al 2008); could be of length > 1; if length > 1 the order matters.
    //  The first distance elements are prefered (the last elements are possible
    //  removed because the contain duplicated indices).
    //  @param size integer, cluster size (number of peaks for a possible cluster),
    //  see .pseudoCluster
    //  @return matrix, index of monoisotopic masses in first row
    //  @references
    //  E.J. Breen, F.G. Hopwood, K.L. Williams, and M.R. Wilkins. 2000.
    //  Automatic poisson peak harvesting for high throughput protein identification.
    //  Electrophoresis 21 (2000): 2243-2251.
    //  @noRd

    static inline std::vector<std::vector<unsigned int>> monoisotopicPattern(const std::vector<MassValue> &x,
                                                                             double minCor = 0.95,
                                                                             double tolerance = 1e-4,
                                                                             double distance = 1.00235,
                                                                             unsigned int size = 3L)
    {
      std::vector<std::vector<unsigned int>> pccr;
      {
        std::vector<double> mzs;
        for (const auto &p : x)
          mzs.push_back(p.mass);

        auto pc = pseudoCluster(mzs, size, distance, tolerance);
        if (pc.empty())
          return {};
        std::vector<std::vector<double>> ypc;
        std::vector<double> xpc;
        for (const auto &p : pc)
        {
          std::vector<double> u;
          double sum = 0;
          for (const auto &i : p)
          {
            u.push_back(x[i].intensity);
            sum += x[i].intensity;
          }
          std::transform(std::begin(u), std::end(u), std::begin(u), [&sum](const auto &v) { return v / sum; });
          ypc.emplace_back(u);
          xpc.push_back(x[p[0]].mass);
        }
        std::vector<unsigned int> isotopes;
        unsigned int n = 0;
        std::generate_n(std::back_inserter(isotopes), size, [&n]() { return n++; });
        auto psum = Psum(xpc, isotopes);

        auto cr = colCors(ypc, psum);
        for (unsigned int i = 0; i < cr.size(); ++i)
        {
          if (cr[i] > minCor)
            pccr.emplace_back(std::move(pc[i]));
        }
      }
      if (pccr.empty())
        return {};
      else
        return moveVectorsWithUniqueElementsOnly(pccr);
    }

    // .monoisotopic
    //
    // Loop through multiple .monoisotopicPattern outputs and remove duplicated
    // peaks.
    //
    // @param x double, mass from experimental peak list
    // @param y double, intensity from experimental peak list
    // @param size integer vector, cluster size
    // @param \ldots further arguments passed to .monoisotopicPattern
    // @return double, index of monoisotopic masses
    // @noRd
    static inline std::vector<MassValue> monoisotopic(const std::vector<MassValue> &peaks,
                                                     std::vector<unsigned int> size = {3, 4, 5, 6, 7, 8, 9, 10},
                                                     double minCor = 0.95,
                                                     double tolerance = 1e-4,
                                                     double distance = 1.00235)
    {
      sort(size.begin(), size.end(), std::greater<int>());
      std::vector<std::vector<unsigned int>> patterns;
      for (auto s : size)
      {
        auto pattern = monoisotopicPattern(peaks, minCor, tolerance, distance, s);
        patterns.insert(std::end(patterns), std::begin(pattern), std::end(pattern));
      }

      std::sort(std::begin(patterns), std::end(patterns), [](const auto &a, const auto &b) { return a < b; });
      auto result = moveVectorsWithUniqueElementsOnly(patterns);
      std::vector<MassValue> resultPeaks;
      for (const auto &r : result)
      {
        resultPeaks.emplace_back(peaks[r[0]]);
      }
      return resultPeaks;
    }

    template <class MzsContainer, class IntsContainer>
    static inline std::vector<MassValue> PickPeaks(const MzsContainer &mzs,
                                                  const IntsContainer &ints,
                                                  double SNR = 10,
                                                  unsigned int halfWindowSize = 20,
                                                  double binningTolInPpm = 50.0,
                                                  bool pickMonoisotopic = false)
    {
      std::vector<m2::MassValue> peaks, binPeaks;

      auto noise = m2::Noise::mad(ints);

      m2::Peaks::localMaxima(
        std::begin(ints), std::end(ints), std::begin(mzs), std::back_inserter(peaks), halfWindowSize, SNR * noise);

      if (binningTolInPpm)
      {
        binPeaks.clear();
        m2::Peaks::binPeaks(std::begin(peaks), std::end(peaks), std::back_inserter(binPeaks), binningTolInPpm * 10e-6);
        peaks = std::move(binPeaks);
      }

      if (pickMonoisotopic)
      {
        peaks = m2::Peaks::monoisotopic(peaks);
      }
      return peaks;
    }

    template <class MassAxisType>
    static auto Subrange(const MassAxisType &mzs, const double &lower, const double &upper) noexcept
      -> std::pair<unsigned int, unsigned int>
    {
      unsigned long offset = 0;
      unsigned long length = 0;
      for (auto it = std::cbegin(mzs); it != std::cend(mzs); ++it)
      {
        if (*it < lower)
          ++offset;
        else if (*it < upper)
          ++length;
        else
          break;
      }
      return {offset, length};
    }

  }; // namespace Peaks
} // namespace m2

#endif // !M2_PEAKDETECTION
