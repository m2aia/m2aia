#include <m2ElxRegistrationHelper.h>
#include <m2ElxUtil.h>
#include <mitkImage2DToImage3DSliceFilter.h>
#include <mitkImage3DSliceToImage2DFilter.h>

m2::ElxRegistrationHelper::~ElxRegistrationHelper()
{
  RemoveWorkingDirectory();
}

bool m2::ElxRegistrationHelper::CheckDimensions(const mitk::Image *image)
{
  const auto dims = image->GetDimension();
  const auto sizeZ = image->GetDimensions()[2];
  return dims == 3 && sizeZ == 1;
}

mitk::Image::Pointer m2::ElxRegistrationHelper::GetSlice2DData(const mitk::Image *image)
{
  if (image)
  {
    if (image->GetDimension() == 3)
    {
      auto filter = mitk::Image3DSliceToImage2DFilter::New();
      filter->SetInput(image);
      filter->Update();
      return {filter->GetOutput()};
    }
    else
    {
      return const_cast<mitk::Image *>(image);
    }
  }
  else
  {
    mitkThrow() << "Image data is null!";
  }
}

mitk::Image::Pointer m2::ElxRegistrationHelper::GetSlice3DData(const mitk::Image *image)
{
  if (image)
  {
    if (image->GetDimension() == 2)
    {
      auto filter = mitk::Image2DToImage3DSliceFilter::New();
      filter->SetInput(image);
      filter->Update();
      return {filter->GetOutput()};
    }
    else
    {
      return const_cast<mitk::Image *>(image);
    }
  }
  else
  {
    mitkThrow() << "Image data is null!";
  }
}

void m2::ElxRegistrationHelper::SetPointData(mitk::PointSet *fixed, mitk::PointSet *moving)
{
  if (fixed == nullptr || moving == nullptr)
  {
    MITK_WARN << "Fixed pointset is [" << fixed << "]; moving pointset is [" << moving << "]";
    MITK_WARN << "No pointsets are used.";
    m_UsePointsForRegistration = false;
  }
  else
  {
    m_FixedPoints = fixed;
    m_MovingPoints = moving;
    m_UsePointsForRegistration = true;
  }
}

void m2::ElxRegistrationHelper::SetMaskData(mitk::Image *fixed, mitk::Image *moving)
{
  if (fixed == nullptr || moving == nullptr)
  {
    MITK_WARN << "Can not proceed: fixed mask is [" << fixed << "]; moving mask is [" << moving << "]";
    return;
  }

  if (!CheckDimensions(fixed) && !CheckDimensions(moving))
  {
    MITK_ERROR << "Fixed mask image [" << m2::ElxUtil::GetShape(fixed) << "] and moving mask image ["
               << m2::ElxUtil::GetShape(moving) << "]! This is yet not implemented.\n"
               << "Shape has to be [NxMx1]";
    return;
  }

  m_FixedMask = GetSlice2DData(fixed);
  m_MovingMask = GetSlice2DData(moving);

  // if images were already set, check if geometries fit
  if (m_FixedImage && m_MovingImage)
  {
    if (!(mitk::Equal(*m_FixedImage->GetGeometry(), *m_FixedMask->GetGeometry()) &&
          mitk::Equal(*m_MovingImage->GetGeometry(), *m_MovingMask->GetGeometry())))
    {
      MITK_ERROR << "Fixed image [" << m2::ElxUtil::GetShape(m_FixedImage) << "] and fixed mask image ["
                 << m2::ElxUtil::GetShape(m_FixedMask) << "].\n"
                 << "Moving image [" << m2::ElxUtil::GetShape(m_MovingImage) << "] and moving mask image ["
                 << m2::ElxUtil::GetShape(m_MovingMask) << "].\n"
                 << "Image geometries of mask image and image have to be equal!";
    }
  }

  m_UseMasksForRegistration = true;
}

void m2::ElxRegistrationHelper::SetImageData(mitk::Image *fixed, mitk::Image *moving)
{
  if (fixed == nullptr || moving == nullptr)
    mitkThrow() << "Can not proceed: fixed is [" << fixed << "]; moving is [" << moving << "]";

  if (!CheckDimensions(fixed) && !CheckDimensions(moving))
    mitkThrow() << "Fixed image [" << m2::ElxUtil::GetShape(fixed) << "] and moving image ["
                << m2::ElxUtil::GetShape(moving) << "]! This is yet not implemented.\n"
                << "Shape has to be [NxMx1]";

  m_FixedImage = GetSlice2DData(fixed);
  m_MovingImage = GetSlice2DData(moving);

  // if masks were already set, check if geometries fit
  if (m_UseMasksForRegistration)
  {
    if (!(mitk::Equal(*m_FixedImage->GetGeometry(), *m_FixedMask->GetGeometry()) &&
          mitk::Equal(*m_MovingImage->GetGeometry(), *m_MovingMask->GetGeometry())))
    {
      mitkThrow() << "Fixed image [" << m2::ElxUtil::GetShape(m_FixedImage) << "] and fixed mask image ["
                  << m2::ElxUtil::GetShape(m_FixedMask) << "].\n"
                  << "Moving image [" << m2::ElxUtil::GetShape(m_MovingImage) << "] and moving mask image ["
                  << m2::ElxUtil::GetShape(m_MovingMask) << "].\n"
                  << "Image geometries of mask image and image have to be equal!";
    }
  }
}

void m2::ElxRegistrationHelper::SetDirectory(const std::string &dir)
{
  m_WorkingDirectory = dir;
}

void m2::ElxRegistrationHelper::SetRegistrationParameters(const std::vector<std::string> &params)
{
  m_RegistrationParameters = params;
}

void m2::ElxRegistrationHelper::SetAdditionalBinarySearchPath(const std::string &path)
{
  m_BinarySearchPath = ElxUtil::JoinPath({path});
}

void m2::ElxRegistrationHelper::CreateWorkingDirectory()
{
  // Create a temporary directory if workdir not defined
  m_WorkingDirectory = ElxUtil::JoinPath({m_WorkingDirectory});

  if (m_WorkingDirectory.empty())
    m_WorkingDirectory = mitk::IOUtil::CreateTemporaryDirectory();

  if (!itksys::SystemTools::PathExists(m_WorkingDirectory))
    itksys::SystemTools::MakeDirectory(m_WorkingDirectory);
}

void m2::ElxRegistrationHelper::GetRegistration()
{
  if (m_FixedImage.IsNull() || m_MovingImage.IsNull())
  {
    MITK_ERROR << "No image set for registration!";
    return;
  }

  const auto exeElastix = m2::ElxUtil::Executable("elastix", m_BinarySearchPath);
  if (exeElastix.empty())
    mitkThrow() << "Elastix executable not found!";

  CreateWorkingDirectory();

  if(m_RegistrationParameters.empty())
    m_RegistrationParameters.push_back(DEFAULT_PARAMETER_FILE);

  // Write parameter files
  for (unsigned int i = 0; i < m_RegistrationParameters.size(); ++i)
  {
    const auto targetParamterFilePath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "pp" + std::to_string(i) + ".txt"});
    const auto element = m_RegistrationParameters[i];
    std::string parameterText;
    if (itksys::SystemTools::FileExists(element) && !itksys::SystemTools::FileIsDirectory(element))
    {
      const auto extension = itksys::SystemTools::GetFilenameExtension(element);
      // TODO: additional checks if p is a real path to an elastix parameter file
      auto ifs = std::ifstream(element);
      parameterText = std::string(std::istreambuf_iterator<char>{ifs}, {});
    }
    else
    {
      parameterText = element;
    }

    // add PointsEuclidianDistance metric
    if (m_UsePointsForRegistration)
    {
      if (parameterText.find("MultiResolution") != std::string::npos)
        ElxUtil::ReplaceParameter(parameterText, "Registration", "\"MultiMetricMultiResolutionRegistration\"");
      else
        ElxUtil::ReplaceParameter(parameterText, "Registration", "\"MultiMetricRegistration\"");
      ElxUtil::ReplaceParameter(
        parameterText, "Metric", "\"AdvancedMattesMutualInformation\" \"CorrespondingPointsEuclideanDistanceMetric\"");
    }
    // Write the parameter file to working directory
    std::ofstream outStream(targetParamterFilePath);
    outStream << parameterText;
    outStream.close();
  }

  const auto movingResultPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "result.0.nrrd"});
  const auto movingMaskResultPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "result.nrrd"});

  const auto fixedPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "fixed.nrrd"});
  const auto movingPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "moving.nrrd"});
  mitk::IOUtil::Save(m_MovingImage, movingPath);
  mitk::IOUtil::Save(m_FixedImage, fixedPath);

  // START THE REGISTRATION
  Poco::Process::Args args;
  args.insert(args.end(), {"-f", fixedPath});
  args.insert(args.end(), {"-m", movingPath});
  args.insert(args.end(), {"-out", m_WorkingDirectory});
  if (m_UseMasksForRegistration)
  {
    const auto fixedMaskPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "fixedMask.nrrd"});
    const auto movingMaskPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "movingMask.nrrd"});
    mitk::IOUtil::Save(m_MovingMask, movingMaskPath);
    mitk::IOUtil::Save(m_FixedMask, fixedMaskPath);
    args.insert(args.end(), {"-fMask", fixedMaskPath});
    args.insert(args.end(), {"-mMask", movingMaskPath});
  }

  if (m_UsePointsForRegistration)
  {
    const auto fixedPointsPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "fixedPoints.nrrd"});
    const auto movingPointsPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "movingPoints.nrrd"});
    ElxUtil::SavePointSet(m_MovingPoints, movingPointsPath);
    ElxUtil::SavePointSet(m_FixedPoints, fixedPointsPath);
    args.insert(args.end(), {"-mp", movingPointsPath});
    args.insert(args.end(), {"-fp", fixedPointsPath});
  }

  for (unsigned int i = 0; i < m_RegistrationParameters.size(); ++i)
  {
    const auto parameterFile = ElxUtil::JoinPath({m_WorkingDirectory, "/", "pp" + std::to_string(i) + ".txt"});
    args.insert(args.end(), {"-p", parameterFile});
  }

  MITK_INFO << "Start " << exeElastix << " ...";
  Poco::Pipe oPipe;
  Poco::ProcessHandle ph(Poco::Process::launch(exeElastix, args, nullptr, &oPipe, nullptr));
  oPipe.close();
  ph.wait();
  MITK_INFO << exeElastix << " complete";

  for (unsigned int i = 0; i < m_RegistrationParameters.size(); ++i)
  {
    const auto transformationParameterFile =
      ElxUtil::JoinPath({m_WorkingDirectory, "/", "TransformParameters." + std::to_string(i) + ".txt"});
    auto ifs = std::ifstream(transformationParameterFile);
    m_Transformations.emplace_back(std::string{std::istreambuf_iterator<char>{ifs}, {}});
  }

  RemoveWorkingDirectory();
}

std::vector<std::string> m2::ElxRegistrationHelper::GetTransformation()
{
  return m_Transformations;
}

mitk::Image::Pointer m2::ElxRegistrationHelper::WarpImage(const mitk::Image *data,
                                                          const std::string &pixelType,
                                                          const unsigned char &interpolationOrder)
{
  const auto exeTransformix = m2::ElxUtil::Executable("transformix", m_BinarySearchPath);
  if (exeTransformix.empty())
    mitkThrow() << "Transformix executable not found!";

  CreateWorkingDirectory();

  if (!CheckDimensions(data))
  {
    MITK_ERROR << "Image [" << m2::ElxUtil::GetShape(data) << "]. This is yet not implemented.\n"
               << "Shape has to be [NxMx1]";
    return nullptr;
  }

  const auto imagePath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "data.nrrd"});
  const auto resultPath = ElxUtil::JoinPath({m_WorkingDirectory, "/", "result.nrrd"});

  mitk::IOUtil::Save(GetSlice2DData(data), imagePath);

  // write all transformations
  for (unsigned int i = 0; i < m_Transformations.size(); ++i)
  {
    auto T = m_Transformations[i];
    auto transformationPath =
      ElxUtil::JoinPath({m_WorkingDirectory, "/", "TransformParameters." + std::to_string(i) + ".txt"});
    ElxUtil::ReplaceParameter(T, "ResultImagePixelType", "\"" + pixelType + "\"");
    ElxUtil::ReplaceParameter(T, "ResampleInterpolator", "\"FinalBSplineInterpolatorFloat\"");
    ElxUtil::ReplaceParameter(T, "FinalBSplineInterpolationOrder", std::to_string(interpolationOrder));
    std::ofstream(transformationPath) << T;
  }

  const auto transformationPath =
    ElxUtil::JoinPath({m_WorkingDirectory, "/", "TransformParameters." + std::to_string(m_Transformations.size() - 1) + ".txt"});

  Poco::Process::Args args;
  args.insert(args.end(), {"-in", imagePath});
  args.insert(args.end(), {"-tp", transformationPath});
  args.insert(args.end(), {"-out", m_WorkingDirectory});

  MITK_INFO << "Start " << exeTransformix << " ...";
  Poco::Pipe oPipe;
  Poco::ProcessHandle ph(Poco::Process::launch(exeTransformix, args, nullptr, &oPipe, nullptr));
  oPipe.close();
  ph.wait();
  MITK_INFO << exeTransformix << " complete";

  auto resultData = mitk::IOUtil::Load(resultPath).front();
  mitk::Image::Pointer resultImage2D = dynamic_cast<mitk::Image *>(resultData.GetPointer());

  RemoveWorkingDirectory();
  mitk::Image::Pointer resultSlice = GetSlice3DData(resultImage2D);
  auto s = resultSlice->GetGeometry()->GetSpacing();
  s[2] = data->GetGeometry()->GetSpacing()[2];
  resultSlice->GetGeometry()->SetSpacing(s);
  return resultSlice;
}

void m2::ElxRegistrationHelper::SetRemoveWorkingdirectory(bool val)
{
  m_RemoveWorkingDirectory = val;
}

void m2::ElxRegistrationHelper::RemoveWorkingDirectory()
{
  try
  {
    if (m_RemoveWorkingDirectory && itksys::SystemTools::PathExists(m_WorkingDirectory) &&
        itksys::SystemTools::FileIsDirectory(m_WorkingDirectory))
    {
      itksys::SystemTools::RemoveADirectory(m_WorkingDirectory);
    }
  }
  catch (std::exception &e)
  {
    MITK_ERROR << "Cleanup ElxRegistrationHelper fails!\n" << e.what();
  }
}

const std::string m2::ElxRegistrationHelper::DEFAULT_PARAMETER_FILE = R"(
//ImageTypes
(FixedInternalImagePixelType "float")
(MovingInternalImagePixelType "float")
(UseDirectionCosines "true")
//Components
(Registration "MultiResolutionRegistration")
(FixedImagePyramid "FixedRecursiveImagePyramid")
(MovingImagePyramid "MovingRecursiveImagePyramid")
(Interpolator "BSplineInterpolator")
(Metric "AdvancedMattesMutualInformation")
(Optimizer "AdaptiveStochasticGradientDescent")
(ResampleInterpolator "FinalBSplineInterpolator")
(Resampler "DefaultResampler")
(Transform "EulerTransform")
// ********** Pyramid
// Total number of resolutions
(NumberOfResolutions 3)
// ********** Transform
//(CenterOfRotation 128 128) center by default
(AutomaticTransformInitialization "true")
(AutomaticScalesEstimation "true")
(HowToCombineTransforms "Compose")
// ********** Optimizer
// Maximum number of iterations in each resolution level:
(MaximumNumberOfIterations 300 300 600)
(AutomaticParameterEstimation "true")
(UseAdaptiveStepSizes "true")
// ********** Metric
//Number of grey level bins in each resolution level:
(NumberOfHistogramBins 32)
(FixedKernelBSplineOrder 1)
(MovingKernelBSplineOrder 3)
// ********** Several
(WriteTransformParametersEachIteration "false")
(WriteTransformParametersEachResolution "false")
(ShowExactMetricValue "false")
(ErodeMask "true")
// ********** ImageSampler
// Number of spatial samples used to compute the
// mutual information in each resolution level:
(ImageSampler "RandomCoordinate")
(NumberOfSpatialSamples 2048)
(NewSamplesEveryIteration "true")
// ********** Interpolator and Resampler
//Order of B-Spline interpolation used in each resolution level:
(BSplineInterpolationOrder 1)
//Order of B-Spline interpolation used for applying the final deformation:
(FinalBSplineInterpolationOrder 3)
//Default pixel value for pixels that come from outside the picture:
(DefaultPixelValue 0)

// The pixel type and format of the resulting deformed moving image
(WriteResultImage "true")
(ResultImagePixelType "float")
(ResultImageFormat "nrrd")

)";