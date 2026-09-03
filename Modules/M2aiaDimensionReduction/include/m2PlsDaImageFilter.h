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
  /** Partial least squares discriminant analysis of a m2::SpectralFeatureMatrix.

      This is the only supervised method of the view: it is told which class every pixel belongs
      to, through the label values of the mask, and looks for the directions that separate those
      classes rather than the directions that carry the most variance. Where PCA answers "what
      varies most in this image", PLS-DA answers "which peaks tell these regions apart", and its
      loadings are the ranked answer to that question.

      Classes are the label values of the mask; the label value zero is the unlabeled background
      and takes no part. At least two classes are required.

      The components are computed with the NIPALS algorithm for PLS2 against a one hot coding of
      the classes. */
  class M2AIADIMENSIONREDUCTION_EXPORT PlsDaImageFilter : public m2::EmbeddingFilterBase
  {
  public:
    mitkClassMacro(PlsDaImageFilter, m2::EmbeddingFilterBase);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

    std::string GetMethodName() const override { return "PLSDA"; }

    /** Divides every peak by its standard deviation before the decomposition, so that a weak peak
        can separate the classes as well as a strong one. This is the usual choice; switching it
        off lets the intensity of a peak decide how much it can contribute. */
    itkSetMacro(AutoScale, bool);
    itkGetMacro(AutoScale, bool);

    /** The class values found in the data, in the order of the columns of the class coding. */
    const std::vector<unsigned int> &GetClassValues() const { return m_ClassValues; }

    /** How strongly every class is associated with every component: rows are the components,
        columns the classes in the order of GetClassValues(). */
    const Eigen::MatrixXf &GetClassLoadings() const { return m_ClassLoadings; }

  protected:
    PlsDaImageFilter() = default;
    ~PlsDaImageFilter() override = default;

    void ComputeEmbedding() override;

  private:
    /** The distinct classes of the feature matrix, background excluded. */
    bool DetermineClasses();

    std::vector<unsigned int> m_ClassValues;
    Eigen::MatrixXf m_ClassLoadings;
    bool m_AutoScale = true;
  };
} // namespace m2
