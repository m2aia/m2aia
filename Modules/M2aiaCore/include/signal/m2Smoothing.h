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
#include <deque>
#include <functional>
#include <mitkExceptionMacro.h>
#include <numeric>
#include <signal/m2SignalCommon.h>
#include <vector>
#include <vnl/algo/vnl_matrix_inverse.h>
#include <vnl/vnl_matrix.h>

namespace m2
{
  namespace Signal
  {
    inline vnl_matrix<double> savitzkyGolayCoefficients(int hws, int order)
    {
      std::vector<double> kk;
      for (int i = 0; i < order; i++)
        kk.push_back(i);

      int nm = 2 * hws + 1;
      int nk = kk.size();
      vnl_matrix<double> K(nm, nk, 0);
      for (int i = 0; i < nm; i++)
        K.set_row(i, kk.data());

      //     ## filter is applied to -m:m around current data point
      //     ## to avoid removing (NA) of left/right extrema
      //     ## lhs: 0:2*m
      //     ## rhs: (n-2m):n

      //     ## filter matrix contains 2*m+1 rows
      //     ## row 1:m == lhs coef
      //     ## row m+1 == typical sg coef
      //     ## row (n-m-1):n == rhs coef
      vnl_matrix<double> F(nm, nm, 0);

      for (int i = 0; i < (hws + 1); i++)
      {
        std::vector<double> v;
        for (int p = 0; p < nm; p++)
          v.push_back(p - i);

        vnl_matrix<double> M(nm, nk);
        for (int k = 0; k < nk; k++)
          M.set_column(k, v.data());

        vnl_matrix<double> X(nm, nk);
        for (int c = 0; c < int(X.cols()); c++)
          for (int r = 0; r < int(X.rows()); r++)
            X.put(r, c, std::pow(M.get(r, c), K.get(r, c)));

        //        auto T = vnl_svd< double >( A ).solve( b );

        auto T = vnl_matrix_inverse<double>(X.transpose() * X) * X.transpose();
        F.set_row(i, T.get_row(0));
      }
      //     ## rhs (row (n-m):n) are equal to reversed lhs
      for (int i = hws, j = hws; i < nm; i++, j--)
      {
        F.set_row(i, F.get_row(j));
        //       F[(m + 2L):nm] = rev(F[seq_len(m), ]);
      }

      return F;
    }

    inline std::vector<double> savitzkyGolayKernel(int hws, int order)
    {
      auto sg_coef = m2::Signal::savitzkyGolayCoefficients(hws, order);
      auto row = sg_coef.get_row(hws);
      auto b = row.data_block();
      auto e = row.data_block() + row.size();
      return std::vector<double>(b, e);
    }

    template <class DataIterType, class KernelIterType>
    static void filter(
      DataIterType start, DataIterType end, KernelIterType kernel_start, KernelIterType kernel_end, bool extend = true)
    {
      using T = typename DataIterType::value_type;

      const auto kernelSize = std::distance(kernel_start, kernel_end);
      const size_t dataSize = std::distance(start, end);
      assert((kernelSize % 2) == 1);
      const int hws = kernelSize / 2;

      if (extend)
      {
        std::deque<T> yy(start, end);
        // extend border conditions
        for (int i = 0;;)
        {
          yy.push_front(*start);
          yy.push_back(*(end - 1));
          ++i;
          if (i == hws)
            break;
        }

        size_t j = 0;
        auto it = yy.begin();
        for (;;)
        {
          *(start + j) = std::inner_product(kernel_start, kernel_end, it, T(0));
          ++j;
          if (j == dataSize)
            break;
          ++it;
        }
      }
      else
      {
        std::vector<T> yy(start, end);

        auto it = yy.begin();
        auto end = std::next(yy.begin(), yy.size() - hws);
        int j = hws;

        // inner product as convolution
        //     [a b c d e f g]  <-- normalized kernel coeffs
        // [... t u v w x y z ...] < -- array of values being convoluted
        // conv value at w is computed as the inner product: (a*t + b*u + c*v + d*w + e*x + f*y + g*z)

        for (;;)
        {
          *(start + j) = std::inner_product(kernel_start, kernel_end, it, T(0));
          ++it;
          if (it == end)
            break;
          ++j;
        }

        //// fix left/right extrema
        for (int i = 0;;)
        {
          *(start + i) = *(start + hws);
          *(start + dataSize - 1 - i) = *(start + j);
          ++i;
          if (i == hws)
            break;
        }
      }
    }

    template <class ItValueType>
    class SmoothingFunctor
    {
    private:
      SmoothingType m_strategy;
      int m_hws;
      std::vector<double> m_kernel;
      bool m_isKernelInitialized = false;

    public:
      void InitializeKernel()
      {
        switch (m_strategy)
        {
          case m2::SmoothingType::SavitzkyGolay:
            m_kernel = m2::Signal::savitzkyGolayKernel(m_hws, 3);
            m_isKernelInitialized = true;
            break;
          case m2::SmoothingType::Gaussian:
          {
            m_kernel.resize(m_hws * 2 + 1);
            double sigma = m_hws / 4.0;
            double sigma2 = sigma * sigma;
            for (int x = -m_hws; x < m_hws + 1; ++x)
              m_kernel[x + m_hws] = std::exp(-0.5 / sigma2 * x * x);
            auto sum = std::accumulate(std::begin(m_kernel), std::end(m_kernel), double(0));
            std::transform(
              std::begin(m_kernel), std::end(m_kernel), std::begin(m_kernel), [sum](const auto &v) { return v / sum; });
            m_isKernelInitialized = true;
          }
          case m2::SmoothingType::None:
            break;
        }
      }

      void Initialize(SmoothingType strategy, unsigned int hws)
      {
        m_strategy = strategy;
        m_hws = hws;
        InitializeKernel();
      }

      
      void operator()(typename std::vector<ItValueType>::iterator start, typename std::vector<ItValueType>::iterator end)
      {
        if (m_isKernelInitialized)
          m2::Signal::filter(start, end, std::begin(m_kernel), std::end(m_kernel), true);
      }
    };

    

  }; // namespace Signal
} // namespace m2
