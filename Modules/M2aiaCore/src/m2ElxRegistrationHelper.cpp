#include <m2ElxRegistrationHelper.h>
#include <m2ElxUtil.h>

bool m2::ElxRegistrationHelper::CheckDimensions(mitk::Image *fixed, mitk::Image *moving)
{
  const auto fixedDimension = fixed->GetDimension();
  const auto movingDimension = moving->GetDimension();
  const auto fixedSizeZ = fixed->GetDimensions()[2];
  const auto movingSizeZ = moving->GetDimensions()[2];
  return !(fixedDimension > 3 || movingDimension > 3 || fixedSizeZ != 1 || movingSizeZ != 1);
}

mitk::Image::Pointer m2::ElxRegistrationHelper::GetSliceData(mitk::Image *image)
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
      return {image};
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
    mitkThrow() << "Can not proceed: fixed pointset is [" << fixed << "]; moving pointset is [" << moving << "]";

  m_FixedPoints = fixed;
  m_MovingPoints = moving;
}

void m2::ElxRegistrationHelper::SetMaskData(mitk::Image *fixed, mitk::Image *moving)
{
  if (fixed == nullptr || moving == nullptr)
    mitkThrow() << "Can not proceed: fixed mask is [" << fixed << "]; moving mask is [" << moving << "]";

  if (!CheckDimensions(fixed, moving))
    mitkThrow() << "Fixed mask image [" << m2::ElxUtil::GetShape(fixed) << "] and moving mask image ["
                << m2::ElxUtil::GetShape(moving) << "]! This is yet not implemented.\n"
                << "Shapes must be [NxMx1] or [NxM]";

  m_FixedMask = GetSliceData(fixed);
  m_MovingMask = GetSliceData(moving);

  // if images were already set, check if geometries fit
  if (m_FixedImage && m_MovingImage)
  {
    if (!(mitk::Equal(*m_FixedImage->GetGeometry(), *m_FixedMask->GetGeometry()) &&
          mitk::Equal(*m_MovingImage->GetGeometry(), *m_MovingMask->GetGeometry())))
    {
      mitkThrow() << "Fixed image [" << m2::ElxUtil::GetShape(m_FixedImage) << "] and fixed mask image ["
                  << m2::ElxUtil::GetShape(m_FixedMask) << "].\n"
                  << "Moving image [" << m2::ElxUtil::GetShape(m_MovingImage) << "] and moving mask image ["
                  << m2::ElxUtil::GetShape(m_MovingMask) << "].\n"
                  << "Image geometries of mask image and image must be equal!";
    }
  }

  m_UseMasksForRegistration = true;
}

void m2::ElxRegistrationHelper::SetImageData(mitk::Image *fixed, mitk::Image *moving)
{
  if (fixed == nullptr || moving == nullptr)
    mitkThrow() << "Can not proceed: fixed is [" << fixed << "]; moving is [" << moving << "]";

  if (!CheckDimensions(fixed, moving))
    mitkThrow() << "Fixed image [" << m2::ElxUtil::GetShape(fixed) << "] and moving image ["
                << m2::ElxUtil::GetShape(moving) << "]! This is yet not implemented.\n"
                << "Shapes must be [NxMx1] or [NxM]";

  m_FixedImage = GetSliceData(fixed);
  m_MovingImage = GetSliceData(moving);

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
                  << "Image geometries of mask image and image must be equal!";
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

std::vector<std::string> m2::ElxRegistrationHelper::GetRegistration()
{
  if (m_FixedImage.IsNull() || m_MovingImage.IsNull())
  {
    MITK_ERROR << "No image set for registration!";
    return {};
  }

  const auto exeElastix = m2::ElxUtil::Executable("elastix");
  if (exeElastix.empty())
    mitkThrow() << "Elastix executable not found!";

  const auto exeTransformix = m2::ElxUtil::Executable("transformix");
  if (exeTransformix.empty())
    mitkThrow() << "Transformix executable not found!";

  // Create a temporary directory if workdir not defined
  m_WorkingDirectory = ElxUtil::JoinPath({m_WorkingDirectory});
  if (m_WorkingDirectory.empty())
    m_WorkingDirectory = mitk::IOUtil::CreateTemporaryDirectory();

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
      auto ifs = std::ifstream(targetParamterFilePath);
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
  Poco::ProcessHandle ph(Poco::Process::launch(exeElastix, args));
  ph.wait();
  MITK_INFO << exeElastix << " complete";

  std::vector<std::string> resultTransformations;

  for (unsigned int i = 0; i < m_RegistrationParameters.size(); ++i)
  {
    const auto transformationParameterFile =
      ElxUtil::JoinPath({m_WorkingDirectory, "/", "TransformParameters." + std::to_string(i) + ".txt"});
    auto ifs = std::ifstream(transformationParameterFile);
    resultTransformations.emplace_back(std::string{std::istreambuf_iterator<char>{ifs}, {}});
  }

  if (m_RemoveWorkingDirectory && itksys::SystemTools::FileIsDirectory(m_WorkingDirectory))
  {
    itksys::SystemTools::RemoveADirectory(m_WorkingDirectory);
  }

  return resultTransformations;
}

mitk::Image::Pointer m2::ElxRegistrationHelper::WarpMask(mitk::Image *)
{
  return nullptr;
}

mitk::Image::Pointer m2::ElxRegistrationHelper::WarpImage(mitk::Image *)
{
  return nullptr;
}

void m2::ElxRegistrationHelper::SetRemoveWorkingdirectory(bool val)
{
  m_RemoveWorkingDirectory = val;
}
