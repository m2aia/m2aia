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
#include <m2ImzMLMassSpecImage.h>

namespace m2
{
  class M2AIACORE_EXPORT ImzMLXMLParser
  {
  public:
    /*!
     * Input: An ImzMLMassSpecImage object
     *
     * \param data
     */

    static void FastReadMetaData(m2::ImzMLMassSpecImage::Pointer data);

    static void SlowReadMetaData(m2::ImzMLMassSpecImage::Pointer data);

    //static std::string WriteMetaData(m2::ImzMLMassSpecImage::Pointer val, std::string &path);

    static auto findLine(std::ifstream &f, std::string name, std::string start_tag, bool eol = false)
      -> unsigned long long;
  };

} // namespace mitk
