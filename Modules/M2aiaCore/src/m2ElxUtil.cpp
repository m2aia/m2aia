/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#include <m2ElxConfig.h>
#include <m2ElxUtil.h>
#include <mitkException.h>

std::string m2::ElxUtil::Executable(const std::string &name, std::string additionalSearchPath)
{
  std::string elxPath, elxPathExe = "";
  const std::string version = "5";

  if (additionalSearchPath.empty())
  {
    additionalSearchPath = ELASTIX_DIR;
    // this works only during development
  }

  std::string correctedName = itksys::SystemTools::GetFilenameWithoutExtension(name);
#ifdef __WIN32__
  correctedName += ".exe";
#endif

  const auto regex = std::regex{correctedName + "[a-z:\\s]+" + version + "\\.[0-9]+"};

  { // check additionalSearchPath
    if (!itksys::SystemTools::FileIsDirectory(additionalSearchPath))
      elxPath = itksys::SystemTools::GetParentDirectory(additionalSearchPath);
    else
      elxPath = additionalSearchPath;
    elxPathExe = m2::ElxUtil::JoinPath({elxPath, "/", correctedName});
    if (m2::ElxUtil::CheckVersion(elxPathExe, regex))
    {
      MITK_INFO << "Use Elastix found at [" << elxPath << "]";
      return elxPathExe;
    }
  }

  { // check system variable ELASTIX_PATH
    itksys::SystemTools::GetEnv("ELASTIX_PATH", elxPath);
    elxPathExe = m2::ElxUtil::JoinPath({elxPath, "/", correctedName});
    if (m2::ElxUtil::CheckVersion(elxPathExe, regex))
    {
      MITK_INFO << "Use Elastix found at [" << elxPath << "]";
      return elxPathExe;
    }
  }

  { // check PATH
    if (m2::ElxUtil::CheckVersion(correctedName, regex))
    {
      itksys::SystemTools::GetEnv("PATH", elxPath);
      MITK_INFO << "Use system Elastix found in [" << elxPath << "]";
      elxPathExe = correctedName;
      return elxPathExe;
    }
    else
    {
      MITK_ERROR << "Elastix executables could not be found!\n"
                    "Please specify the system variable ELASTIX_PATH";
    }
  }

  return "";
}

bool m2::ElxUtil::CheckVersion(const std::string &executablePath,
                               const std::regex &version_regex,
                               const std::string &argVersion)
{
  Poco::Process::Args args;
  args.push_back(argVersion);
  Poco::Pipe outPipe;
  Poco::PipeInputStream istr(outPipe);
  Poco::ProcessHandle ph(Poco::Process::launch(executablePath, args, nullptr, &outPipe, nullptr));
  ph.wait();
  std::stringstream ss;
  Poco::StreamCopier::copyStream(istr, ss);
  auto pass = std::regex_search(ss.str(), version_regex);
  if (pass)
    MITK_INFO << ss.str();
  return pass;
}

/**
 * @brief CreatePath can be used to normalize and join path arguments
 * Example call:
 * CreatPath({"this/is/a/directory/path/with/or/without/a/slash/","/","test"});
 * @param args is a vector of strings
 * @return std::string
 */
std::string m2::ElxUtil::JoinPath(std::vector<std::string> &&args)
{
  return itksys::SystemTools::ConvertToOutputPath(itksys::SystemTools::JoinPath(args));
}

mitk::Image::Pointer m2::ElxUtil::WarpImage(const mitk::Image *image,
                                            const std::vector<std::string> &transformations,
                                            const std::string &processDir,
                                            bool removeProcessDir,
                                            std::function<void(std::string &)> transformationModifier)
{
  if (transformations.empty())
    return const_cast<mitk::Image *>(image);

  auto executablePath = m2::ElxUtil::Executable("transformix");

  using itkSys = itksys::SystemTools;

  mitk::Image::Pointer outImage;
  mitk::ImageToImageFilter::Pointer filter;
  std::string workdir = processDir;
  if (processDir.empty())
    workdir = mitk::IOUtil::CreateTemporaryDirectory();

  std::string workdirPath = JoinPath({workdir});
  const auto parameterFilePath = JoinPath({workdirPath, "/", "Transform.txt"});
  const auto movingImagefilePath = JoinPath({workdirPath, "/", "moving.nrrd"});
  const auto inputImagefilePath = JoinPath({workdirPath, "/", "input.nrrd"});
  const auto resultImagefilePath = JoinPath({workdirPath, "/", "result.nrrd"});

  if (image != nullptr)
  {
    auto Filter3DToSlice = mitk::Image3DSliceToImage2DFilter::New();
    Filter3DToSlice->SetInput(image);
    Filter3DToSlice->Update();
    mitk::IOUtil::Save(Filter3DToSlice->GetOutput(), movingImagefilePath);
    itkSys::CopyFileAlways(movingImagefilePath, inputImagefilePath);

    for (std::string trafo : transformations)
    {
      // apply the transformation file changes e.g. pixel type output, interpolation mode etc..
      transformationModifier(trafo);

      std::ofstream(parameterFilePath) << trafo;

      Poco::Process::Args args = {"-in", movingImagefilePath, "-out", workdirPath, "-tp", parameterFilePath};
      auto handle = Poco::Process::launch(executablePath, args);
      auto err = handle.wait();
      if (err)
        MITK_ERROR << "Some error (" << err << ") on Poco::Process::launch call of" << executablePath;

      // result image is the new moving image for the next iteration
      itkSys::CopyFileAlways(resultImagefilePath, movingImagefilePath);
      break;
    }

    auto res = mitk::IOUtil::Load(movingImagefilePath);
    outImage = dynamic_cast<mitk::Image *>(res[0].GetPointer());

    auto FilterSliceTo3D = mitk::Image2DToImage3DSliceFilter::New();
    FilterSliceTo3D->SetInput(image);
    FilterSliceTo3D->Update();
    outImage = FilterSliceTo3D->GetOutput();
  }
  if (removeProcessDir)
  {
    itkSys::RemoveADirectory(workdirPath);
  }
  else
  {
    MITK_INFO << "Warp-Image processing dir: " << workdirPath;
  }
  return outImage;
}

std::vector<std::string> m2::ElxUtil::GetRegistration(mitk::Image::Pointer inputFixed,
                                                      mitk::Image::Pointer inputMoving,
                                                      const std::vector<std::string> &parameterFilePathOrStrings,
                                                      const std::string &userDefinedProcessingPath,
                                                      bool /*removeProcessDir*/,
                                                      std::function<void(std::string &)> /*transformationModifier*/)
{
  if (!inputMoving && !inputFixed)
    mitkThrow() << "FixedImage[" << inputFixed.IsNotNull() << "] MovingImage[" << inputMoving.IsNotNull() << "]";
  // check input dimensions (must 2D)
  const auto inputDimensionFixed = inputFixed->GetDimension();
  const auto inputDimensionMoving = inputMoving->GetDimension();
  const auto fixedSizeZ = inputFixed->GetDimensions()[2];
  const auto movingSizeZ = inputMoving->GetDimensions()[2];
  mitk::Image::ConstPointer fixed2D, moving2D;

  if (inputDimensionFixed > 3 || inputDimensionMoving > 3 || fixedSizeZ != 1 || movingSizeZ != 1)
  {
    mitkThrow() << "Fixed image [" << GetShape(inputFixed) << "] and moving image [" << GetShape(inputMoving)
                << "]! This is yet not implemented. Shapes must be [NxMx1] or [NxM]";
  }
  else if (inputDimensionFixed == 2 || inputDimensionMoving == 2)
  {
    fixed2D = inputFixed;
    moving2D = inputMoving;
  }
  else
  {
    {
      auto filter = mitk::Image3DSliceToImage2DFilter::New();
      filter->SetInput(inputFixed);
      filter->Update();
      fixed2D = filter->GetOutput();
    }
    {
      auto filter = mitk::Image3DSliceToImage2DFilter::New();
      filter->SetInput(inputMoving);
      filter->Update();
      moving2D = filter->GetOutput();
    }
  }

  auto exeElastix = m2::ElxUtil::Executable("elastix");
  if (exeElastix.empty())
    mitkThrow() << "Elastix executable not found!";


  // Create a temporary directory if workdir not defined
  std::string workdir = userDefinedProcessingPath;
  if (workdir.empty())
    workdir = mitk::IOUtil::CreateTemporaryDirectory();

  // process handler
  const auto StartPocoProcess = [](const auto &exe, const auto &args) {
    // Poco::Pipe outPipe;
    // Poco::PipeInputStream istr(outPipe);
    MITK_INFO << "Start Registration ...";
    Poco::ProcessHandle ph(Poco::Process::launch(exe, args, nullptr, nullptr, nullptr));
    ph.wait();
    MITK_INFO << "Registration Complete";
  };

  std::vector<std::string> resultTransformations;
  std::string workdirPath = JoinPath({workdir});

  const auto resultTransformPath = JoinPath({workdirPath, "/", "TransformParameters.0.txt"});

  // Write parameter files
  for (unsigned int i = 0; i < parameterFilePathOrStrings.size(); ++i)
  {
    const auto targetParamterFilePath = JoinPath({workdirPath, "/", "pp_" + std::to_string(i) + ".txt"});
    const auto element = parameterFilePathOrStrings[i];
    if (itksys::SystemTools::FileExists(element) && !itksys::SystemTools::FileIsDirectory(element))
    {
      auto ifs = std::ifstream(resultTransformPath);
      std::string parameterText(std::istreambuf_iterator<char>{ifs}, {});

      const auto extension = itksys::SystemTools::GetFilenameExtension(element);

      // TODO: additional checks if p is a real path to an elastix parameter file
      if (extension.find("txt") != std::string::npos)
      {
        MITK_INFO << parameterText;
        itksys::SystemTools::CopyAFile(element, targetParamterFilePath);
      }
    }
    else
    { // if it is no valid path we assume that the string is a parameter file
      std::ofstream outStream(targetParamterFilePath);
      outStream << element;
      outStream.close();
      // TODO: ReplaceParameter(param, "MaximumNumberOfIterations",
      // std::to_string(m_Controls.DeformableMaxIters->value()));
    }
  }


  const auto resultPath = JoinPath({workdirPath, "/", "result.0.nrrd"});
  const auto fixedPath = JoinPath({workdirPath, "/", "fixed.nrrd"});

  auto movingPath = JoinPath({workdirPath, "/", "moving_" + std::to_string(0) + ".nrrd"});
  mitk::IOUtil::Save(moving2D, movingPath);
  mitk::IOUtil::Save(fixed2D, fixedPath);

  for (unsigned int i = 0; i < parameterFilePathOrStrings.size(); ++i)
  {
    const auto parameterFile = JoinPath({workdirPath, "/", "pp_" + std::to_string(i) + ".txt"});

    // fixed 
    Poco::Process::Args args;
    args.insert(args.end(), {"-f", fixedPath});
    args.insert(args.end(), {"-out", workdir});

    // changes every iteration
    args.insert(args.end(), {"-m", movingPath});
    args.insert(args.end(), {"-p", parameterFile});
    StartPocoProcess(exeElastix, args);

    // Post process
    movingPath = JoinPath({workdirPath, "/", "moving_" + std::to_string(i+1) + ".nrrd"});
    itksys::SystemTools::CopyFileAlways(resultPath, movingPath);

    // read transformations and save in vector
    auto ifs = std::ifstream(resultTransformPath);
    std::string parameterText(std::istreambuf_iterator<char>{ifs}, {});
    resultTransformations.push_back(parameterText);
  }

  return resultTransformations;

  // if (image != nullptr)
  // {
  //   auto Filter3DToSlice = mitk::Image3DSliceToImage2DFilter::New();
  //   Filter3DToSlice->SetInput(image);
  //   Filter3DToSlice->Update();
  //   mitk::IOUtil::Save(Filter3DToSlice->GetOutput(), movingImagefilePath);
  //   itkSys::CopyFileAlways(movingImagefilePath, inputImagefilePath);

  //   for (std::string trafo : transformations)
  //   {
  //     // apply the transformation file changes e.g. pixel type output, interpolation mode etc..
  //     transformationModifier(trafo);

  //     std::ofstream(parameterFilePath) << trafo;

  //     Poco::Process::Args args = {"-in", movingImagefilePath, "-out", workdirPath, "-tp", parameterFilePath};
  //     auto handle = Poco::Process::launch(executablePath, args);
  //     auto err = handle.wait();
  //     if (err)
  //       MITK_ERROR << "Some error (" << err << ") on Poco::Process::launch call of" << executablePath;

  //     // result image is the new moving image for the next iteration
  //     itkSys::CopyFileAlways(resultImagefilePath, movingImagefilePath);
  //     break;
  //   }

  //   auto res = mitk::IOUtil::Load(movingImagefilePath);
  //   outImage = dynamic_cast<mitk::Image *>(res[0].GetPointer());

  //   auto FilterSliceTo3D = mitk::Image2DToImage3DSliceFilter::New();
  //   FilterSliceTo3D->SetInput(image);
  //   FilterSliceTo3D->Update();
  //   outImage = FilterSliceTo3D->GetOutput();
  // }
  // if (removeProcessDir)
  // {
  //   itkSys::RemoveADirectory(workdirPath);
  // }
  // else
  // {
  //   MITK_INFO << "Warp-Image processing dir: " << workdirPath;
  // }
  // return outImage;
}
