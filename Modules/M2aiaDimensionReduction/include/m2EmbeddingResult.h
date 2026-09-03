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
#include <mitkImage.h>

namespace m2
{
  /** Turns the result of a m2::EmbeddingFilterBase back into image space.

      Every method produces its result as rows of a matrix, one row per pixel of the feature
      matrix. These functions write those rows into an image with the grid of the reference the
      matrix was built from; pixels that did not take part stay zero. */
  namespace EmbeddingResult
  {
    /** The rows [rowOffset, rowOffset + rowCount) of an embedding as a vector image with one
        component per column. */
    M2AIADIMENSIONREDUCTION_EXPORT mitk::Image::Pointer ToVectorImage(const Eigen::MatrixXf &embedding,
                                                                     const std::vector<itk::Index<3>> &indices,
                                                                     unsigned int rowOffset,
                                                                     unsigned int rowCount,
                                                                     const mitk::Image *reference);

    /** The rows of the image with the given position in the feature matrix as a vector image. */
    M2AIADIMENSIONREDUCTION_EXPORT mitk::Image::Pointer ToVectorImage(const Eigen::MatrixXf &embedding,
                                                                     const m2::SpectralFeatureMatrix &features,
                                                                     unsigned int imageIndex);

    /** Cluster indices as a label image. Label values start at one so that the pixels which did
        not take part keep the unlabeled value zero. */
    M2AIADIMENSIONREDUCTION_EXPORT mitk::Image::Pointer ToLabelImage(const std::vector<unsigned int> &labels,
                                                                    const std::vector<itk::Index<3>> &indices,
                                                                    unsigned int rowOffset,
                                                                    unsigned int rowCount,
                                                                    const mitk::Image *reference);

    /** The labels of the image with the given position in the feature matrix as a label image. */
    M2AIADIMENSIONREDUCTION_EXPORT mitk::Image::Pointer ToLabelImage(const std::vector<unsigned int> &labels,
                                                                    const m2::SpectralFeatureMatrix &features,
                                                                    unsigned int imageIndex);

    /** Rescales every column of the given rows onto [0,255], the range the RGB conversion of
        m2::MultiSliceFilter expects. */
    M2AIADIMENSIONREDUCTION_EXPORT void RescaleToByteRange(Eigen::MatrixXf &embedding,
                                                           unsigned int rowOffset,
                                                           unsigned int rowCount);
  } // namespace EmbeddingResult
} // namespace m2
