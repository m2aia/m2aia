/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2NmfImageFilter.h>
#include <m2TruncatedSvd.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
  /** Keeps a denominator away from zero without shifting the result noticeably. */
  constexpr float EPSILON = 1e-9f;

  /** The positive part of a vector, the negative part with its sign dropped. */
  Eigen::VectorXf PositivePart(const Eigen::VectorXf &v)
  {
    return v.cwiseMax(0.0f);
  }

  Eigen::VectorXf NegativePart(const Eigen::VectorXf &v)
  {
    return (-v).cwiseMax(0.0f);
  }
} // namespace

void m2::NmfImageFilter::InitializeFactors(const Eigen::MatrixXf &A, unsigned int components)
{
  const auto decomposition = m2::TruncatedSvd::Compute(A, components);
  const auto rank = decomposition.GetRank();

  m_W = Eigen::MatrixXf::Zero(A.rows(), rank);
  m_H = Eigen::MatrixXf::Zero(rank, A.cols());

  // the leading singular vectors of a non-negative matrix are non-negative up to their sign, so
  // the first component can be taken over directly
  m_W.col(0) = std::sqrt(decomposition.S(0)) * decomposition.U.col(0).cwiseAbs();
  m_H.row(0) = std::sqrt(decomposition.S(0)) * decomposition.V.col(0).cwiseAbs().transpose();

  // for the others the positive and the negative part are each a candidate; the one carrying more
  // energy is kept, which is the construction of Boutsidis and Gallopoulos
  for (unsigned int component = 1; component < rank; ++component)
  {
    const Eigen::VectorXf u = decomposition.U.col(component);
    const Eigen::VectorXf v = decomposition.V.col(component);

    const Eigen::VectorXf uPositive = PositivePart(u), uNegative = NegativePart(u);
    const Eigen::VectorXf vPositive = PositivePart(v), vNegative = NegativePart(v);

    const float positiveEnergy = uPositive.norm() * vPositive.norm();
    const float negativeEnergy = uNegative.norm() * vNegative.norm();

    const bool takePositive = positiveEnergy >= negativeEnergy;
    const Eigen::VectorXf &left = takePositive ? uPositive : uNegative;
    const Eigen::VectorXf &right = takePositive ? vPositive : vNegative;
    const float energy = takePositive ? positiveEnergy : negativeEnergy;

    if (left.norm() <= 0.0f || right.norm() <= 0.0f)
      continue;

    const float scale = std::sqrt(decomposition.S(component) * energy);
    m_W.col(component) = scale * left / left.norm();
    m_H.row(component) = (scale * right / right.norm()).transpose();
  }

  // a factor that starts at zero can never leave it again under multiplicative updates, so the
  // zeros are lifted to a small share of the average intensity
  const float floorValue = A.mean() * 0.01f + EPSILON;
  m_W = m_W.cwiseMax(floorValue);
  m_H = m_H.cwiseMax(floorValue);
}

void m2::NmfImageFilter::ComputeEmbedding()
{
  Eigen::MatrixXf A = m_Features.data;
  const auto components = std::min<unsigned int>(m_NumberOfComponents, static_cast<unsigned int>(A.cols()));

  if (components == 0)
  {
    MITK_ERROR << "NMF was started without a component to compute.";
    return;
  }

  // the factorization is only defined on non-negative data; intensities are non-negative, but a
  // scaling of the feature matrix may have moved them
  if (A.minCoeff() < 0.0f)
  {
    MITK_WARN << "The feature matrix contains negative values; they are treated as zero. Use a "
                 "scaling that keeps the intensities non-negative.";
    A = A.cwiseMax(0.0f);
  }

  const double norm = A.norm();
  if (norm <= 0.0)
  {
    MITK_ERROR << "The feature matrix is empty; nothing to factorize.";
    return;
  }

  MITK_INFO << "NMF on a matrix of " << A.rows() << " pixels and " << A.cols() << " peaks, " << components
            << " components";

  InitializeFactors(A, components);

  double previousResidual = std::numeric_limits<double>::max();
  m_PerformedIterations = 0;

  for (unsigned int iteration = 0; iteration < m_Iterations && !IsCancelRequested(); ++iteration)
  {
    // multiplicative updates keep both factors non-negative without a projection step
    m_H.array() *= (m_W.transpose() * A).array() /
                   ((m_W.transpose() * m_W * m_H).array() + EPSILON);
    m_W.array() *= (A * m_H.transpose()).array() /
                   ((m_W * m_H * m_H.transpose()).array() + EPSILON);

    m_PerformedIterations = iteration + 1;

    // the residual costs a further product of this size, so it is only checked now and then
    if (iteration % 10 == 9 || iteration + 1 == m_Iterations)
    {
      const double residual = (A - m_W * m_H).norm() / norm;
      const double improvement = previousResidual - residual;
      previousResidual = residual;
      m_RelativeResidual = residual;

      if (improvement >= 0.0 && improvement < m_Tolerance)
      {
        MITK_INFO << "NMF converged after " << m_PerformedIterations << " iterations";
        break;
      }
    }

    ReportProgress(iteration + 1, m_Iterations);
  }

  m_Embedding = m_W;
  m_Loadings = m_H;

  MITK_INFO << "NMF finished after " << m_PerformedIterations << " iterations with a relative residual of "
            << m_RelativeResidual;
}
