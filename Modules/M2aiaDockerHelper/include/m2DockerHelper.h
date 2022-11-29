/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#pragma once
#include <mitkDataNode.h>
#include <vector>
#include <string>

/**
  \brief m2DockerHelper can be used to launch docker processes. Please refer to doc of ExecuteModule method for further information.
*/
namespace m2
{
class DockerHelper
{
public:
  static std::vector<std::string> split(const std::string& str, const char delimiter);

  /// @brief escapes character in input string (useful for path manipulation)
  /// @param input string which should be escaped
  /// @param c character which shall be escaped
  /// @return string with escaped character
  static std::string escape(std::string input, const char c);

  /// @brief Lauches a process with Docker executing the specified container, BLOCKING FOR DURATION OF EXECUTION
  /// @param containerName Name of the container to execute
  /// @param nodes Datanodes from DatanodeManager that will be used as input
  /// @param moduleParams Args for the module
  /// @return path of output directory or empty string in case of error
  static std::string ExecuteModule(const std::string& containerName, const std::vector<mitk::DataNode::ConstPointer>& nodes, const std::vector<std::string>& moduleParams);
};
} // namespace m2
