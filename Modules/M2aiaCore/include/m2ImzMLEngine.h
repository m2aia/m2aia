/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#pragma once

#include <M2aiaCoreExports.h>
#include <map>
#include <string>


namespace m2
{
  class M2AIACORE_EXPORT TemplateEngine
  {
    enum RenderIf
    {
      NoneIf = 0x0,
      OpenIf = 0x1,
      CloseIf = 0x2,
      IsFalseIf = 0x4,
      IsTrueIf = 0x8
    };

  public:
    static std::string render(const std::string &view,
                              std::map<std::string, std::string> &map,
                              char from = '{',
                              char to = '}');
  };
} // namespace ImzML
