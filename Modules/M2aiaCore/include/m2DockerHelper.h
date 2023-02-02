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

#pragma once

#include <M2aiaCoreExports.h>
#include <m2HelperUtils.h>
#include <mitkBaseData.h>

#include <mitkPointSet.h>
#include <string>
#include <vector>
#include <map>
#include <functional>


namespace m2
{
  /**
   * @brief This class manages file base
   *
   */
  class M2AIACORE_EXPORT DockerHelper
  {
  protected:
    

    struct MappingsAndArguments{
      std::vector<std::string> mappings;
      std::vector<std::string> arguments;
    };

    std::string m_ImageName, m_WorkingDirectory;
    bool m_AutoRemoveImage = false;
    bool m_AutoRemoveContainer = false;
    

    struct DataInfo{
      DataInfo(std::string targetArgument,  std::string nameWithExtension)
      : targetArgument(targetArgument),nameWithExtension(nameWithExtension)
      {};
      std::string targetArgument;
      std::string nameWithExtension;
    };
    std::map<mitk::BaseData::Pointer, DataInfo> m_Data;
    std::map<std::string,  std::string> m_Outputs;
    
    std::vector<mitk::BaseData::Pointer> m_OutputData;

    // std::map<DiskPath, ContainerPath> m_ReadableData;

    static std::string CreateWorkingDirectory();
    static void RemoveWorkingDirectory(std::string);



    void ExecuteDockerCommand(std::string command, const std::vector<std::string> & args);
    void Run(const std::vector<std::string> &cmdArgs, const std::vector<std::string> &entryPointArgs);
    void RemoveImage(std::vector<std::string> args = {});
    void LoadResults();

    MappingsAndArguments DataToDockerRunArguments() const;

  public:
    virtual ~DockerHelper(){}
    DockerHelper(std::string image):m_ImageName(image){
      m_WorkingDirectory = m2::HelperUtils::TempDirPath();
    }
    
    static bool CheckDocker();

    // void SetData(mitk::BaseData::Pointer data, std::string targetArgument, std::string extension);
    void AddData(mitk::BaseData::Pointer data, std::string targetArgument, std::string nameWithExtension);
    void AddOutput(std::string targetArgument, std::string nameWithExtension);

    void GetResults();
    void EnableAutoRemoveImage(bool value);
    void EnableAutoRemoveContainer(bool value);

  };

} // namespace m2