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


