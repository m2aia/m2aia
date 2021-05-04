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

#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>
#include <Poco/StreamCopier.h>
#include <itksys/System.h>
#include <itksys/SystemTools.hxx>
#include <m2CoreCommon.h>
#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkImage2DToImage3DSliceFilter.h>
#include <mitkImage3DSliceToImage2DFilter.h>
#include <mitkLabelSetImage.h>
#include <mitkPointSet.h>
#include <regex>

namespace m2
{
  namespace System
  {
    int CheckVersion(const std::string &executablePath,
                     const std::regex &version_regex,
                     const std::string &argVersion = "--version")
    {
      Poco::Process::Args args;
      args.push_back(argVersion);
      Poco::Pipe outPipe;
      Poco::PipeInputStream istr(outPipe);
      Poco::ProcessHandle ph(Poco::Process::launch(executablePath, args, nullptr, &outPipe, nullptr));
      ph.wait();
      std::stringstream ss;
      Poco::StreamCopier::copyStream(istr, ss);
      MITK_INFO << ss.str();
      return std::regex_search(ss.str(), version_regex);
    }
  } // namespace System

  namespace Transform
  {
    mitk::Image::Pointer WarpImage(
      const std::string &executablePath,
      const mitk::Image *image,
      const std::vector<std::string> &transformations,
      const std::string &processDir = "",
      bool removeProcessDir = true,
      std::function<void(std::string &)> transformationModifier = [](std::string &) {})
    {
      if (transformations.empty())
        return const_cast<mitk::Image *>(image);

      if (!System::CheckVersion(executablePath, std::regex{"transformix[a-z:\\s]+5\\.[0-9]+"}))
        MITK_ERROR << "Version conflict: transformix version >= 5.000 expected!";

      using itkSys = itksys::SystemTools;

      mitk::Image::Pointer outImage;
      mitk::ImageToImageFilter::Pointer filter;
      std::string workdir = processDir;
      if (processDir.empty())
        workdir = mitk::IOUtil::CreateTemporaryDirectory();

      std::string workdirPath = itkSys::ConvertToOutputPath(workdir);
      const auto parameterFilePath = itkSys::ConvertToOutputPath(itkSys::JoinPath({workdirPath, "/", "Transform.txt"}));
      const auto movingImagefilePath = itkSys::ConvertToOutputPath(itkSys::JoinPath({workdirPath, "/", "moving.nrrd"}));
      const auto inputImagefilePath = itkSys::ConvertToOutputPath(itkSys::JoinPath({workdirPath, "/", "input.nrrd"}));
      const auto resultImagefilePath = itkSys::ConvertToOutputPath(itkSys::JoinPath({workdirPath, "/", "result.nrrd"}));

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

          Poco::Process::Args args;
          args.push_back("-in");
          args.push_back(movingImagefilePath);
          args.push_back("-out");
          args.push_back(workdirPath);
          args.push_back("-tp");
          args.push_back(parameterFilePath);

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
  } // namespace Transform
} // namespace m2
