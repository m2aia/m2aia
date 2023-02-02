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

#include <itksys/System.h>
#include <itksys/SystemTools.hxx>
#include <mitkIOUtil.h>

namespace m2
{
  namespace HelperUtils
  {
    std::string TempDirPath() {
      auto path = mitk::IOUtil::CreateTemporaryDirectory("m2_XXXXXX");
      return itksys::SystemTools::ConvertToOutputPath(path);
    }

    std::string FilePath(std::string path, std::string ext) {
      return mitk::IOUtil::CreateTemporaryFile("XXXXXX", path) + ext;
    }
    
    /**
     * @brief CreatePath can be used to normalize and join path arguments
     * Example call:
     * CreatPath({"this/is/a/directory/path/with/or/without/a/slash/","/","test"});
     * @param args is a vector of strings
     * @return std::string
     */
    std::string JoinPath(std::vector<std::string> &&args)
    {
      return itksys::SystemTools::ConvertToOutputPath(itksys::SystemTools::JoinPath(args));
    }
  } // namespace HelperUtils

} // namespace m2