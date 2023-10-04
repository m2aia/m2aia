/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2SpectrumImageHelper.h>
#include <mitkImagePixelReadAccessor.h>
#include <m2CoreCommon.h>

#include <mitkCoreServices.h>
#include <mitkIPreferencesService.h>
#include <mitkIPreferences.h>

void m2::SpectrumImageHelper::AddArguments(mitk::DockerHelper & helper){
  auto* preferencesService = mitk::CoreServices::GetPreferencesService();
	auto preferences = preferencesService->GetSystemPreferences();

  auto norm = m2::NormalizationStrategyTypeNames[preferences->GetInt("m2aia.signal.NormalizationStrategy", to_underlying(m2::NormalizationStrategyType::None))];
  auto tran = m2::IntensityTransformationTypeNames[preferences->GetInt("m2aia.signal.IntensityTransformationStrategy", to_underlying(m2::IntensityTransformationType::None))];
  auto pool = m2::RangePoolingStrategyTypeNames[preferences->GetInt("m2aia.signal.RangePoolingStrategy", to_underlying(m2::RangePoolingStrategyType::Mean))];
  auto smoo = m2::SmoothingTypeNames[preferences->GetInt("m2aia.signal.SmoothingStrategy", to_underlying(m2::SmoothingType::None))];
  auto base = m2::BaselineCorrectionTypeNames[preferences->GetInt("m2aia.signal.BaselineCorrectionStrategy", to_underlying(m2::BaselineCorrectionType::None))];
  auto smoo_val = preferences->Get("m2aia.signal.SmoothingValue", "2");
  auto base_val = preferences->Get("m2aia.signal.BaselineCorrectionValue", "50");
  auto tol = preferences->Get("m2aia.signal.Tolerance", "75.0");
  
  helper.AddApplicationArgument("--normalization", norm);
  helper.AddApplicationArgument("--intensity_transform", tran);
  helper.AddApplicationArgument("--range_pooling", pool);
  helper.AddApplicationArgument("--smoothing", smoo);
  helper.AddApplicationArgument("--smoothing_value", smoo_val);
  helper.AddApplicationArgument("--baseline_correction", base);
  helper.AddApplicationArgument("--baseline_correction_value", base_val);
  helper.AddApplicationArgument("--tolerance", tol);

// python ArgumentParser
// =========================================================
// parser.add_argument("--normalization", default="None")
// parser.add_argument("--intensity_transform", default="None" )
// parser.add_argument("--range_pooling", default="Mean")
// parser.add_argument("--smoothing", default="None")
// parser.add_argument("--smoothing_value", default=2, type=np.int32)
// parser.add_argument("--baseline_correction", default="None")
// parser.add_argument("--baseline_correction_value", default=50, type=np.int32)
// parser.add_argument("--tolerance", default=75, type=np.float32)
// =========================================================
// args = parser.parse_args()
// =========================================================
// I = m2.ImzMLReader(args.imzml)
// I.SetSmoothing(args.smoothing, args.smoothing_value)
// I.SetBaselineCorrection(args.baseline_correction, args.baseline_correction_value)
// I.SetNormalization(args.normalization)
// I.SetPooling(args.range_pooling)
// I.SetIntensityTransformation(args.intensity_transform)
// I.SetTolerance(args.tolerance)
// I.Execute()


}

std::vector<unsigned long> m2::SpectrumImageHelper::GetIntensityDataShape(
  const m2::SpectrumImage * image,
  const std::vector<m2::Interval> &intervals)
{
  using namespace std;
  // pixels in image
  unsigned int N = accumulate(image->GetDimensions(), image->GetDimensions() + 3, 1, multiplies<unsigned int>());
  std::vector<unsigned long> v;
  v.push_back(intervals.size());
  v.push_back(N);
  return v;
}

std::vector<float> m2::SpectrumImageHelper::GetIntensityData(
  const m2::SpectrumImage * image,
  const std::vector<m2::Interval> &intervals)
{
  using namespace std;
  // pixels in image
  unsigned int N = accumulate(image->GetDimensions(), image->GetDimensions() + 3, 1, multiplies<unsigned int>());
  auto maskImage = image->GetMaskImage();

  auto tmpImage = mitk::Image::New();
  tmpImage->Initialize(image);
  mitk::ImagePixelReadAccessor<m2::DisplayImagePixelType, 3> tAcc(tmpImage);

  vector<float> values;
  values.reserve(N * intervals.size());
  auto inserter = back_inserter(values);
  MITK_INFO << "Generate intensity values for #intervals (" << intervals.size()
            << ") using interval centers and a tolerance of " << image->GetTolerance()
            << " isUsingPPM=" << (image->GetUseToleranceInPPM() ? "True" : "False");
  for (const auto &p : intervals)
  {
    image->GetImage(p.x.mean(), image->ApplyTolerance(p.x.mean()), maskImage, tmpImage);
    copy(tAcc.GetData(), tAcc.GetData() + N, inserter);
  }
  return values;
}