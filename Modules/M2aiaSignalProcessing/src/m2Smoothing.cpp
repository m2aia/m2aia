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
#include <deque>
#include <vector>
//#include <algorithm>
#include <numeric>
#include <vnl/algo/vnl_matrix_inverse.h>
#include <vnl/vnl_matrix.h>

namespace m2
{
  struct M2AIASIGNALPROCESSING_EXPORT Smoothing
  {
    static vnl_matrix<double> savitzkyGolayCoefficients(int hws, int order)
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

    static std::vector<double> savitzkyGolayKernel(int hws, int order)
    {
      auto sg_coef = m2::Smoothing::savitzkyGolayCoefficients(hws, order);
      auto row = sg_coef.get_row(hws);
      auto b = row.data_block();
      auto e = row.data_block() + row.size();
      return std::vector<double>(b, e);
    }

    template <class T>
    static void filter(std::vector<T> &y, std::vector<double> const &kernel, bool extend = true)
    {
      assert((kernel.size() % 2) == 1);
      const int hws = kernel.size() / 2;
      if (extend)
      {
        std::deque<T> yy(y.begin(), y.end());
        // extend border conditions
        for (int i = 0;;)
        {
          yy.push_front(y.front());
          yy.push_back(y.back());
          ++i;
          if (i == hws)
            break;
        }

        size_t j = 0;
        auto it = yy.begin();
        for (;;)
        {
          y[j] = std::inner_product(kernel.begin(), kernel.end(), it, T(0));
          ++j;
          if (j == y.size())
            break;
          ++it;
        }
      }
      else
      {
        std::vector<T> yy(y.begin(), y.end());

        auto it = yy.begin();
        auto end = std::next(yy.begin(), yy.size() - hws * 2);
        int j = hws;

        // inner product as convolution
        //     [a b c d e f g]  <-- normalized kernel coeffs
        // [... t u v w x y z ...] < -- array of values being convoluted
        // conv value at w is computed as the inner product: (a*t + b*u + c*v + d*w + e*x + f*y + g*z)

        for (;;)
        {
          y[j] = std::inner_product(kernel.begin(), kernel.end(), it, T(0));
          ++it;
          if (it == end)
            break;
          ++j;
        }

        //// fix left/right extrema
        for (int i = 0;;)
        {
          y[i] = y[hws];
          y[y.size() - 1 - i] = y[j];
          ++i;
          if (i == hws)
            break;
        }
      }
    }
  }; // namespace Smoothing
} // namespace m2
