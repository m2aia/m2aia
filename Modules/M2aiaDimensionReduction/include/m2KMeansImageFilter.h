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
  /** How the distance between a pixel and a cluster centroid is measured. */
  enum class DistanceMetric
  {
    EUCLIDEAN,
    CORRELATION,
    COSINE
  };

  /** Which features the clustering runs on. */
  enum class KMeansVariant
  {
    STANDARD, // the peak intensities alone
    SPATIAL   // the peak intensities together with the position of the pixel
  };

  /** K-means over the pixels of a m2::SpectralFeatureMatrix.

      Unlike the embedding methods this produces a cluster assignment, so the result is published
      as a segmentation. All images of the feature matrix are clustered together, which is what
      makes the clusters comparable between them. */
  class M2AIADIMENSIONREDUCTION_EXPORT KMeansImageFilter : public m2::EmbeddingFilterBase
  {
  public:
    mitkClassMacro(KMeansImageFilter, m2::EmbeddingFilterBase);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

    std::string GetMethodName() const override { return "KMeans"; }
    bool ProducesLabels() const override { return true; }

    itkSetMacro(NumberOfClusters, unsigned int);
    itkGetMacro(NumberOfClusters, unsigned int);

    itkSetMacro(SpatialWeight, double);
    itkGetMacro(SpatialWeight, double);

    itkSetMacro(MaximumIterations, unsigned int);
    itkGetMacro(MaximumIterations, unsigned int);

    void SetDistanceMetric(DistanceMetric metric) { m_DistanceMetric = metric; }
    DistanceMetric GetDistanceMetric() const { return m_DistanceMetric; }

    void SetKMeansVariant(KMeansVariant variant) { m_KMeansVariant = variant; }
    KMeansVariant GetKMeansVariant() const { return m_KMeansVariant; }

    /** The centroids of the last run, restricted to their spectral part. */
    const std::vector<Eigen::VectorXd> &GetCentroids() const { return m_Centroids; }

  protected:
    KMeansImageFilter() = default;
    ~KMeansImageFilter() override = default;

    void ComputeEmbedding() override;

  private:
    /** Runs k-means on the peak intensities, extended by the scaled pixel position when the
        spatial variant is selected. */
    void DoKMeans(const Eigen::MatrixXd &data, const std::vector<itk::Index<3>> &spatialCoordinates);

    double ComputeDistance(const Eigen::VectorXd &point1, const Eigen::VectorXd &point2) const;

    unsigned int m_NumberOfClusters = 0;
    unsigned int m_MaximumIterations = 100;
    double m_SpatialWeight = 0.5;
    DistanceMetric m_DistanceMetric = DistanceMetric::EUCLIDEAN;
    KMeansVariant m_KMeansVariant = KMeansVariant::STANDARD;
    std::vector<Eigen::VectorXd> m_Centroids;
  };
} // namespace m2
