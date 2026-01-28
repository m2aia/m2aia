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
#include <m2SpectrumImage.h>
#include <mitkImage.h>
#include <mitkImageAccessByItk.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkLabelSetImage.h>
#include <mitkProgressBar.h>

// OpenMP
#include <omp.h>


// Function to calculate Euclidean distance between two points
double euclideanDistance(const Eigen::VectorXd &point1, const Eigen::VectorXd &point2)
{
  return (point1 - point2).norm();
}

// Function to calculate correlation distance between two points
double correlationDistance(const Eigen::VectorXd &point1, const Eigen::VectorXd &point2)
{
  double mean1 = point1.mean();
  double mean2 = point2.mean();
  double numerator = (point1.array() - mean1).matrix().dot((point2.array() - mean2).matrix());
  double denominator = std::sqrt((point1.array() - mean1).square().sum() * (point2.array() - mean2).square().sum());
  return 1.0 - (numerator / denominator);
}

// Function to calculate cosine similarity between two points
double cosineSimilarity(const Eigen::VectorXd &point1, const Eigen::VectorXd &point2)
{
  double dotProduct = point1.dot(point2);
  double norm1 = point1.norm();
  double norm2 = point2.norm();
  return dotProduct / (norm1 * norm2);
}


double m2::KMeansImageFilter::ComputeDistance(const Eigen::VectorXd &point1, const Eigen::VectorXd &point2) const
{
  switch (m_DistanceMetric)
  {
    case DistanceMetric::EUCLIDEAN:
      return euclideanDistance(point1, point2);
    case DistanceMetric::CORRELATION:
      return correlationDistance(point1, point2);
    case DistanceMetric::COSINE:
      return 1.0 - cosineSimilarity(point1, point2); // Convert similarity to distance
    default:
      return euclideanDistance(point1, point2);
  }
}


// Calculate spatial distance between two pixel coordinates
double m2::KMeansImageFilter::ComputeSpatialDistance(const itk::Index<3> &coord1, const itk::Index<3> &coord2) const
{
  double sum = 0.0;
  for (int i = 0; i < 3; ++i)
  {
    double diff = static_cast<double>(coord1[i] - coord2[i]);
    sum += diff * diff;
  }
  return std::sqrt(sum);
}

void m2::KMeansImageFilter::GenerateData()
{
  m_ValidIndicesMap.clear();
  m_Outputs.clear();
  MITK_INFO << "Start KMeansImageFilter ....";

  if (!m_Intervals.size())
  {
    MITK_INFO << "Intervals are not set";
    return;
  }

  for (auto [imageId, image] : m_Inputs)
  {
    if (auto spectrumImage = dynamic_cast<m2::ImzMLSpectrumImage *>(image.GetPointer()))
    {
      if (!spectrumImage->GetImageAccessInitialized())
        MITK_INFO << "Image access not initialized";

      if (!spectrumImage->GetMultilabelSegmentation())
        MITK_INFO << "Mask image not set";

      std::vector<itk::Index<3>> validIndices;
      auto maskImage = spectrumImage->GetMultilabelSegmentation()->GetGroupImage(0);
      mitk::ImagePixelReadAccessor<mitk::MultiLabelSegmentation::LabelValueType, 3> maskAcc(maskImage);

      for (auto s : spectrumImage->GetSpectra())
        if (maskAcc.GetPixelByIndex(s.index) != 0)
          validIndices.push_back(s.index);

      m_ValidIndicesMap[imageId] = validIndices;
      MITK_INFO << "Image found with " << validIndices.size() << " valid indices";
    }
  }

  auto N = std::accumulate(m_ValidIndicesMap.begin(),
                           m_ValidIndicesMap.end(),
                           0,
                           [](auto &acc, auto &pair) { return acc + pair.second.size(); });
  MITK_INFO << "N: " << N;

  Eigen::MatrixXd data(N, m_Intervals.size());

  MITK_INFO << "Start filling data matrix ....";
  size_t offset = 0;
  mitk::ProgressBar::GetInstance()->AddStepsToDo(m_Inputs.size() * m_Intervals.size());
  for (auto [imageId, image] : m_Inputs)
  {
    auto validIndices = m_ValidIndicesMap[imageId];
    auto spectrumImage = dynamic_cast<m2::SpectrumImage *>(image.GetPointer());

    // fill the data matrix
    mitk::Image::Pointer ionImage = spectrumImage->mitk::Image::Clone();
    for (size_t col = 0; col < m_Intervals.size(); ++col)
    {
      const auto mz = m_Intervals.at(col).x.mean();
      spectrumImage->GetImage(mz, spectrumImage->ApplyTolerance(mz), spectrumImage->GetMultilabelSegmentation()->GetGroupImage(0), ionImage);

      size_t v = offset;
      for (auto index : validIndices)
      {
        mitk::ImagePixelReadAccessor<m2::DisplayImagePixelType, 3> acc(ionImage);
        data(v++, col) = acc.GetPixelByIndex(index);
      }
      mitk::ProgressBar::GetInstance()->Progress();
    }
    offset += validIndices.size();
  }

  MITK_INFO << "Data matrix filled ....";
  // Normalize data
  for (int col = 0; col < data.cols(); ++col)
  {
    double minVal = data.col(col).minCoeff();
    double maxVal = data.col(col).maxCoeff();
    if (maxVal - minVal > 0)
      data.col(col) = (data.col(col).array() - minVal) / (maxVal - minVal);
    else
      data.col(col).setZero();
  }


  MITK_INFO << "Start clustering ....";
  std::vector<int> clusterAssignments(data.rows(), -1);
  // Collect all spatial coordinates in a single vector
  std::vector<itk::Index<3>> allCoordinates;
  allCoordinates.reserve(N);
  for (auto &[imageId, indices] : m_ValidIndicesMap)
  {
    allCoordinates.insert(allCoordinates.end(), indices.begin(), indices.end());
  }

  // Choose clustering algorithm based on selected variant
  switch (m_KMeansVariant)
  {
    case KMeansVariant::STANDARD:
    {
      m_SpatialWeight = 0.0; // Disable spatial weighting for standard KMeans
      DoSpatialKMeans(data, allCoordinates, m_NumberOfClusters, clusterAssignments);
    }
      break;
    case KMeansVariant::SPATIAL:
      DoSpatialKMeans(data, allCoordinates, m_NumberOfClusters, clusterAssignments);
      break;
    default:
    {
      m_SpatialWeight = 0.0; // Disable spatial weighting for standard KMeans
      DoSpatialKMeans(data, allCoordinates, m_NumberOfClusters, clusterAssignments);
    }
  }

  auto classLabelIterator = clusterAssignments.begin();
  for (auto [imageId, image] : m_Inputs)
  {
    auto spectrumImage = dynamic_cast<m2::SpectrumImage *>(image.GetPointer());

    auto clusteredImage = dynamic_cast<mitk::MultiLabelSegmentation *>(this->GetOutput(imageId).GetPointer());
    auto maskImage = spectrumImage->GetMultilabelSegmentation()->GetGroupImage(0);
    clusteredImage->Initialize(maskImage);

    // Get the underlying group image (group 0 is created by Initialize)
    auto groupImage = clusteredImage->GetGroupImage(0);
    
    {
      mitk::ImagePixelWriteAccessor<mitk::MultiLabelSegmentation::LabelValueType, 3> c_acc(groupImage);
      for (auto s : m_ValidIndicesMap[imageId])
        c_acc.SetPixelByIndex(s, *classLabelIterator++ + 1);
    }

    clusteredImage->InitializeByLabeledImage(groupImage);
  }
}


void m2::KMeansImageFilter::DoSpatialKMeans(const Eigen::MatrixXd &data,
                                                    const std::vector<itk::Index<3>> &spatialCoordinates,
                                                    int k,
                                                    std::vector<int> &clusterAssignments)
{
  MITK_INFO << "Start Spectral-Spatial KMeans: rows: " << data.rows() << " cols: " << data.cols();
  MITK_INFO << "Spatial weight: " << m_SpatialWeight;

  if (spatialCoordinates.size() != static_cast<size_t>(data.rows()))
  {
    MITK_ERROR << "Number of spatial coordinates doesn't match number of data points";
    return;
  }

  // Create combined feature matrix including both spectral and spatial features
  int spatialDims = 3; // x, y, z coordinates
  Eigen::MatrixXd combinedData(data.rows(), data.cols() + spatialDims);

  // Find spatial min and max for normalization
  std::vector<double> spatialMin(spatialDims, std::numeric_limits<double>::max());
  std::vector<double> spatialMax(spatialDims, std::numeric_limits<double>::lowest());

  for (const auto &coord : spatialCoordinates)
  {
    for (int dim = 0; dim < spatialDims; ++dim)
    {
      spatialMin[dim] = std::min(spatialMin[dim], static_cast<double>(coord[dim]));
      spatialMax[dim] = std::max(spatialMax[dim], static_cast<double>(coord[dim]));
    }
  }

  // Calculate spatial feature scaling factor
  // This controls how much emphasis to put on spatial proximity
  double spatialScalingFactor = 2.0;

  // Combine spectral and spatial features
  for (int i = 0; i < data.rows(); ++i)
  {
    // Copy spectral data
    combinedData.block(i, 0, 1, data.cols()) = data.row(i);

    // Add scaled spatial data
    for (int dim = 0; dim < spatialDims; ++dim)
    {
      // Normalize spatial coordinates to [0,1] range
      double range = spatialMax[dim] - spatialMin[dim];
      double normalizedCoord = (range > 0) ? (spatialCoordinates[i][dim] - spatialMin[dim]) / range : 0;

      // Scale the spatial components by the spatial weight factor
      combinedData(i, data.cols() + dim) = normalizedCoord * m_SpatialWeight * spatialScalingFactor;
    }
  }

  // Now run standard k-means on the combined feature space
  // Initialize centroids
  m_Centroids.resize(k);
  for (int i = 0; i < k; ++i)
  {
    int randomIndex = rand() % combinedData.rows();
    m_Centroids[i] = combinedData.row(randomIndex);
  }

  bool centroidsChanged = true;
  int maxIterations = 100;
  int iteration = 0;

  while (centroidsChanged && iteration < maxIterations)
  {
    centroidsChanged = false;
    iteration++;

// Assign each point to closest centroid
#pragma omp parallel for
    for (int i = 0; i < combinedData.rows(); ++i)
    {
      double minDistance = std::numeric_limits<double>::max();
      int closestCentroid = -1;

      for (int j = 0; j < k; ++j)
      {
        double distance = ComputeDistance(combinedData.row(i), m_Centroids[j]);
        if (distance < minDistance)
        {
          minDistance = distance;
          closestCentroid = j;
        }
      }

      if (clusterAssignments[i] != closestCentroid)
      {
#pragma omp critical
        {
          clusterAssignments[i] = closestCentroid;
          centroidsChanged = true;
        }
      }
    }

    // Update centroids
    std::vector<Eigen::VectorXd> newCentroids(k, Eigen::VectorXd::Zero(combinedData.cols()));
    std::vector<int> counts(k, 0);

    for (int i = 0; i < combinedData.rows(); ++i)
    {
      int cluster = clusterAssignments[i];
      newCentroids[cluster] += combinedData.row(i);
      counts[cluster]++;
    }

    for (int j = 0; j < k; ++j)
    {
      if (counts[j] > 0)
      {
        newCentroids[j] /= counts[j];
        m_Centroids[j] = newCentroids[j];
      }
    }

    MITK_INFO << "Spectral-Spatial KMeans iteration " << iteration << " completed";
  }

  // Store final centroids (only the spectral part)
  for (int i = 0; i < k; ++i)
  {
    m_Centroids[i] = m_Centroids[i].head(data.cols());
  }

  MITK_INFO << "Spectral-Spatial KMeans finished after " << iteration << " iterations";
}
