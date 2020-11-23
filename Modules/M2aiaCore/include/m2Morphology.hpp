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

/* Dilation/Erosion is based on the vHGW Algorithm
 * (needs only 3 comparison per element):
 *
 * M. van Herk. "A Fast Algorithm for Local Minimum and Maximum Filters on
 * Rectangular and Octagonal Kernels."
 * Pattern Recognition Letters 13.7 (1992): 517-521.
 *
 * J. Y. Gil and M. Werman. "Computing 2-Dimensional Min, Median and Max
 * Filters." IEEE Transactions (1996): 504-507.
 *
 * Gil and Kimmel (GK) decrease the number of comparisons to nearly 1.5:
 *
 * J. Y. Gil and R. Kimmel. "Efficient Dilation, Erosion, Opening, and Closing
 * Algorithms." Pattern Analysis and Machine Intelligence,
 * IEEE Transactions on 24.12 (2002): 1606-1617.
 *
 * The implementation of the GK algorithm is much more complex and needs pointer
 * arithmetic, "goto" or multiple for-loops (increasing number of comparision).
 * The speed improvement is marginal and does not matter for our purposes.
 */

#include <algorithm>
#include <cstring>
#include <vector>

namespace m2
{
  namespace Morphology
  {
    template <class T>
    void dilation(std::vector<T> &y, unsigned int s, std::vector<T> &output)
    {
      std::vector<T> f, g, h;
      unsigned int n, fn, k, q, i, r, j, gi, hi;

      n = unsigned(y.size());
      q = s;
      k = 2 * q + 1;

      /* add q (== halfWindowSize) values left/right
       * increase n to make n%windowSize == 0 */
      fn = n + 2 * q + (k - (n % k));
      f.resize(fn, 0);
      g.resize(fn, 0);
      h.resize(fn, 0);

      T *xy = y.data();
      T *xf = f.data();
      T *xg = g.data();
      T *xh = h.data();
      T *xo = output.data();

      memcpy(xf + q, xy, n * sizeof(T));

      /* init left extrema */
      for (i = 0; i < q; ++i)
      {
        xf[i] = xf[q];
        xh[i] = xf[q];
      }
      /* init right extrema */
      r = q + n - 1;
      for (i = q + n; i < fn; ++i)
      {
        xf[i] = xf[r];
        xg[i] = xf[r];
      }

      /* preprocessing */
      for (i = q, r = i + k - 1; i < n + q; i += k, r += k)
      {
        /* init most left/right elements */
        xg[i] = xf[i];
        xh[r] = xf[r];

        for (j = 1, gi = i + 1, hi = r - 1; j < k; ++j, ++gi, --hi)
        {
          if (xg[gi - 1] < xf[gi])
          {
            xg[gi] = xf[gi];
          }
          else
          {
            xg[gi] = xg[gi - 1];
          }
          if (xh[hi + 1] < xf[hi])
          {
            xh[hi] = xf[hi];
          }
          else
          {
            xh[hi] = xh[hi + 1];
          }
        }
      }

      /* merging */
      for (i = 0, gi = k - 1, hi = 0; i < n; ++i, ++gi, ++hi)
      {
        if (xg[gi] < xh[hi])
        {
          xo[i] = xh[hi];
        }
        else
        {
          xo[i] = xg[gi];
        }
      }
    }

    template <class T>
    void erosion(std::vector<T> &y, unsigned int s, std::vector<T> &output)
    {
      std::vector<T> f, g, h;
      unsigned int n, fn, k, q, i, r, j, gi, hi;

      n = unsigned(y.size());
      q = s;
      k = 2 * q + 1;

      /* add q (== halfWindowSize) values left/right
       * increase n to make n%windowSize == 0 */
      fn = n + 2 * q + (k - (n % k));
      f.resize(fn, 0);
      g.resize(fn, 0);
      h.resize(fn, 0);

      T *xy = y.data();
      T *xf = f.data();
      T *xg = g.data();
      T *xh = h.data();
      T *xo = output.data();

      memcpy(xf + q, xy, n * sizeof(T));

      /* init left extrema */
      for (i = 0; i < q; ++i)
      {
        xf[i] = xf[q];
        xh[i] = xf[q];
      }
      /* init right extrema */
      r = q + n - 1;
      for (i = q + n; i < fn; ++i)
      {
        xf[i] = xf[r];
        xg[i] = xf[r];
      }

      /* preprocessing */
      for (i = q, r = i + k - 1; i < n + q; i += k, r += k)
      {
        /* init most left/right elements */
        xg[i] = xf[i];
        xh[r] = xf[r];

        for (j = 1, gi = i + 1, hi = r - 1; j < k; ++j, ++gi, --hi)
        {
          if (xg[gi - 1] > xf[gi])
          {
            xg[gi] = xf[gi];
          }
          else
          {
            xg[gi] = xg[gi - 1];
          }
          if (xh[hi + 1] > xf[hi])
          {
            xh[hi] = xf[hi];
          }
          else
          {
            xh[hi] = xh[hi + 1];
          }
        }
      }

      /* merging */
      for (i = 0, gi = k - 1, hi = 0; i < n; ++i, ++gi, ++hi)
      {
        if (xg[gi] > xh[hi])
        {
          xo[i] = xh[hi];
        }
        else
        {
          xo[i] = xg[gi];
        }
      }
    }

  } // namespace Morphology
} // namespace m2
