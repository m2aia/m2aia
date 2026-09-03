/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2KMeansImageFilter.h>

// OpenMP
#include <omp.h>

#include <algorithm>
#include <limits>

namespace
{
  double EuclideanDistance(const Eigen::VectorXd &point1, const Eigen::VectorXd &point2)
  {
    return (point1 - point2).norm();
  }

  double CorrelationDistance(const Eigen::VectorXd &point1, const Eigen::VectorXd &point2)
  {
    const double mean1 = point1.mean();
    const double mean2 = point2.mean();
    const double numerator = (point1.array() - mean1).matrix().dot((point2.array() - mean2).matrix());
    const double denominator =
      std::sqrt((point1.array() - mean1).square().sum() * (point2.array() - mean2).square().sum());
    return 1.0 - (numerator / denominator);
  }

  double CosineSimilarity(const Eigen::VectorXd &point1, const Eigen::VectorXd &point2)
  {
    return point1.dot(point2) / (point1.norm() * point2.norm());
  }
} // namespace

double m2::KMeansImageFilter::ComputeDistance(const Eigen::VectorXd &point1, const Eigen::VectorXd &point2) const
{
  switch (m_DistanceMetric)
  {
    case DistanceMetric::CORRELATION:
      return CorrelationDistance(point1, point2);
    case DistanceMetric::COSINE:
      return 1.0 - CosineSimilarity(point1, point2); // a similarity turned into a distance
    case DistanceMetric::EUCLIDEAN:
    default:
      return EuclideanDistance(point1, point2);
  }
}

void m2::KMeansImageFilter::ComputeEmbedding()
{
  if (m_NumberOfClusters == 0)
  {
    MITK_ERROR << "K-Means was started without a number of clusters.";
    return;
  }

  if (static_cast<unsigned int>(m_Features.data.rows()) < m_NumberOfClusters)
  {
    MITK_ERROR << "Fewer pixels than clusters; nothing to cluster.";
    return;
  }

  // the clustering works in double precision, the feature matrix is kept small in float
  const Eigen::MatrixXd data = m_Features.data.cast<double>();
  m_Labels.assign(static_cast<size_t>(data.rows()), 0);

  DoKMeans(data, m_Features.indices);
}

void m2::KMeansImageFilter::DoKMeans(const Eigen::MatrixXd &data,
                                     const std::vector<itk::Index<3>> &spatialCoordinates)
{
  const int k = static_cast<int>(m_NumberOfClusters);
  MITK_INFO << "K-Means on " << data.rows() << " pixels and " << data.cols() << " peaks, " << k << " clusters";

  if (spatialCoordinates.size() != static_cast<size_t>(data.rows()))
  {
    MITK_ERROR << "Number of spatial coordinates doesn't match number of data points";
    return;
  }

  // the standard variant is the spatial one without any weight on the position
  const double spatialWeight = m_KMeansVariant == KMeansVariant::SPATIAL ? m_SpatialWeight : 0.0;
  const int spatialDims = 3;
  Eigen::MatrixXd combinedData(data.rows(), data.cols() + spatialDims);

  std::vector<double> spatialMin(spatialDims, std::numeric_limits<double>::max());
  std::vector<double> spatialMax(spatialDims, std::numeric_limits<double>::lowest());
  for (const auto &coord : spatialCoordinates)
    for (int dim = 0; dim < spatialDims; ++dim)
    {
      spatialMin[dim] = std::min(spatialMin[dim], static_cast<double>(coord[dim]));
      spatialMax[dim] = std::max(spatialMax[dim], static_cast<double>(coord[dim]));
    }

  // how much the position may outweigh the intensities at a spatial weight of one
  const double spatialScalingFactor = 2.0;

  for (Eigen::Index i = 0; i < data.rows(); ++i)
  {
    combinedData.block(i, 0, 1, data.cols()) = data.row(i);
    for (int dim = 0; dim < spatialDims; ++dim)
    {
      const double range = spatialMax[dim] - spatialMin[dim];
      const double normalizedCoord = range > 0 ? (spatialCoordinates[i][dim] - spatialMin[dim]) / range : 0.0;
      combinedData(i, data.cols() + dim) = normalizedCoord * spatialWeight * spatialScalingFactor;
    }
  }

  m_Centroids.resize(k);
  for (int i = 0; i < k; ++i)
    m_Centroids[i] = combinedData.row(rand() % combinedData.rows());

  std::vector<int> assignments(combinedData.rows(), -1);
  bool centroidsChanged = true;
  unsigned int iteration = 0;

  while (centroidsChanged && iteration < m_MaximumIterations && !IsCancelRequested())
  {
    centroidsChanged = false;
    ++iteration;

#pragma omp parallel for
    for (Eigen::Index i = 0; i < combinedData.rows(); ++i)
    {
      double minDistance = std::numeric_limits<double>::max();
      int closestCentroid = -1;

      for (int j = 0; j < k; ++j)
      {
        const double distance = ComputeDistance(combinedData.row(i), m_Centroids[j]);
        if (distance < minDistance)
        {
          minDistance = distance;
          closestCentroid = j;
        }
      }

      if (assignments[i] != closestCentroid)
      {
#pragma omp critical
        {
          assignments[i] = closestCentroid;
          centroidsChanged = true;
        }
      }
    }

    std::vector<Eigen::VectorXd> newCentroids(k, Eigen::VectorXd::Zero(combinedData.cols()));
    std::vector<int> counts(k, 0);

    for (Eigen::Index i = 0; i < combinedData.rows(); ++i)
    {
      newCentroids[assignments[i]] += combinedData.row(i);
      counts[assignments[i]]++;
    }

    for (int j = 0; j < k; ++j)
      if (counts[j] > 0)
        m_Centroids[j] = newCentroids[j] / counts[j];

    ReportProgress(iteration, m_MaximumIterations);
  }

  m_Labels.resize(assignments.size());
  std::transform(assignments.begin(),
                 assignments.end(),
                 m_Labels.begin(),
                 [](int assignment) { return static_cast<unsigned int>(std::max(assignment, 0)); });

  // only the spectral part of a centroid is a spectrum and worth keeping
  for (int i = 0; i < k; ++i)
    m_Centroids[i] = m_Centroids[i].head(data.cols());

  MITK_INFO << "K-Means finished after " << iteration << " iterations";
}
