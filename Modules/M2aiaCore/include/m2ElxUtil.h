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

#include <M2aiaCoreExports.h>
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
#include <string>

namespace m2
{
  class M2AIACORE_EXPORT ElxUtil
  {
  public:
    /**
     * @brief Search an elastix parameter file string for the specific entry and replaces it (or if not found appends
     * the string) by "(" + what + " " + by + ")
     *
     * @param paramFileString
     * @param what
     * @param by
     */
    static void ReplaceParameter(std::string &paramFileString, std::string what, std::string by)
    {
      auto pos1 = paramFileString.find("(" + what);
      auto pos2 = paramFileString.find(')', pos1);
      if (pos1 == std::string::npos || pos2 == std::string::npos)
        paramFileString += "(" + what + " " + by + ")\n";
      else
        paramFileString.replace(pos1, pos2 - pos1 + 1, "(" + what + " " + by + ")\n");
    }

    /**
     * @brief Search the system for elastix executables (name)
     * 1) additional search path
     * 2) check ELASTIX_PATH
     * 3) check PATH
     *
     * @param name
     * @param additionalSearchPath
     * @return std::string
     */
    static std::string Executable(const std::string &name, std::string additionalSearchPath = "");

    /**
     * @brief CheckVersion can be used to evaluate the version of any external executable using regular expressions.
     * System::CheckVersion("path/to/exe", std::regex{"exeinfo[a-z:\\s]+5\\.[0-9]+"}, "--version")
     *
     * @param executablePath
     * @param version_regex
     * @param argVersion
     * @return bool
     */
    static bool CheckVersion(const std::string &executablePath,
                             const std::regex &version_regex,
                             const std::string &argVersion = "--version");

    /**
     * @brief CreatePath can be used to normalize and join path arguments
     * Example call:
     * CreatPath({"this/is/a/directory/path/with/or/without/a/slash/","/","test"});
     * @param args is a vector of strings
     * @return std::string
     */
    static std::string JoinPath(std::vector<std::string> &&args);

    /**
     * @brief
     *
     * @param image
     * @param transformations
     * @param processDir
     * @param removeProcessDir
     * @param transformationModifier
     * @return mitk::Image::Pointer
     */
    static mitk::Image::Pointer WarpImage(
      const mitk::Image *image,
      const std::vector<std::string> &transformations,
      const std::string &processDir = "",
      bool removeProcessDir = true,
      std::function<void(std::string &)> transformationModifier = [](std::string &) {});

    static std::string GetShape(const mitk::Image *img)
    {
      std::string shape;
      for (unsigned int i = 0; i < img->GetDimension(); ++i)
      {
        shape += std::to_string(img->GetDimensions()[i]);
        if (i < img->GetDimension() - 1)
          shape += ", ";
      }
      return shape;
    }

    static void SavePointSet(const mitk::PointSet::Pointer & pnts, const std::string & path)
    {
      std::ofstream f(path);
      f << "point\n";
      f << std::to_string(pnts->GetSize()) << "\n";
      for (auto it = pnts->Begin(); it != pnts->End(); ++it)
      {
        const auto &p = it->Value();
        f << p[0] << " " << p[1] << "\n";
      }

      f.close();
    }
  };
} // namespace m2