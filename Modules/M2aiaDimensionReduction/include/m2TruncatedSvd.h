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

#include <M2aiaDimensionReductionExports.h>
#include <itkeigen/Eigen/Dense>

namespace m2
{
  /** A truncated singular value decomposition A ~ U diag(S) V^T. */
  struct M2AIADIMENSIONREDUCTION_EXPORT TruncatedSvdResult
  {
    /** Rows of A by rank. */
    Eigen::MatrixXf U;
    /** The leading singular values, largest first. */
    Eigen::VectorXf S;
    /** Columns of A by rank. */
    Eigen::MatrixXf V;

    unsigned int GetRank() const { return static_cast<unsigned int>(S.size()); }
  };

  /** The leading singular vectors of a matrix.

      A feature matrix has one row per pixel, so it is tall and a full decomposition computes a
      left singular matrix as large as the input to return a handful of columns. The randomized
      range finder of Halko, Martinsson and Tropp costs O(rows * columns * rank) instead of
      O(rows * columns^2) and needs memory for the rank asked for, not for every column. It is
      used whenever it can save work; below that the exact decomposition is taken.

      The result is deterministic: the random matrix is drawn from a fixed seed, and the sign of
      every component is chosen so that its largest entry is positive. Without that convention the
      sign of a component would be arbitrary and could differ between two runs of the same data. */
  class M2AIADIMENSIONREDUCTION_EXPORT TruncatedSvd
  {
  public:
    /** \param rank            number of singular vectors to return
        \param forceExact      skip the randomized range finder whatever the size of the matrix
        \param oversampling    extra columns the range finder uses to meet the wanted accuracy
        \param powerIterations passes over the matrix that sharpen the separation of the singular
                               values; two is enough for the decay a feature matrix shows */
    static TruncatedSvdResult Compute(const Eigen::MatrixXf &A,
                                      unsigned int rank,
                                      bool forceExact = false,
                                      unsigned int oversampling = 10,
                                      unsigned int powerIterations = 2,
                                      unsigned int seed = 42);

  private:
    static TruncatedSvdResult Exact(const Eigen::MatrixXf &A, unsigned int rank);
    static TruncatedSvdResult Randomized(const Eigen::MatrixXf &A,
                                         unsigned int rank,
                                         unsigned int oversampling,
                                         unsigned int powerIterations,
                                         unsigned int seed);
    /** Makes the largest entry of every component positive. */
    static void NormalizeSigns(TruncatedSvdResult &result);
  };
} // namespace m2
