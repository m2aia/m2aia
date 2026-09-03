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
#include <m2SpectralFeatureMatrix.h>

#include <mitkCommon.h>
#include <itkObject.h>
#include <itkObjectFactory.h>

#include <atomic>
#include <functional>
#include <vector>

namespace m2
{
  /** Common interface of every dimensionality reduction and clustering method of the Data
      Compression view.

      A method takes a m2::SpectralFeatureMatrix and produces either an embedding - one row per
      pixel of the matrix, one column per output component - or a cluster assignment. Turning
      either of them back into an image is the job of m2::EmbeddingResult, so a new method only
      has to implement ComputeEmbedding(). */
  class M2AIADIMENSIONREDUCTION_EXPORT EmbeddingFilterBase : public itk::Object
  {
  public:
    mitkClassMacroItkParent(EmbeddingFilterBase, itk::Object);

    /** The features are moved in, they are large enough that a copy is worth avoiding. */
    void SetFeatures(m2::SpectralFeatureMatrix features) { m_Features = std::move(features); }
    const m2::SpectralFeatureMatrix &GetFeatures() const { return m_Features; }

    /** Number of components the caller asks for; a method may produce fewer. */
    itkSetMacro(NumberOfComponents, unsigned int);
    itkGetMacro(NumberOfComponents, unsigned int);

    /** Called with the number of steps done and the total; must be safe to call from the thread
        the method runs on. */
    void SetProgressCallback(std::function<void(unsigned int, unsigned int)> callback)
    {
      m_ProgressCallback = std::move(callback);
    }

    /** Asks the running method to stop early. Methods check this between iterations, so a
        cancelled run leaves an incomplete but valid result. */
    void RequestCancel() { m_CancelRequested = true; }
    bool IsCancelRequested() const { return m_CancelRequested; }

    /** Runs the method. Does nothing but warn if the feature matrix is empty. */
    void Update();

    /** Rows are the pixels of the feature matrix, columns the output components. Empty for
        methods that only assign clusters. */
    const Eigen::MatrixXf &GetEmbedding() const { return m_Embedding; }

    /** One zero based cluster index per row of the feature matrix. Empty for methods that embed. */
    const std::vector<unsigned int> &GetLabels() const { return m_Labels; }

    /** How much every peak contributes to every component: rows are the components of the
        embedding, columns the peaks of the feature matrix. This is what makes a component
        interpretable as a spectrum, and it is empty for methods such as t-SNE that do not express
        their result in terms of the peaks. */
    const Eigen::MatrixXf &GetLoadings() const { return m_Loadings; }

    /** Whether the result of this method is a cluster assignment rather than an embedding. */
    virtual bool ProducesLabels() const { return false; }

    /** Name used for the result node and its properties, for example "PCA". */
    virtual std::string GetMethodName() const = 0;

  protected:
    EmbeddingFilterBase() = default;
    ~EmbeddingFilterBase() override = default;

    /** Fills m_Embedding, or m_Labels for a clustering method. */
    virtual void ComputeEmbedding() = 0;

    void ReportProgress(unsigned int done, unsigned int total) const
    {
      if (m_ProgressCallback)
        m_ProgressCallback(done, total);
    }

    m2::SpectralFeatureMatrix m_Features;
    Eigen::MatrixXf m_Embedding;
    Eigen::MatrixXf m_Loadings;
    std::vector<unsigned int> m_Labels;
    unsigned int m_NumberOfComponents = 3;

  private:
    std::function<void(unsigned int, unsigned int)> m_ProgressCallback;
    std::atomic<bool> m_CancelRequested{false};
  };
} // namespace m2
