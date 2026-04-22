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

#include <algorithm>
#include <cmath>
#include <numeric>
#include <utility>
#include <vector>
#include <signal/m2Smoothing.h>

namespace m2
{
  namespace Signal
  {

    /**
     * @brief Suppress atmospheric absorption bands by linear bridging.
     *
     * For each band [lo_wn, hi_wn] the spectral values inside the band are
     * replaced by a straight-line ramp connecting the boundary values just
     * outside the band.  This removes CO₂ (~2280–2400 cm⁻¹) and H₂O vapour
     * bending (~1500–1900 cm⁻¹) and stretching (~3540–3900 cm⁻¹) artefacts
     * that arise from residual atmospheric gas in the beam path.
     *
     * @tparam XVec   Random-access container of x values  (double).
     * @tparam YVec   Random-access container of y values  (float).
     * @param xs      Wavenumber axis (cm⁻¹), strictly monotone.
     * @param ys      Intensity/transmittance values (same length as xs).
     * @param bands   List of (lo_cm⁻¹, hi_cm⁻¹) ranges to suppress.
     */
    template <class XVec, class YVec>
    void SuppressAtmosphericBands(
      const XVec &xs,
      YVec &ys,
      const std::vector<std::pair<double, double>> &bands)
    {
      const auto n = static_cast<int>(xs.size());

      for (const auto &band : bands)
      {
        const double lo = band.first;
        const double hi = band.second;

        // Find the first index with xs[i] >= lo
        int iLo = static_cast<int>(
          std::lower_bound(xs.begin(), xs.end(), lo) - xs.begin());
        // Find the last index with xs[i] <= hi
        int iHi = static_cast<int>(
          std::upper_bound(xs.begin(), xs.end(), hi) - xs.begin()) - 1;

        if (iLo > iHi || iLo >= n || iHi < 0)
          continue; // band outside spectrum range – skip

        // Clamp to valid spectrum indices
        iLo = std::max(iLo, 0);
        iHi = std::min(iHi, n - 1);

        // Anchor values: one step outside the band on each side
        const int iLeft  = std::max(iLo - 1, 0);
        const int iRight = std::min(iHi + 1, n - 1);

        const double yLeft  = static_cast<double>(ys[iLeft]);
        const double yRight = static_cast<double>(ys[iRight]);
        const double xLeft  = xs[iLeft];
        const double xRight = xs[iRight];
        const double xSpan  = xRight - xLeft;

        for (int k = iLo; k <= iHi; ++k)
        {
          if (xSpan < 1e-12)
          {
            ys[k] = static_cast<typename YVec::value_type>(0.5 * (yLeft + yRight));
          }
          else
          {
            const double t = (xs[k] - xLeft) / xSpan;
            ys[k] = static_cast<typename YVec::value_type>(yLeft + t * (yRight - yLeft));
          }
        }
      }
    }

    /**
     * @brief Default atmospheric bands for mid-IR tissue spectroscopy.
     *
     * Returns the three standard ranges used to suppress residual atmospheric
     * absorption in FTIR tissue/cell spectroscopy:
     *
     *  - H₂O bending overtone  : 1500–1900 cm⁻¹
     *  - CO₂ asymm. stretch    : 2280–2400 cm⁻¹
     *  - H₂O stretching        : 3540–3900 cm⁻¹
     */
    inline std::vector<std::pair<double, double>> DefaultAtmosphericBands()
    {
      return {
        {1500.0, 1900.0}, // H₂O bending
        {2280.0, 2400.0}, // CO₂
        {3540.0, 3900.0}  // H₂O stretching
      };
    }

    /**
     * @brief Polynomial scattering baseline correction (EMSC-lite).
     *
     * Fits a polynomial of degree @p degree to the spectrum using ordinary
     * least squares and subtracts the fitted curve from @p ys.  This removes
     * the slowly-varying multiplicative/additive baseline caused by Mie
     * scattering in thick tissue sections without distorting narrow absorption
     * bands.
     *
     * The x-axis is internally normalised to [-1, 1] for numerical stability.
     * Degree 2 (quadratic) is sufficient for most FTIR tissue spectra.
     *
     * @tparam XVec  Container of x values (double).
     * @tparam YVec  Container of y values (float).
     * @param xs     Wavenumber axis (cm⁻¹).
     * @param ys     Intensity values (modified in-place).
     * @param degree Polynomial degree (1 = linear, 2 = quadratic, …).  Max 4.
     */
    template <class XVec, class YVec>
    void PolynomialScatteringCorrection(const XVec &xs, YVec &ys, int degree = 2)
    {
      const int N = static_cast<int>(xs.size());
      if (N < degree + 1)
        return;

      // Normalise x to [-1, 1]
      const double xMin = xs.front();
      const double xMax = xs.back();
      const double xMid   = 0.5 * (xMin + xMax);
      const double xScale = (xMax > xMin) ? 0.5 * (xMax - xMin) : 1.0;

      std::vector<double> t(N);
      for (int k = 0; k < N; ++k)
        t[k] = (xs[k] - xMid) / xScale;

      // Build and solve the normal equations A^T A c = A^T b
      // where A is the N × (degree+1) Vandermonde matrix in t.
      const int nd = degree + 1;

      // Pre-compute power sums:  S[r] = sum_k  t[k]^r   for r = 0 … 2*degree
      std::vector<double> S(2 * nd, 0.0);
      for (int k = 0; k < N; ++k)
      {
        double tpow = 1.0;
        for (int r = 0; r < 2 * nd; ++r, tpow *= t[k])
          S[r] += tpow;
      }

      // Right-hand side:  R[r] = sum_k  t[k]^r * y[k]
      std::vector<double> R(nd, 0.0);
      for (int k = 0; k < N; ++k)
      {
        double tpow = 1.0;
        for (int r = 0; r < nd; ++r, tpow *= t[k])
          R[r] += tpow * static_cast<double>(ys[k]);
      }

      // Augmented matrix  M[row][0..nd-1 | nd]
      std::vector<std::vector<double>> M(nd, std::vector<double>(nd + 1, 0.0));
      for (int row = 0; row < nd; ++row)
      {
        for (int col = 0; col < nd; ++col)
          M[row][col] = S[row + col];
        M[row][nd] = R[row];
      }

      // Gauss-Jordan elimination with partial pivoting
      for (int col = 0; col < nd; ++col)
      {
        // Find pivot
        int piv = col;
        for (int row = col + 1; row < nd; ++row)
          if (std::abs(M[row][col]) > std::abs(M[piv][col]))
            piv = row;
        if (piv != col)
          std::swap(M[col], M[piv]);

        const double d = M[col][col];
        if (std::abs(d) < 1e-14)
          continue;
        for (int c = col; c <= nd; ++c)
          M[col][c] /= d;

        for (int row = 0; row < nd; ++row)
        {
          if (row == col)
            continue;
          const double f = M[row][col];
          for (int c = col; c <= nd; ++c)
            M[row][c] -= f * M[col][c];
        }
      }

      // Coefficients: c[0] + c[1]*t + c[2]*t² + …
      std::vector<double> c(nd);
      for (int r = 0; r < nd; ++r)
        c[r] = M[r][nd];

      // Subtract the fitted polynomial from ys
      for (int k = 0; k < N; ++k)
      {
        double baseline = 0.0;
        double tpow = 1.0;
        for (int r = 0; r < nd; ++r, tpow *= t[k])
          baseline += c[r] * tpow;
        ys[k] = static_cast<typename YVec::value_type>(
          static_cast<double>(ys[k]) - baseline);
      }
    }

    /**
     * @brief Per-spectrum vector (L₂) normalisation.
     *
     * Divides every element of @p ys by its Euclidean (L₂) norm, making all
     * spectra unit-length vectors.  This removes global intensity offsets
     * caused by differences in sample thickness or detector sensitivity across
     * pixels without changing spectral shape.
     *
     * If the norm is below a numerical threshold the spectrum is left unchanged.
     *
     * @tparam YVec Container of intensity values (float).
     * @param ys    Intensity values (modified in-place).
     */
    template <class YVec>
    void VectorNormalize(YVec &ys)
    {
      double l2 = 0.0;
      for (const auto &v : ys)
        l2 += static_cast<double>(v) * static_cast<double>(v);
      l2 = std::sqrt(l2);

      if (l2 < 1e-12)
        return;

      for (auto &v : ys)
        v = static_cast<typename YVec::value_type>(static_cast<double>(v) / l2);
    }

    /**
     * @brief Savitzky-Golay first derivative.
     *
     * Fits a polynomial of degree @p polyOrder over a sliding window of
     * half-width @p hws and evaluates its first derivative at every point.
     * Boundary values are handled by constant-extension (clamping).
     *
     * @tparam YVec     Container of intensity values (float or double).
     * @param ys        Intensity values (modified in-place).
     * @param hws       Half-window size (default 5 → 11-point window).
     * @param polyOrder Polynomial degree for the fit (default 4).
     */
    template <class YVec>
    void FirstDerivative(YVec &ys, int hws = 5, int polyOrder = 4)
    {
      if (static_cast<int>(ys.size()) < 2 * hws + 1)
        return;
      auto kernel = m2::Signal::savitzkyGolayDerivativeKernel(hws, polyOrder, 1);
      m2::Signal::filter(ys.begin(), ys.end(), kernel.begin(), kernel.end(), true);
    }

    /**
     * @brief Savitzky-Golay second derivative.
     *
     * Fits a polynomial of degree @p polyOrder over a sliding window of
     * half-width @p hws and evaluates its second derivative at every point.
     * Boundary values are handled by constant-extension (clamping).
     *
     * @tparam YVec     Container of intensity values (float or double).
     * @param ys        Intensity values (modified in-place).
     * @param hws       Half-window size (default 5 → 11-point window).
     * @param polyOrder Polynomial degree for the fit (default 4).
     */
    template <class YVec>
    void SecondDerivative(YVec &ys, int hws = 5, int polyOrder = 4)
    {
      if (static_cast<int>(ys.size()) < 2 * hws + 1)
        return;
      auto kernel = m2::Signal::savitzkyGolayDerivativeKernel(hws, polyOrder, 2);
      m2::Signal::filter(ys.begin(), ys.end(), kernel.begin(), kernel.end(), true);
    }

  } // namespace Signal
} // namespace m2
