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

#include <chrono>
#include <iostream>
#include <mbilog.h>
#include <string>

namespace mitk
{
  class Timer
  {
  public:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::duration<float> duration;
    std::string m_what;

    ~Timer()
    {
      duration = std::chrono::high_resolution_clock::now() - start;
      MITK_INFO << m_what << ": " << duration.count() << "s";
    }

    explicit Timer(std::string what) : m_what(what)
    {
      MITK_INFO << m_what << " started ...";
      start = std::chrono::high_resolution_clock::now();
    }
  };

} // namespace mitk