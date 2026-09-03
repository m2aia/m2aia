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
#include <m2EmbeddingFilterBase.h>

namespace m2
{
  /** Non-negative matrix factorization of a m2::SpectralFeatureMatrix.

      The feature matrix is approximated by a product of two non-negative factors, W of the pixels
      by the components and H of the components by the peaks. Because nothing may become negative,
      the components add up instead of cancelling out, and every one of them reads as a spectral
      signature with a distribution over the image - which is what a principal component, free to
      mix positive and negative contributions, does not give.

      The embedding is W, the loadings are H. */
  class M2AIADIMENSIONREDUCTION_EXPORT NmfImageFilter : public m2::EmbeddingFilterBase
  {
  public:
    mitkClassMacro(NmfImageFilter, m2::EmbeddingFilterBase);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

    std::string GetMethodName() const override { return "NMF"; }

    /** Upper bound on the multiplicative update steps. */
    itkSetMacro(Iterations, unsigned int);
    itkGetMacro(Iterations, unsigned int);

    /** The factorization stops early once the residual improves by less than this fraction. */
    itkSetMacro(Tolerance, double);
    itkGetMacro(Tolerance, double);

    /** How far the factors are from the feature matrix, relative to it, after the last step. */
    itkGetMacro(RelativeResidual, double);

    /** Number of update steps that were actually taken. */
    itkGetMacro(PerformedIterations, unsigned int);

  protected:
    NmfImageFilter() = default;
    ~NmfImageFilter() override = default;

    void ComputeEmbedding() override;

  private:
    /** Non-negative double singular value decomposition: a deterministic starting point that
        converges much faster than random factors and makes a run reproducible. */
    void InitializeFactors(const Eigen::MatrixXf &A, unsigned int components);

    Eigen::MatrixXf m_W;
    Eigen::MatrixXf m_H;
    unsigned int m_Iterations = 200;
    unsigned int m_PerformedIterations = 0;
    double m_Tolerance = 1e-4;
    double m_RelativeResidual = 0.0;
  };
} // namespace m2
