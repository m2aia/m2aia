/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#ifndef BiomarkerRoc_h
#define BiomarkerRoc_h

#include <algorithm>
#include <array>
#include <tuple>
#include <vector>

/// @brief performs the Roc analysis on m_mzAppliedImage after writing the data from DoRocAnalysis into it
class BiomarkerRoc
{
public:
  static const std::string VIEW_ID;
  BiomarkerRoc();
  template <typename InputIterator_1, typename InputIterator_2, typename InputIterator_3, typename OutputIterator>
  void DoRocAnalysis(InputIterator_1 mz_iter_begin,
                     InputIterator_1 mz_iter_end,
                     InputIterator_2 image_iter_begin,
                     InputIterator_2 image_iter_end,
                     InputIterator_3 mask_iter_begin,
                     OutputIterator output_iter)
  {
    // prepare data
    for (; mz_iter_begin != mz_iter_end; ++mz_iter_begin)
    {
      m_valuesLabel1.clear();
      m_valuesLabel2.clear();
      for (auto image_idx = image_iter_begin, mask_idx = mask_iter_begin; image_idx != image_iter_end; ++image_idx, ++mask_idx) 
      {
        if (*mask_idx == 1) //label 1
          m_valuesLabel1.push_back(*image_idx);
        else if (*mask_idx == 2) //label 2
          m_valuesLabel2.push_back(*image_idx);
      }
      m_valuesLabel1.shrink_to_fit();
      m_valuesLabel2.shrink_to_fit();

      CalculateTrueRates();

      // calculate AUC
      double AUC = 0;
      for (std::size_t i = 0; i < m_TrueRates.size() - 1; ++i)
      {
        double fpr, tpr, fpr_next, tpr_next;
        std::tie(fpr, tpr) = m_TrueRates[i]; 
        std::tie(fpr_next, tpr_next) = m_TrueRates[i + 1]; 
        double a = tpr_next, c = tpr, h = fpr - fpr_next;
        // trapez approx
        AUC += (a + c) / 2 * h;
      }
      *output_iter = AUC;
      ++output_iter;
    }
  }

private:
  void CalculateTrueRates()
    {
    // get thresholds
    m_thresholds[0] = std::min(
      *std::min_element(m_valuesLabel1.begin(), m_valuesLabel1.end()), 
      *std::min_element(m_valuesLabel2.begin(), m_valuesLabel2.end())
    );
    m_thresholds[m_thresholds.size() - 1] = std::max(
      *std::max_element(m_valuesLabel1.begin(), m_valuesLabel1.end()), 
      *std::max_element(m_valuesLabel2.begin(), m_valuesLabel2.end())
    );
    for (auto i = 1; i < m_thresholds.size() - 1; ++i)
    {
      m_thresholds[i] = i / m_thresholds.size() * (m_thresholds[m_thresholds.size() - 1] - m_thresholds[0]);
    }
    // get TPR, TNR
    for (auto i = 0; i < m_thresholds.size(); ++i)
    {
      double TP = 0, TN = 0, FN = 0, FP = 0;
      for (const auto& valueLabelOne : m_valuesLabel1)
      {
        if (valueLabelOne < m_thresholds[i])
          FN += 1;
        else
          TP += 1;
      }
      for (const auto& valueLabelTwo : m_valuesLabel2)
      {
        if (valueLabelTwo < m_thresholds[i])
          TN += 1;
        else
          FP += 1;
      }
      m_TrueRates[i] = std::make_tuple(( FP / (FP + TN) ), ( TP / (TP + FN) ));
    }
  }

  static constexpr const int m_render_granularity = 20;
  std::array<double, m_render_granularity> m_thresholds;
  std::array<std::tuple<double, double>, m_render_granularity> m_TrueRates; // contains TNR and TPR data respectively
  std::vector<double> m_valuesLabel1, m_valuesLabel2;
};

#endif // BiomarkerRoc_h
