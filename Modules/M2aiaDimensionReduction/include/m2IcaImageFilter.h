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
  /** Independent component analysis of a m2::SpectralFeatureMatrix, with the fixed point
      algorithm of Hyvaerinen (FastICA).

      A principal component is the direction of the largest remaining variance, which mixes
      whatever happens to vary together. An independent component is instead the direction whose
      distribution is least Gaussian, which separates signals that overlap in the image but do not
      depend on each other. The components come out unordered - unlike a decomposition by variance
      there is no first, second and third.

      The data is whitened with a truncated decomposition first, so the number of components also
      sets how much of the variance is kept. */
  class M2AIADIMENSIONREDUCTION_EXPORT IcaImageFilter : public m2::EmbeddingFilterBase
  {
  public:
    mitkClassMacro(IcaImageFilter, m2::EmbeddingFilterBase);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

    std::string GetMethodName() const override { return "ICA"; }

    /** Upper bound on the fixed point steps. */
    itkSetMacro(Iterations, unsigned int);
    itkGetMacro(Iterations, unsigned int);

    /** The iteration stops once no component turns by more than this much. */
    itkSetMacro(Tolerance, double);
    itkGetMacro(Tolerance, double);

    /** Steepness of the tanh contrast function; one is the usual choice and values outside
        [1, 2] are unusual. */
    itkSetMacro(Alpha, double);
    itkGetMacro(Alpha, double);

    itkGetMacro(PerformedIterations, unsigned int);
    itkGetMacro(Converged, bool);

  protected:
    IcaImageFilter() = default;
    ~IcaImageFilter() override = default;

    void ComputeEmbedding() override;

  private:
    /** Replaces a matrix by the closest one with orthonormal rows. Doing this to all components
        at once, instead of removing each from the next, keeps the components from being ordered
        by the accident of which was found first. */
    static Eigen::MatrixXf SymmetricDecorrelation(const Eigen::MatrixXf &W);

    unsigned int m_Iterations = 200;
    unsigned int m_PerformedIterations = 0;
    double m_Tolerance = 1e-4;
    double m_Alpha = 1.0;
    bool m_Converged = false;
  };
} // namespace m2
