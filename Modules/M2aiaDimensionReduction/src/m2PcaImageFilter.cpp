/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2PcaImageFilter.h>
#include <m2Timer.h>
#include <m2CoreCommon.h>

#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>

#include <itkVectorImage.h>
#include <itkImageRegionIterator.h>

#include <algorithm>
#include <numeric>

namespace m2 {

void PcaImageFilter::initMatrix()
{
  // Get input images
  auto input = this->GetIndexedInputs();
  if (input.empty()) {
    MITK_ERROR << "No input images provided to PCA filter";
    return;
  }
  
  // Calculate dimensions from first input image
  auto mitkImage = dynamic_cast<mitk::Image*>(input.front().GetPointer());
  if (!mitkImage) {
    MITK_ERROR << "Invalid input image";
    return;
  }
  
  // Only the pixels covered by the mask take part in the decomposition; without a
  // mask all pixels of the image are used.
  m_ValidIndices.clear();
  if (m_MaskImage.IsNotNull())
  {
    this->GetValidIndices();
  }
  else
  {
    const auto dimensions = mitkImage->GetDimensions();
    for (unsigned int x = 0; x < dimensions[0]; ++x)
      for (unsigned int y = 0; y < dimensions[1]; ++y)
        for (unsigned int z = 0; z < dimensions[2]; ++z)
          m_ValidIndices.push_back({x, y, z});
  }

  if (m_ValidIndices.empty()) {
    MITK_ERROR << "The mask of the PCA filter does not contain any pixel";
    m_DataMatrix.resize(0, 0);
    return;
  }

  // Set up data matrix dimensions (pixels × images)
  const unsigned long numRows = m_ValidIndices.size();
  const unsigned long numColumns = input.size();
  
  MITK_INFO << "Creating data matrix with dimensions " << numRows << " x " << numColumns;
  m_DataMatrix.resize(numRows, numColumns);
  
  // Fill data matrix - each column contains the masked pixels of one image
  unsigned int columnIndex = 0;
  for (auto it = input.begin(); it != input.end(); ++it, ++columnIndex) {
    mitkImage = dynamic_cast<mitk::Image*>(it->GetPointer());
    if (!mitkImage) {
      MITK_WARN << "Skipping invalid input image at index " << columnIndex;
      continue;
    }
    
    try {
      mitk::ImagePixelReadAccessor<m2::DisplayImagePixelType, 3> accessor(mitkImage);
      for (unsigned long row = 0; row < numRows; ++row)
        m_DataMatrix(row, columnIndex) = accessor.GetPixelByIndex(m_ValidIndices[row]);
    }
    catch (mitk::Exception& e) {
      MITK_ERROR << "Error accessing pixel data for image " << columnIndex << ": " << e.what();
    }
  }
  
  MITK_INFO << "Data matrix initialized with " << m_DataMatrix.rows() << " rows and " 
            << m_DataMatrix.cols() << " columns";
}

Eigen::MatrixXf PcaImageFilter::GetEigenImageMatrix()
{
  return m_EigenImageMatrix;
}

Eigen::VectorXf PcaImageFilter::GetMeanImage()
{
  return m_MeanImage;
}

Eigen::MatrixXf PcaImageFilter::GetLoadings()
{
  return m_Loadings;
}

void PcaImageFilter::GenerateData()
{
  auto timer = m2::Timer("PCA - GenerateData");
  
  // Initialize data matrix from input images
  this->initMatrix();
  
  if (m_DataMatrix.rows() == 0 || m_DataMatrix.cols() == 0) {
    MITK_ERROR << "Data matrix is empty, cannot perform PCA";
    return;
  }
  
  // Step 1: Center the data by subtracting the mean
  m_MeanImage = m_DataMatrix.rowwise().mean();
  Eigen::MatrixXf centeredData = m_DataMatrix.colwise() - m_MeanImage;
  
  // Step 2: Perform SVD (Singular Value Decomposition)
  MITK_INFO << "Performing SVD on matrix with dimensions " << centeredData.rows() << " x " << centeredData.cols();
  Eigen::JacobiSVD<Eigen::MatrixXf> svd(centeredData, Eigen::ComputeThinU | Eigen::ComputeThinV);
  
  // Store principal components (U matrix from SVD)
  m_EigenImageMatrix = svd.matrixU();
  
  // Calculate and store loadings (V matrix scaled by singular values)
  m_Loadings = svd.matrixV() * svd.singularValues().asDiagonal();
  
  MITK_INFO << "SVD complete: found " << svd.singularValues().size() << " components";
  MITK_INFO << "First few singular values: " 
            << svd.singularValues().head(std::min(5, (int)svd.singularValues().size())).transpose();
  
  // Limit to requested number of components
  unsigned int numComponentsToOutput = std::min(m_NumberOfComponents, 
                                              (unsigned int)m_EigenImageMatrix.cols());
  
  if (numComponentsToOutput == 0) {
    MITK_WARN << "Number of components to output is zero";
    return;
  }
  
  // Create output vector image containing the principal components
  auto eigenIonVectorImage = initializeItkVectorImage(numComponentsToOutput);
  m2::DisplayImagePixelType* outputData = eigenIonVectorImage->GetBufferPointer();
  
  // Fill the output image with principal components; the rows of the decomposition
  // are the masked pixels, all other pixels of the output stay zero
  const auto size = eigenIonVectorImage->GetLargestPossibleRegion().GetSize();
  for (unsigned int c = 0; c < numComponentsToOutput; ++c) {
    const auto& component = m_EigenImageMatrix.col(c);
    for (unsigned int p = 0; p < m_EigenImageMatrix.rows(); ++p) {
      const auto& index = m_ValidIndices[p];
      const size_t offset = index[0] + size[0] * (index[1] + size[1] * index[2]);
      outputData[offset * numComponentsToOutput + c] = component(p);
    }
  }
  
  // Convert to MITK image and set geometry from input
  mitk::Image::Pointer eigenIonImage = this->GetOutput(0);
  mitk::CastToMitkImage(eigenIonVectorImage, eigenIonImage);
  
  // Copy geometry from input image
  if (this->GetInput()) {
    eigenIonImage->SetSpacing(this->GetInput()->GetGeometry()->GetSpacing());
    eigenIonImage->SetOrigin(this->GetInput()->GetGeometry()->GetOrigin());
    eigenIonImage->GetGeometry()->SetIndexToWorldTransform(this->GetInput()->GetGeometry()->GetIndexToWorldTransform());
  }
  
  MITK_INFO << "PCA completed successfully with " << numComponentsToOutput << " components";
}

} // namespace m2
