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
#include <m2ImzMLSpectrumImage.h>

namespace m2
{
  class M2AIACORE_EXPORT ImzMLParser
  {
  public:
    /*!
     * Input: An ImzMLSpectrumImage object
     *
     * \param data
     */

    using FunctionType = std::function<void(const std::string &)>;
    using FunctionMap = std::unordered_map<std::string, FunctionType>;

    static void ReadImageMetaData(m2::ImzMLSpectrumImage::Pointer data);

    static void ReadImageSpectrumMetaData(m2::ImzMLSpectrumImage::Pointer data);

    static void GetElementName(const std::string &line, std::string &name)
    {
      name.clear();
      auto p = line.find('<'); //no spaces allowed after '<'
      if (p != std::string::npos)
      {
        ++p; // let p point after '<'
        auto e = line.find_first_of(" >", p); // fine first of either ' ' or '>'
        while (p != e)
          name.push_back(line[p++]);
      }
    }



    static std::string & attributeValue(const std::string &line, const std::string &tag, std::string &attribute)
    {
      auto p = line.find(tag);
      attribute.clear();
      if (p != std::string::npos)
      {
        auto s_value = line.find('"', p) + 1;
        auto e_value = line.find('"', s_value);
        attribute.reserve(e_value-s_value+1);
        while (s_value != e_value)
          attribute.push_back(line[s_value++]);
      }
      return attribute;
    }


    static bool evaluateAccession(const std::string &line, const std::string &accession, const FunctionMap &dict)
    {
      auto keyFuncPairIt = dict.find(accession);
      if (keyFuncPairIt != dict.end())
      {
        keyFuncPairIt->second(line); // call the function
        return true;
      }
      return false;
    }

    static void EvaluateContext(const std::string &line, const std::string &tag, const FunctionMap &dict)
    {
      auto keyFuncPairIt = dict.find(tag);
      if (keyFuncPairIt != dict.end())
      {
        return keyFuncPairIt->second(line); // call the function
      }
      return;
    };

    //static std::string WriteMetaData(m2::ImzMLSpectrumImage::Pointer val, std::string &path);

    static auto findLine(std::ifstream &f, std::string name, std::string start_tag, bool eol = false)
      -> unsigned long long;
  };

} // namespace mitk
