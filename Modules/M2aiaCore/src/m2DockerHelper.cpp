/*===================================================================
MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

// Additional Attributions: Lorenz Schwab

#include <Poco/Process.h>
#include <m2DockerHelper.h>
#include <mitkIOUtil.h>
#include <m2HelperUtils.h>
#include <filesystem>

bool m2::DockerHelper::CheckDocker()
{
  std::string command = "docker ps";
  int result = std::system(command.c_str());
  if (result == 0)
  {
    MITK_INFO << "Docker is installed on this system." << std::endl;
    return true;
  }
  else
  {
    MITK_INFO << "Docker is not installed on this system." << std::endl;
    return false;
  }
}

void m2::DockerHelper::AddData(mitk::BaseData::Pointer data, std::string targetArgument, std::string nameWithExtension)
{
  m_Data.try_emplace(data, targetArgument, nameWithExtension);
}

void m2::DockerHelper::AddOutput(std::string targetArgument, std::string nameWithExtension){
  m_Outputs.try_emplace(targetArgument, nameWithExtension);
}

void m2::DockerHelper::EnableAutoRemoveImage(bool value)
{
  m_AutoRemoveImage = value;
}

void m2::DockerHelper::EnableAutoRemoveContainer(bool value)
{
  m_AutoRemoveContainer = value;
}

void m2::DockerHelper::ExecuteDockerCommand(std::string command, const std::vector<std::string> &args)
{
  Poco::Process::Args processArgs;
  processArgs.push_back(command);
  for (auto a : args)
  {
    processArgs.push_back(a);
  }

  // launch the process
  Poco::Process p;
  int code;
  auto handle = p.launch("docker", processArgs);
  code = handle.wait();

  if (code)
  {
    mitkThrow() << "docker " << command << " failed with exit code [" << code << "]";
  }
}

void m2::DockerHelper::Run(const std::vector<std::string> &cmdArgs, const std::vector<std::string> &entryPointArgs)
{

  std::vector<std::string> args;
  args.insert(args.end(), cmdArgs.begin(), cmdArgs.end());
  
  if (m_AutoRemoveContainer && std::find(args.begin(), args.end(), "--rm") == args.end())
    args.push_back("--rm");

  args.push_back(m_ImageName);
  args.insert(args.end(), entryPointArgs.begin(), entryPointArgs.end());

  ExecuteDockerCommand("run", args);
}

void m2::DockerHelper::RemoveImage(std::vector<std::string> args)
{
  if (std::find(args.begin(), args.end(), "-f") == args.end())
    args.insert(args.begin(), "-f");
  ExecuteDockerCommand("rmi", args);
}

m2::DockerHelper::MappingsAndArguments m2::DockerHelper::DataToDockerRunArguments() const
{
  MappingsAndArguments ma;

  // working directory name of the host system is used as mounting point name in the container
  // this folder is used as persistent communication bridge between container and host and vice versa.
  const auto dirPathContainer = m_WorkingDirectory.substr(m_WorkingDirectory.rfind('/'));
  // add this as not "read only" mapping
  ma.mappings.push_back("-v");
  ma.mappings.push_back(m_WorkingDirectory + ":" + dirPathContainer); 

  for (const auto kv : m_Data)
  {
    const auto data = kv.first;
    const auto dataInfo = kv.second;
    std::string filePath;
    data->GetPropertyList()->GetStringProperty("MITK.IO.reader.inputlocation", filePath);
    if (filePath.empty())
    {
      const auto filePathHost = itksys::SystemTools::ConvertToOutputPath(m_WorkingDirectory + "/" + dataInfo.nameWithExtension);
      mitk::IOUtil::Save(data, filePathHost);
      const auto filePathContainer = itksys::SystemTools::ConvertToOutputPath(dirPathContainer + "/" + dataInfo.nameWithExtension);
      ma.arguments.push_back(dataInfo.targetArgument);
      ma.arguments.push_back(filePathContainer);
    }
    else
    {
      // Actual this dir is never used on the host system, but the name is involved on in the container as mounting point
      const auto phantomDirPathHost = m2::HelperUtils::TempDirPath(); 
      // Extract the directory name
      const auto dirPathContainer = phantomDirPathHost.substr(phantomDirPathHost.rfind('/'));
      // so delete it on the host again
      itksys::SystemTools::RemoveADirectory(phantomDirPathHost); 

      // find the files location (parent dir) on the host system 
      const auto dirPathHost = itksys::SystemTools::GetParentDirectory(filePath);

      // mount this parent dir as read only into the container
      ma.mappings.push_back("-v");
      ma.mappings.push_back(dirPathHost + ":" + dirPathContainer + ":ro");

      auto fileName = itksys::SystemTools::GetFilenameWithoutExtension(filePath);
      auto proposedExtension = itksys::SystemTools::GetFilenameExtension(dataInfo.nameWithExtension);

      const auto filePathContainer = itksys::SystemTools::ConvertToOutputPath(dirPathContainer + "/" + fileName + proposedExtension);
      ma.arguments.push_back(dataInfo.targetArgument);
      ma.arguments.push_back(filePathContainer);
    }
  }

  for (const auto kv : m_Outputs)
  {
    const auto argumentName = kv.first;
    const auto fileName = kv.second;

    // pass as arguments to ma.arguments
    const auto filePathContainer = itksys::SystemTools::ConvertToOutputPath(dirPathContainer + "/" + fileName);
    ma.arguments.push_back(argumentName);
    ma.arguments.push_back(filePathContainer);

  }

  return ma;
}

void m2::DockerHelper::LoadResults(){

for (const auto kv : m_Outputs)
  {
    const auto argumentName = kv.first;
    const auto fileName = kv.second;

    // pass as arguments to ma.arguments
    const auto filePathContainer = itksys::SystemTools::ConvertToOutputPath(m_WorkingDirectory + "/" + fileName);
    auto data = mitk::IOUtil::Load(filePathContainer);
    MITK_INFO << "Load " << filePathContainer;
    m_OutputData.insert(m_OutputData.end(), data.begin(), data.end());

  }
}

void m2::DockerHelper::GetResults()
{
  if (!CheckDocker())
  {
    mitkThrow() << "No Docker instance found!";
  }

  auto runArgs = DataToDockerRunArguments();

  Run(runArgs.mappings, runArgs.arguments);
  LoadResults();

  MITK_INFO << "Size of the results vector " << m_OutputData.size();
   

  // autoremove image
  if (m_AutoRemoveImage)
  {
    RemoveImage({m_ImageName});
  }
}