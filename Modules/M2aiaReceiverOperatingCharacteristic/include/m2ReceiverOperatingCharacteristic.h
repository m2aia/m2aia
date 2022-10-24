/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#ifndef m2ReceiverOperatorCharacteristic_h
#define m2ReceiverOperatorCharacteristic_h

#include <algorithm>
#include <array>
#include <iterator>
#include <tuple>
#include <vector>

namespace m2
{
#define RENDER_GRANULARITY 20

  /// @brief performs the Roc analysis on m_mzAppliedImage after writing the data from DoRocAnalysis into it
  class ReceiverOperatorCharacteristic
  {
  public:
    static const std::string VIEW_ID;
    /// @brief performs the roc analysis for the given iterator
    /// @tparam IterType type of data_iterator
    /// @param data_iter iterator to a container with mz values and corresponding label, sorted by mz values
    /// @return the area under the roc curve
    template <typename IterType>
    static double DoRocAnalysis(IterType data_iter_begin, IterType data_iter_end, size_t P, size_t N)
    {
      double R1 = 0, R2 = 0;
      size_t i = 0;
      for (auto data = data_iter_begin; data != data_iter_end; ++data, ++i)
      {
        double d;
        bool b;
        std::tie(d, b) = *data;
        if (b) /* == TUMOR */
          R1 += i + 1;
        else
          R2 += i + 1;
      }
      double U1 = R1 - (P * (P + 1)) / 2;
      double U2 = R2 - (N * (N + 1)) / 2;
      return U1 / (U1 + U2); // AUC
    }

    /// @brief Performs a slower calculation of the AUC and returns additional data, which is not needed for calculating the AUC itself but useful for diagnostics
    /// @tparam IterType type of iterator
    /// @param data_iter_begin begin of data
    /// @param data_iter_end end of data
    /// @param P number of positives (true positives + false positives)
    /// @param N number of negatives (true negatives + false negatives)
    /// @return a vector containing the false positive rates and the true positive rates as well as the AUC
    template <typename IterType>
    static std::tuple<std::vector<std::tuple<double, double>>, double> DoRocAnalysisSlow(IterType data_iter_begin,
                                                                                         IterType data_iter_end,
                                                                                         size_t P,
                                                                                         size_t N)
    {
      std::vector<double> TPR, FPR;
      std::vector<std::tuple<double, double>> Rates;
      Rates.reserve(std::distance(data_iter_begin, data_iter_end));
      TPR.reserve(std::distance(data_iter_begin, data_iter_end));
      FPR.reserve(std::distance(data_iter_begin, data_iter_end));
      double TP = P;
      double TN = 0;
      for (auto data = data_iter_begin; data != data_iter_end; ++data)
      {
        double value;
        bool label;
        std::tie(value, label) = *data;
        if (label) // positive found
          --TP;
        else
          ++TN;
        TPR.push_back(TP / P);
        FPR.push_back(1 - (TN / N));
        Rates.push_back({(1 - (TN / N)), (TP / P)});
      }
      TPR.shrink_to_fit();
      FPR.shrink_to_fit();
      Rates.shrink_to_fit();
      double auc = 0;
      for (size_t i = 0; i < FPR.size() - 1; ++i)
      {
        double a = TPR[i], c = TPR[i + 1], h = FPR[i] - FPR[i + 1];
        auc += (a + c) / 2 * h;
      }
      std::tuple<std::vector<std::tuple<double, double>>, double> returnValue = std::make_tuple(Rates, auc);
      return returnValue;
    }
  };

} // namespace m2

#endif // m2ReceiverOperatorCharacteristic
