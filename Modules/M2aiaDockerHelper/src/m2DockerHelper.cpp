/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include "m2DockerHelper.h"

// Poco
#include <Poco/Process.h>

// std
#include <charconv>
#include <filesystem>
#include <sstream>

// mitk
#include <mitkIOUtil.h>

// m2aia
#include <m2ImzMLSpectrumImage.h>
#include <m2SpectrumImageBase.h>

#define DH_SIG "[M2aiaDockerHelper] "
namespace m2 
{

std::vector<std::string> DockerHelper::split(const std::string& str, const char delimiter)
{
  std::stringstream sstream(str);
  std::string temp;
  std::vector<std::string> vec;
  while(getline(sstream, temp, delimiter))
  {
    if (temp != "")
      vec.push_back(temp);
  }
  return vec;
}

std::string& DockerHelper::escape(std::string& input, const char c)
{
  size_t pos = 0;
  while (pos < input.size())
  {
    pos = input.find_first_of(c, pos);
    if (pos >= input.size()) return input;
    input.replace(input.begin() + pos, input.begin() + pos + 1, (std::string("\\") + c).c_str());
    pos += 2; // skip the backslash and c
    
    std::string space = "";
    for (size_t i = 0; i < pos; ++i) space += " ";
    MITK_INFO << DH_SIG << "[escape] " << input;
    MITK_INFO << DH_SIG << "[escape] " << space << "^" << " pos";
  }
  return input;
}

std::string DockerHelper::ExecuteModule(const std::string& containerName, const std::vector<mitk::DataNode::ConstPointer>& nodes, const std::vector<std::string>& moduleParams)
{
  // command to be launched
  std::string command = "docker";
  // args of the command to be launched
  std::string arguments = "run -t --rm --name=m2aia-container";
  std::vector<std::string> splitPath;
  std::vector<std::string> imzmlNames;

  for (size_t i = 0; i < nodes.size(); ++i)
  {
    auto image = static_cast<m2::ImzMLSpectrumImage*>(nodes[i]->GetData());
    std::string inputPath_i;
    image->GetPropertyList()->GetStringProperty("MITK.IO.reader.inputlocation", inputPath_i);
    splitPath = split(inputPath_i, '/');
    inputPath_i = "/";
    for (size_t j = 0; j < splitPath.size() - 1; ++j)
      inputPath_i += splitPath[j] + "/";
    imzmlNames.push_back(splitPath[splitPath.size() - 1]);
    arguments += " -v " + inputPath_i + ":/data";
    std::array<char, 4> buffer = { 0 };
    std::to_chars(buffer.data(), buffer.data() + buffer.size(), i + 1); // convert i+1 to char[]
    buffer[buffer.size() - 1] = 0; // terminate string
    arguments += buffer.data() + std::string{"/ "};
  }

  std::string outputPath = mitk::IOUtil::CreateTemporaryDirectory();
  auto outputPathVec = split(outputPath, '/');
  outputPath = "/";
  for (size_t i = 0; i < outputPathVec.size(); ++i)
    if (outputPathVec[i] != "")
      outputPath += outputPathVec[i] + "/";

  // mount output directory
  arguments += " -v " + outputPath + ":/output1/ " + containerName + " ";

  std::string interpreter = "";
  std::string fileEnding = "";

  // TODO: replace this with a better way to check what kind of file will be executed
  if (containerName.find("Py") != std::string::npos || containerName.find("py") != std::string::npos)
  {
    interpreter = "python3";
    fileEnding = ".py";
  } 
  else if (containerName.find("r-") != std::string::npos || containerName.find("R-") != std::string::npos)
  {
    interpreter = "Rscript";
    fileEnding = ".R";
  }

  // args to the cli_module inside container
  arguments += interpreter + " cli-module" + fileEnding;
  auto stv = split(arguments, ' ');

  // store escaped filenames as one argument each
  for (auto imzmlName : imzmlNames)
    stv.push_back(imzmlName);
  // get the args  
  Poco::Process::Args processArgs;
  processArgs.insert(processArgs.end(), stv.begin(), stv.end());
  processArgs.insert(processArgs.end(), moduleParams.begin(), moduleParams.end());
  // launch the process
  Poco::Process p;
  MITK_INFO << DH_SIG << "launching process with command '" << command << " " << arguments << "'.";
  auto handle = p.launch(command, processArgs);
  // blocking until launched process is done
  int code = handle.wait();
  MITK_INFO << DH_SIG << "finished processing with code " << code;
  if (code) 
  {
    MITK_INFO << DH_SIG << strerror(code);
    return "";
  }
  else return outputPath;
}

} // namespace m2
