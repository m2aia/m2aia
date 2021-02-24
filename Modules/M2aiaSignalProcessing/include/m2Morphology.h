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
#include <algorithm>
#include <vector>
#include <functional>

namespace m2
{
  namespace Signal
  {
    template <class T, class U>
    void MorphologicalOperation(std::vector<T> &y, unsigned int s, std::vector<T> &output, U cmp = U()) noexcept
    {
      std::vector<T> f, g, h;
      unsigned int n, fn, k, q, i, r, j, gi, hi;

      n = unsigned(y.size());
      q = s;
      k = 2 * q + 1;

      fn = n + 2 * q + (k - (n % k));
      f.resize(fn, 0);
      g.resize(fn, 0);
      h.resize(fn, 0);

      T *data_y = y.data();
      T *data_f = f.data();
      T *data_g = g.data();
      T *data_h = h.data();
      T *data_out = output.data();

      memcpy(data_f + q, data_y, n * sizeof(T));
      for (i = 0; i < q; ++i)
      {
        data_f[i] = data_f[q];
        data_h[i] = data_f[q];
      }
      r = q + n - 1;
      for (i = q + n; i < fn; ++i)
      {
        data_f[i] = data_f[r];
        data_g[i] = data_f[r];
      }

      for (i = q, r = i + k - 1; i < n + q; i += k, r += k)
      {
        data_g[i] = data_f[i];
        data_h[r] = data_f[r];

        for (j = 1, gi = i + 1, hi = r - 1; j < k; ++j, ++gi, --hi)
        {
          if (cmp(data_g[gi - 1], data_f[gi]))
          {
            data_g[gi] = data_f[gi];
          }
          else
          {
            data_g[gi] = data_g[gi - 1];
          }
          if (cmp(data_h[hi + 1], data_f[hi]))
          {
            data_h[hi] = data_f[hi];
          }
          else
          {
            data_h[hi] = data_h[hi + 1];
          }
        }
      }

      for (i = 0, gi = k - 1, hi = 0; i < n; ++i, ++gi, ++hi)
      {
        if (cmp(data_g[gi], data_h[hi]))
        {
          data_out[i] = data_h[hi];
        }
        else
        {
          data_out[i] = data_g[gi];
        }
      }
    }

    template <class T>
    void Dilation(std::vector<T> &y, unsigned int s, std::vector<T> &output) noexcept
    {
      m2::Signal::MorphologicalOperation<T, std::less<>>(y, s, output);
    }

    template <class T>
    void Erosion(std::vector<T> &y, unsigned int s, std::vector<T> &output) noexcept
    {
      m2::Signal::MorphologicalOperation<T, std::greater<>>(y, s, output);
    }

  }; // namespace Signal
} // namespace m2
