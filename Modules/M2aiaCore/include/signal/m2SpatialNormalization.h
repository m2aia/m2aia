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
  namespace Signal
  {

    template <typename InputIterator, typename AccumulateType, typename BinaryOperation, typename Predicate>
    const AccumulateType accumulate_if(InputIterator first,
                                       const InputIterator last,
                                       AccumulateType init,
                                       BinaryOperation &&binary_op,
                                       Predicate &&predicate)
    {
      for (; first != last; ++first)
        if (predicate(*first))
          init = binary_op(init, *first);
      return init;
    }

    template <typename InputIterator, typename OutputIterator, typename UnaryOperation, typename Predicate>
    OutputIterator transform_if(
      InputIterator first, InputIterator last, OutputIterator dest_first, UnaryOperation unary_op, Predicate predicate)
    {
      for (; first != last; ++first, ++dest_first)
      {
        if (predicate(*first))
          *dest_first = unary_op(*first);
      }
      return dest_first;
    }

    template <typename InputIterator, typename Predicate>
    auto minmax_if(InputIterator first, InputIterator last, Predicate predicate)
    {
      using ValueType = typename std::iterator_traits<InputIterator>::value_type;
      bool found_any = false;
      ValueType min_value{}, max_value{};

      for (; first != last; ++first)
      {
        if (predicate(*first))
        {
          auto value = *first;
          if (!found_any)
          {
            min_value = max_value = value;
            found_any = true;
          }
          else
          {
            if (value < min_value)
              min_value = value;
            if (value > max_value)
              max_value = value;
          }
        }
      }

      if (!found_any)
        throw std::runtime_error("No elements satisfying predicate");

      return std::make_pair(min_value, max_value);
    }

    template <typename InputIterator, typename MaskIterator>
    double Mean(InputIterator first, InputIterator last, MaskIterator first_mask)
    {
      auto maskIt = first_mask;
      double N = accumulate_if(
        first, last, 0, [](auto s, auto) -> int { return s + 1; }, [&maskIt](auto) { return *maskIt++ > 0; });

      maskIt = first_mask;
      double sum = accumulate_if(first, last, 0.0, std::plus<>(), [&maskIt](auto) { return *maskIt++ > 0; });
      auto mean = sum / N;
      return mean;
    }

    template <typename InputIterator, typename MaskIterator>
    double StdDev(InputIterator first, InputIterator last, MaskIterator first_mask, double mean)
    {
      auto maskIt = first_mask;
      double N = accumulate_if(
        first, last, 0, [](auto s, auto) -> int { return s + 1; }, [&maskIt](auto) { return *maskIt++ > 0; });

      if (N <= 0)
        MITK_ERROR << " No N Detected";
      maskIt = first_mask;
      auto sum = accumulate_if(
        first,
        last,
        0.0,
        [mean](auto s, auto val) { return s + std::pow(val - mean, 2); },
        [&maskIt](auto) { return *maskIt++ > 0; });
      auto stddev = std::sqrt(sum / N);
      return stddev;
    }

    template <typename InputIterator, typename MaskIterator, typename OutputIterator>
    void ApplyScore(InputIterator first,
                    InputIterator last,
                    MaskIterator first_mask,
                    OutputIterator dest_first,
                    double mu,
                    double sigma)
    {
      if (sigma == 0.0)
      {
        // Constant image: all valid pixels become 0 after centering; avoid division by zero.
        auto maskIt = first_mask;
        transform_if(
          first,
          last,
          dest_first,
          [mu](auto val) { return val - mu; },
          [&maskIt](auto) { return *maskIt++ > 0; });
        return;
      }
      auto maskIt = first_mask;
      transform_if(
        first,
        last,
        dest_first,
        [mu, sigma](auto val) { return (val - mu) / sigma; },
        [&maskIt](auto) { return *maskIt++ > 0; });
    }

    template <typename InputIterator, typename MaskIterator>
    double Median(InputIterator first, InputIterator last, MaskIterator first_mask)
    {
      std::vector<double> intsCpy;
      auto maskIt = first_mask;
      std::copy_if(first, last, std::back_inserter(intsCpy), [&maskIt](auto) { return *maskIt++ > 0; });
      auto n = intsCpy.size();
      auto mid = n / 2;
      std::nth_element(intsCpy.begin(), intsCpy.begin() + mid, intsCpy.end());
      auto median = intsCpy[mid];
      if (n % 2 == 0)
      {
        std::nth_element(intsCpy.begin(), intsCpy.begin() + mid - 1, intsCpy.end());
        median = (median + intsCpy[mid - 1]) / 2.0;
      }
      return median;
    }

    template <typename InputIterator, typename MaskIterator>
    double MedianAbsoluteDeviation(InputIterator first,
                                   InputIterator last,
                                   MaskIterator first_mask,
                                   const double constant = 1.4826)
    {
      std::vector<double> intsCpy;
      auto maskIt = first_mask;
      auto median = Median(first, last, maskIt);

      std::vector<double> absDiffOfMedian;
      maskIt = first_mask;
      transform_if(
        first,
        last,
        std::back_inserter(absDiffOfMedian),
        [median](auto val) { return std::abs(val - median); },
        [&maskIt](auto) { return *maskIt++ > 0; });

      auto absDiffMedian = Median(absDiffOfMedian.begin(), absDiffOfMedian.end(), absDiffOfMedian.begin());
      return constant * absDiffMedian;
    }

    template <typename InputIterator, typename MaskIterator, typename OutputIterator>
    void RobustStandardizeImage(InputIterator first,
                                InputIterator last,
                                MaskIterator first_mask,
                                OutputIterator dest_first)
    {
      auto maskIt = first_mask;
      auto median = Median(first, last, maskIt);

      maskIt = first_mask;
      auto mad = MedianAbsoluteDeviation(first, last, maskIt, median);

      maskIt = first_mask;
      ApplyScore(first, last, maskIt, dest_first, median, mad);
    }

    template <typename InputIterator, typename MaskIterator, typename OutputIterator>
    void StandardizeImage(InputIterator first, InputIterator last, MaskIterator first_mask, OutputIterator dest_first)
    {
      auto maskIt = first_mask;
      double mean = Mean(first, last, maskIt);
      maskIt = first_mask;
      double stddev = StdDev(first, last, maskIt, mean);
      maskIt = first_mask;
      ApplyScore(first, last, maskIt, dest_first, mean, stddev);
    }

    template <typename InputIterator, typename MaskIterator, typename OutputIterator>
    void ParetoScaling(InputIterator first, InputIterator last, MaskIterator first_mask, OutputIterator dest_first)
    {
      auto maskIt = first_mask;
      double mean = Mean(first, last, maskIt);
      maskIt = first_mask;
      double stddev = StdDev(first, last, maskIt, mean);
      maskIt = first_mask;
      ApplyScore(first, last, maskIt, dest_first, mean, std::sqrt(stddev));
    }

    template <typename InputIterator, typename MaskIterator, typename OutputIterator>
    void VastScaling(InputIterator first, InputIterator last, MaskIterator first_mask, OutputIterator dest_first)
    {
      auto maskIt = first_mask;
      double mean = Mean(first, last, maskIt);
      maskIt = first_mask;
      double stddev = StdDev(first, last, maskIt, mean);
      maskIt = first_mask;
      // sigma = stddev / mean; guard against mean == 0 (would produce NaN)
      double sigma = (mean != 0.0) ? (stddev / mean) : 0.0;
      ApplyScore(first, last, maskIt, dest_first, mean, sigma);
    }

    template <typename InputIterator, typename MaskIterator, typename OutputIterator>
    void MinMaxNormalizeImage(InputIterator first,
                              InputIterator last,
                              MaskIterator first_mask,
                              OutputIterator dest_first)
    {
      auto maskIt = first_mask;
      const auto minMax = minmax_if(first, last, [&maskIt](auto) { return *maskIt++ > 0; });
      const auto minVal = minMax.first;
      const auto maxVal = minMax.second;

      maskIt = first_mask;
      transform_if(
        first,
        last,
        dest_first,
        [minVal, maxVal](auto val) { return (val - minVal) / (maxVal - minVal); },
        [&maskIt](auto) { return *maskIt++ > 0; });
    }

    template <typename InputIterator, typename MaskIterator, typename OutputIterator>
    void RangeScaling(InputIterator first, InputIterator last, MaskIterator first_mask, OutputIterator dest_first)
    {
      auto maskIt = first_mask;
      const auto minMax = minmax_if(first, last, [&maskIt](auto) { return *maskIt++ > 0; });
      const auto minVal = minMax.first;
      const auto maxVal = minMax.second;

      maskIt = first_mask;
      double mean = Mean(first, last, maskIt);

      maskIt = first_mask;
      ApplyScore(first, last, maskIt, dest_first, mean, maxVal - minVal);
    }
  } // namespace Signal
} // namespace m2