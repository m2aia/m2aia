/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2IcaImageFilter.h>
#include <m2TruncatedSvd.h>

#include <algorithm>
#include <cmath>

Eigen::MatrixXf m2::IcaImageFilter::SymmetricDecorrelation(const Eigen::MatrixXf &W)
{
  // W <- (W W^T)^(-1/2) W, evaluated through the eigen decomposition of the small square W W^T
  const Eigen::MatrixXf gram = W * W.transpose();
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXf> solver(gram);

  Eigen::VectorXf inverseRoot = solver.eigenvalues();
  for (Eigen::Index i = 0; i < inverseRoot.size(); ++i)
    inverseRoot(i) = inverseRoot(i) > 0.0f ? 1.0f / std::sqrt(inverseRoot(i)) : 0.0f;

  return solver.eigenvectors() * inverseRoot.asDiagonal() * solver.eigenvectors().transpose() * W;
}

void m2::IcaImageFilter::ComputeEmbedding()
{
  const auto &data = m_Features.data;
  const auto numberOfPixels = data.rows();
  const auto components =
    std::min<unsigned int>(m_NumberOfComponents, static_cast<unsigned int>(std::min(data.cols(), numberOfPixels)));

  if (components == 0 || numberOfPixels < 2)
  {
    MITK_ERROR << "ICA needs at least two pixels and one component.";
    return;
  }

  MITK_INFO << "ICA on a matrix of " << numberOfPixels << " pixels and " << data.cols() << " peaks, " << components
            << " components";
  ReportProgress(0, m_Iterations);

  // every peak is centred; the whitening below assumes a mean of zero per column
  const Eigen::RowVectorXf columnMeans = data.colwise().mean();
  const Eigen::MatrixXf centered = data.rowwise() - columnMeans;

  // whitening: the leading left singular vectors, scaled so that the columns have unit variance
  // and no correlation left between them
  const auto decomposition = m2::TruncatedSvd::Compute(centered, components);
  const auto rank = decomposition.GetRank();
  if (rank == 0)
  {
    MITK_ERROR << "The data could not be whitened; ICA is not possible.";
    return;
  }

  const float scale = std::sqrt(static_cast<float>(numberOfPixels));
  const Eigen::MatrixXf whitened = decomposition.U * scale; // pixels by rank

  Eigen::MatrixXf W = SymmetricDecorrelation(Eigen::MatrixXf::Identity(rank, rank));
  m_Converged = false;
  m_PerformedIterations = 0;

  const float alpha = static_cast<float>(m_Alpha);

  for (unsigned int iteration = 0; iteration < m_Iterations && !IsCancelRequested(); ++iteration)
  {
    // projections of every pixel onto the current components
    const Eigen::MatrixXf projections = whitened * W.transpose(); // pixels by rank

    // the contrast function and its derivative; tanh reacts to distributions with heavier tails
    // than a Gaussian, which is what separates real signals from their mixtures
    const Eigen::MatrixXf g = (projections.array() * alpha).tanh();
    const Eigen::MatrixXf gDerivative = alpha * (1.0f - g.array().square());

    const Eigen::MatrixXf expectation = (g.transpose() * whitened) / static_cast<float>(numberOfPixels);
    const Eigen::VectorXf meanDerivative = gDerivative.colwise().mean();

    const Eigen::MatrixXf candidate = expectation - meanDerivative.asDiagonal() * W;
    const Eigen::MatrixXf updated = SymmetricDecorrelation(candidate);

    // how far each component turned; one means it did not move at all
    const Eigen::VectorXf alignment = (updated * W.transpose()).diagonal().cwiseAbs();
    const float largestChange = (alignment.array() - 1.0f).abs().maxCoeff();

    W = updated;
    m_PerformedIterations = iteration + 1;
    ReportProgress(iteration + 1, m_Iterations);

    if (largestChange < static_cast<float>(m_Tolerance))
    {
      m_Converged = true;
      break;
    }
  }

  if (!m_Converged && !IsCancelRequested())
    MITK_WARN << "ICA did not converge within " << m_Iterations << " iterations; the components are still usable "
                 "but raising the iterations or lowering the number of components may help.";

  m_Embedding = whitened * W.transpose();

  // the components expressed over the peaks again, so that each of them reads as a spectrum
  m_Loadings = W * decomposition.S.head(rank).asDiagonal() * decomposition.V.transpose() / scale;

  MITK_INFO << "ICA finished after " << m_PerformedIterations << " iterations";
}
