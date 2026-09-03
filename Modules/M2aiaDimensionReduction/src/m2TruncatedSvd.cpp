/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2TruncatedSvd.h>

#include <mitkLog.h>

#include <algorithm>
#include <random>

namespace
{
  /** The thin Q of a QR decomposition, the orthonormal basis of the columns of the input. */
  Eigen::MatrixXf OrthonormalBasis(const Eigen::MatrixXf &matrix)
  {
    Eigen::HouseholderQR<Eigen::MatrixXf> qr(matrix);
    return qr.householderQ() * Eigen::MatrixXf::Identity(matrix.rows(), matrix.cols());
  }
} // namespace

void m2::TruncatedSvd::NormalizeSigns(m2::TruncatedSvdResult &result)
{
  for (Eigen::Index component = 0; component < result.U.cols(); ++component)
  {
    Eigen::Index largest = 0;
    result.U.col(component).cwiseAbs().maxCoeff(&largest);
    if (result.U(largest, component) < 0.0f)
    {
      result.U.col(component) *= -1.0f;
      result.V.col(component) *= -1.0f;
    }
  }
}

m2::TruncatedSvdResult m2::TruncatedSvd::Exact(const Eigen::MatrixXf &A, unsigned int rank)
{
  Eigen::JacobiSVD<Eigen::MatrixXf> svd(A, Eigen::ComputeThinU | Eigen::ComputeThinV);

  const auto keep = std::min<Eigen::Index>(rank, svd.singularValues().size());

  TruncatedSvdResult result;
  result.U = svd.matrixU().leftCols(keep);
  result.S = svd.singularValues().head(keep);
  result.V = svd.matrixV().leftCols(keep);

  return result;
}

m2::TruncatedSvdResult m2::TruncatedSvd::Randomized(const Eigen::MatrixXf &A,
                                                    unsigned int rank,
                                                    unsigned int oversampling,
                                                    unsigned int powerIterations,
                                                    unsigned int seed)
{
  const auto sketchSize = std::min<Eigen::Index>(A.cols(), static_cast<Eigen::Index>(rank) + oversampling);

  std::mt19937 generator(seed);
  std::normal_distribution<float> distribution(0.0f, 1.0f);

  Eigen::MatrixXf sketch(A.cols(), sketchSize);
  for (Eigen::Index column = 0; column < sketch.cols(); ++column)
    for (Eigen::Index row = 0; row < sketch.rows(); ++row)
      sketch(row, column) = distribution(generator);

  // a basis of a random subspace of the columns of A, sharpened by alternating passes over A so
  // that it lines up with the leading singular vectors
  Eigen::MatrixXf basis = OrthonormalBasis(A * sketch);
  for (unsigned int iteration = 0; iteration < powerIterations; ++iteration)
  {
    basis = OrthonormalBasis(A.transpose() * basis);
    basis = OrthonormalBasis(A * basis);
  }

  // A projected onto that basis is small enough to decompose exactly
  const Eigen::MatrixXf projected = basis.transpose() * A;
  Eigen::JacobiSVD<Eigen::MatrixXf> svd(projected, Eigen::ComputeThinU | Eigen::ComputeThinV);

  const auto keep = std::min<Eigen::Index>(rank, svd.singularValues().size());

  TruncatedSvdResult result;
  result.U = basis * svd.matrixU().leftCols(keep);
  result.S = svd.singularValues().head(keep);
  result.V = svd.matrixV().leftCols(keep);

  return result;
}

m2::TruncatedSvdResult m2::TruncatedSvd::Compute(const Eigen::MatrixXf &A,
                                                 unsigned int rank,
                                                 bool forceExact,
                                                 unsigned int oversampling,
                                                 unsigned int powerIterations,
                                                 unsigned int seed)
{
  if (A.rows() == 0 || A.cols() == 0 || rank == 0)
    return {};

  // the randomized range finder only pays off while it looks at fewer columns than the matrix has
  const bool randomized =
    !forceExact && (static_cast<Eigen::Index>(rank) + oversampling) < A.cols();

  MITK_INFO << "Truncated SVD of " << A.rows() << "x" << A.cols() << " for " << rank
            << " components, " << (randomized ? "randomized" : "exact");

  auto result = randomized ? Randomized(A, rank, oversampling, powerIterations, seed) : Exact(A, rank);
  NormalizeSigns(result);

  return result;
}
