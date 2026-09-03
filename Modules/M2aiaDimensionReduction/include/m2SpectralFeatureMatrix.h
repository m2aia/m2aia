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
#include <itkeigen/Eigen/Dense>
#include <itkIndex.h>
#include <m2IntervalVector.h>
#include <m2SpectrumImage.h>
#include <mitkImage.h>

#include <functional>
#include <vector>

namespace m2
{
  /** Determines which pixels of an image contribute a row to the feature matrix. */
  enum class FeatureIndexSource
  {
    /** Scan the whole mask grid in x/y/z order and keep every non-zero pixel. */
    MaskScan,
    /** Walk the spectra of the image and keep those whose pixel is inside the mask. Pixels of
        the mask without a measured spectrum are skipped, so this never produces zero rows. */
    Spectra
  };

  /** Per-column scaling applied once the matrix has been filled. */
  enum class FeatureScaling
  {
    /** Leave the intensities untouched. */
    None,
    /** Subtract the column mean. */
    MeanCentered,
    /** Subtract the column mean and divide by the column standard deviation. */
    Standardized,
    /** Map each column onto [0,1]; constant columns become zero. */
    MinMax
  };

  /** The input of every dimensionality reduction: the intensities of the selected peaks over the
      pixels that take part in the computation.

      Rows are pixels in the order they were collected, columns are the peaks of the interval list.
      Several images can be stacked into one matrix; \c imageRowOffsets marks where each of them
      starts and has one entry more than there are images, so the rows of image i are
      [imageRowOffsets[i], imageRowOffsets[i+1]). */
  struct M2AIADIMENSIONREDUCTION_EXPORT SpectralFeatureMatrix
  {
    Eigen::MatrixXf data;
    /** Maps a row of \c data back to the pixel it was taken from. */
    std::vector<itk::Index<3>> indices;
    /** The label value of the mask pixel every row was taken from, which is what a supervised
        method uses as the class of that pixel. Rows collected without a mask are all one. */
    std::vector<unsigned int> classes;
    /** First row of each contributing image, plus the total number of rows as last entry. */
    std::vector<unsigned int> imageRowOffsets = {0};
    /** Grid the results are scattered back into, one per contributing image. This is the image
        itself, never the mask: a mask only has to cover the same grid, so taking the geometry
        from it could place a result somewhere else in world space. */
    std::vector<mitk::Image::Pointer> references;

    bool IsEmpty() const { return data.rows() == 0 || data.cols() == 0; }
    unsigned int GetNumberOfImages() const { return static_cast<unsigned int>(references.size()); }

    /** Number of rows contributed by the image with the given position in \c references. */
    unsigned int GetRowCount(unsigned int imageIndex) const
    {
      return imageRowOffsets.at(imageIndex + 1) - imageRowOffsets.at(imageIndex);
    }
  };

  /** Builds a m2::SpectralFeatureMatrix from spectrum images or from an existing multi-component
      image. Only one ion image is held at a time, so the memory needed does not grow with the
      number of peaks. */
  class M2AIADIMENSIONREDUCTION_EXPORT SpectralFeatureMatrixBuilder
  {
  public:
    /** Adds an image and the mask restricting it. The mask must cover the grid of the image;
        passing null uses every pixel of the image. */
    void AddImage(const m2::SpectrumImage *image, const mitk::Image *mask);

    void SetIndexSource(FeatureIndexSource source) { m_IndexSource = source; }
    void SetScaling(FeatureScaling scaling) { m_Scaling = scaling; }

    /** Shrinks the images by this factor in x and y before the pixels are collected. A factor of
        one, the default, keeps the full resolution. */
    void SetShrinkFactor(unsigned int factor) { m_ShrinkFactor = factor; }

    /** Called once per filled column with the number of columns done and the total. */
    void SetProgressCallback(std::function<void(unsigned int, unsigned int)> callback)
    {
      m_ProgressCallback = std::move(callback);
    }

    unsigned int GetNumberOfImages() const { return static_cast<unsigned int>(m_Images.size()); }

    /** One column per interval, filled with the ion image of that interval. */
    SpectralFeatureMatrix Build(const std::vector<m2::Interval> &intervals) const;

    /** One column per component of the given image, which must have the grid of the image it was
        added with. Used to run a method on the result of another one. */
    SpectralFeatureMatrix BuildFromComponentImage(const mitk::Image *componentImage) const;

    /** The pixels of the mask that take part in the computation, in the order they are collected. */
    static std::vector<itk::Index<3>> CollectValidIndices(const m2::SpectrumImage *image,
                                                          const mitk::Image *mask,
                                                          FeatureIndexSource source);

    /** The pixels that take part together with the label value each of them carries. A mask that
        was binarized has one class, a mask that kept its label values has one per label. */
    static void CollectValidPixels(const m2::SpectrumImage *image,
                                   const mitk::Image *mask,
                                   FeatureIndexSource source,
                                   std::vector<itk::Index<3>> &indices,
                                   std::vector<unsigned int> &classes);

    /** Reduces the resolution of an image in x and y; the pixel type is preserved. */
    static mitk::Image::Pointer Shrink(const mitk::Image *image, unsigned int factor);

    static void ApplyScaling(Eigen::MatrixXf &matrix, FeatureScaling scaling);

  private:
    struct Entry
    {
      const m2::SpectrumImage *image;
      mitk::Image::Pointer mask;
    };

    /** Collects the pixels of every added image and prepares the empty matrix. */
    SpectralFeatureMatrix InitializeMatrix(unsigned int numberOfColumns) const;

    std::vector<Entry> m_Images;
    FeatureIndexSource m_IndexSource = FeatureIndexSource::MaskScan;
    FeatureScaling m_Scaling = FeatureScaling::None;
    unsigned int m_ShrinkFactor = 1;
    std::function<void(unsigned int, unsigned int)> m_ProgressCallback;
  };
} // namespace m2
