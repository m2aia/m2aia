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
#include <string>
namespace m2
{
  namespace Elx
  {
    const std::string Rigid()
    {
      return R"(
(FixedInternalImagePixelType "float")
(MovingInternalImagePixelType "float")

(FixedImageDimension 2)
(MovingImageDimension 2)

(UseDirectionCosines "true")

// **************** Main Components **************************
// The following components should usually be left as they are:
(Registration "MultiResolutionRegistration")
(Interpolator "LinearInterpolator")
(ResampleInterpolator "FinalLinearInterpolator")
(Resampler "DefaultResampler")

(FixedImagePyramid "FixedRecursiveImagePyramid")
(MovingImagePyramid "MovingRecursiveImagePyramid")

(Optimizer "AdaptiveStochasticGradientDescent")
(Metric "AdvancedMattesMutualInformation")
(Transform "EulerTransform")

// ***************** Transformation **************************

(AutomaticScalesEstimation "true")
(AutomaticTransformInitialization "true")
(AutomaticTransformInitializationMethod "CenterOfGravity")
(HowToCombineTransforms "Compose")

// ******************* Similarity measure *********************

(NumberOfHistogramBins 32)

// If you use a mask, this option is important.
// If the mask serves as region of interest, set it to false.
// If the mask indicates which pixels are valid, then set it to true.
// If you do not use a mask, the option doesn't matter.
(ErodeMask "true")

// ******************** Multiresolution **********************

(NumberOfResolutions 3)

// ******************* Optimizer ****************************

(MaximumNumberOfIterations 300)

// The step size of the optimizer, in mm. By default the voxel size is used.
// which usually works well. In case of unusual high-resolution images
// (eg histology) it is necessary to increase this value a bit, to the size
// of the "smallest visible structure" in the image:
// (MaximumStepLength 5.0 3.0 1.0)

// **************** Image sampling **********************

(NumberOfSpatialSamples 25000)
(NewSamplesEveryIteration "true")
(ImageSampler "Random")

// ************* Interpolation and Resampling ****************

// Order of B-Spline interpolation used for applying the final
// deformation.
// 3 gives good accuracy; recommended in most cases.
// 1 gives worse accuracy (linear interpolation)
// 0 gives worst accuracy, but is appropriate for binary images
// (masks, segmentations); equivalent to nearest neighbor interpolation.
(BSplineInterpolationOrder 3)
(FinalBSplineInterpolationOrder 3)

//Default pixel value for pixels that come from outside the picture:
(DefaultPixelValue 0)

(WriteTransformParametersEachResolution "false")

// ************* Required by M2aia *************
// The following components should usually be left as they are:
(WriteResultImage "false")
(ResultImagePixelType "float")
(ResultImageFormat "nrrd")
)";
    }

    const std::string Deformable()
    {
      return R"(
// Example parameter file for B-spline registration
// C-style comments: //

(FixedInternalImagePixelType "float")
(MovingInternalImagePixelType "float")

(FixedImageDimension 2)
(MovingImageDimension 2)

(UseDirectionCosines "true")

// **************** Main Components **************************

// The following components should usually be left as they are:
(Registration "MultiResolutionRegistration")
(ResampleInterpolator "FinalBSplineInterpolator")
(Resampler "DefaultResampler")

// These may be changed to Fixed/MovingSmoothingImagePyramid.
// See the manual.
// (FixedImagePyramid "FixedRecursiveImagePyramid")
// (MovingImagePyramid "MovingRecursiveImagePyramid")

(Optimizer "AdaptiveStochasticGradientDescent")
(Transform "RecursiveBSplineTransform")
(Metric "AdvancedMattesMutualInformation")
(Interpolator "BSplineInterpolator")
(Metric0Weight 1.00)
(Metric1Weight 0.80)

// ******************** Multiresolution **********************

(NumberOfResolutions 4)

// ***************** Transformation **************************

// The control point spacing of the bspline transformation in
// the finest resolution level. Can be specified for each
// dimension differently. Unit: mm.
(FinalGridSpacingInPhysicalUnits 0.35)

// Alternatively, the grid spacing can be specified in voxel units.
// To do that, uncomment the following line and comment/remove
// the FinalGridSpacingInPhysicalUnits definition.
// (FinalGridSpacingInVoxels 15.0 15.0)
(GridSpacingSchedule 8.0 8.0 3.0 3.0 2.5 2.5 1.0 1.0)

// By default the grid spacing is halved after every resolution,
// such that the final grid spacing is obtained in the last
// resolution level. You can also specify your own schedule,
// if you uncomment the following line:
// (GridSpacingSchedule 4.0 4.0 2.0 1.0)
// This setting can also be supplied per dimension.

(HowToCombineTransforms "Compose")

// ******************* Similarity measure *********************

(NumberOfHistogramBins 32)

// If you use a mask, this option is important.
// If the mask serves as region of interest, set it to false.
// If the mask indicates which pixels are valid, then set it to true.
// If you do not use a mask, the option doesn't matter.
(ErodeMask "true")




// ******************* Optimizer ****************************

(MaximumNumberOfIterations 750)


// The step size of the optimizer, in mm. By default the voxel size is used.
// which usually works well. In case of unusual high-resolution images
// (eg histology) it is necessary to increase this value a bit, to the size
// of the "smallest visible structure" in the image:
// (MaximumStepLength 100 80 70 60 50 10 10 1 1 1)

// **************** Image sampling **********************

(NumberOfSpatialSamples 25000)

(NewSamplesEveryIteration "true")
(ImageSampler "Random")


// ************* Interpolation and Resampling ****************


(BSplineInterpolationOrder 3)
//(WriteTransformParametersEachResolution "true")
//(WriteResultImageAfterEachResolution "true")

// Order of B-Spline interpolation used for applying the final
// deformation.
// 3 gives good accuracy; recommended in most cases.
// 1 gives worse accuracy (linear interpolation)
// 0 gives worst accuracy, but is appropriate for binary images
// (masks, segmentations); equivalent to nearest neighbor interpolation.
(FinalBSplineInterpolationOrder 3)

//Default pixel value for pixels that come from outside the picture:
(DefaultPixelValue 0)

// Choose whether to generate the deformed moving image.
// You can save some time by setting this to false, if you are
// not interested in the final deformed moving image, but only
// want to analyze the deformation field for example.
(WriteTransformParametersEachResolution "false")

// The pixel type and format of the resulting deformed moving image
(WriteResultImage "false")
(ResultImagePixelType "float")
(ResultImageFormat "nrrd")
)";
    }
  } // namespace Elx

} // namespace m2