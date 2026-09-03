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
  /** Principal component analysis of a m2::SpectralFeatureMatrix.

      The embedding holds the leading components of the decomposition, one column each, and the
      loadings say how much every peak contributes to them. Only the components that are asked for
      are computed; see m2::TruncatedSvd for what that saves on a feature matrix. */
  class M2AIADIMENSIONREDUCTION_EXPORT PcaImageFilter : public m2::EmbeddingFilterBase
  {
  public:
    mitkClassMacro(PcaImageFilter, m2::EmbeddingFilterBase);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

    std::string GetMethodName() const override { return "PCA"; }

    /** Computes every component instead of only the ones that are asked for. Slower, and only
        worth it to check the approximation the randomized decomposition makes. */
    itkSetMacro(ExactDecomposition, bool);
    itkGetMacro(ExactDecomposition, bool);

    /** The value subtracted from every pixel before the decomposition. */
    const Eigen::VectorXf &GetMeanImage() const { return m_MeanImage; }

    /** The singular values of the components that were kept. */
    const Eigen::VectorXf &GetSingularValues() const { return m_SingularValues; }

  protected:
    PcaImageFilter() = default;
    ~PcaImageFilter() override = default;

    void ComputeEmbedding() override;

    Eigen::VectorXf m_MeanImage;
    Eigen::VectorXf m_SingularValues;
    bool m_ExactDecomposition = false;
  };
} // namespace m2
