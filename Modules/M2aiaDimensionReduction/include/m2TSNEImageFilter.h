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
  /** Barnes-Hut t-SNE of a m2::SpectralFeatureMatrix.

      The features are expected to be standardized; the columns of the embedding are the
      coordinates of the low dimensional map, in the units t-SNE produces them. Rescaling them for
      display is left to the caller. */
  class M2AIADIMENSIONREDUCTION_EXPORT TSNEImageFilter : public m2::EmbeddingFilterBase
  {
  public:
    mitkClassMacro(TSNEImageFilter, m2::EmbeddingFilterBase);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

    std::string GetMethodName() const override { return "tSNE"; }

    itkGetMacro(Perplexity, unsigned int);
    itkSetMacro(Perplexity, unsigned int);

    itkGetMacro(Theta, double);
    itkSetMacro(Theta, double);

    itkGetMacro(Iterations, unsigned int);
    itkSetMacro(Iterations, unsigned int);

  protected:
    TSNEImageFilter() = default;
    ~TSNEImageFilter() override = default;

    void ComputeEmbedding() override;

    unsigned int m_Perplexity = 2;
    unsigned int m_Iterations = 200;
    double m_Theta = 0.5;
  };
} // namespace m2
