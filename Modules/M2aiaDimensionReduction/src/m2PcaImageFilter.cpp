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
#include <m2TruncatedSvd.h>

#include <algorithm>

void m2::PcaImageFilter::ComputeEmbedding()
{
  const auto &data = m_Features.data;
  MITK_INFO << "PCA on a matrix of " << data.rows() << " pixels and " << data.cols() << " peaks";
  ReportProgress(0, 2);

  // every pixel is centred by its own mean over the peaks
  m_MeanImage = data.rowwise().mean();
  const Eigen::MatrixXf centeredData = data.colwise() - m_MeanImage;

  const auto decomposition = m2::TruncatedSvd::Compute(centeredData, m_NumberOfComponents, m_ExactDecomposition);
  ReportProgress(1, 2);

  if (decomposition.GetRank() == 0)
  {
    MITK_WARN << "The decomposition did not yield a single component.";
    return;
  }

  m_Embedding = decomposition.U;
  m_SingularValues = decomposition.S;
  // rows are the components, columns the peaks, as everywhere else
  m_Loadings = (decomposition.V * decomposition.S.asDiagonal()).transpose();

  MITK_INFO << "PCA kept " << decomposition.GetRank() << " components; singular values "
            << decomposition.S.transpose();
  ReportProgress(2, 2);
}
