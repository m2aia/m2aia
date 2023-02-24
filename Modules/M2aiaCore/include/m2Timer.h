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

namespace m2
{
  class Timer
  {
  public:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::duration<float> duration;
    std::string m_what;
    int m_N = 1;

    ~Timer()
    {
      duration = std::chrono::high_resolution_clock::now() - start;
      MITK_INFO << m_what << ": "<< duration.count()<< " Average(" << m_N <<" runs):" << duration.count()/double(m_N) << "s";
    }

    explicit Timer(std::string what) : m_what(what)
    {
      MITK_INFO << m_what << " started ...";
      start = std::chrono::high_resolution_clock::now();
    }

    template<class F>
    explicit Timer(std::string what, int n, F && lambda) : Timer(what)
    {
      m_N = n;
      for(int i = 0; i < m_N; ++i)
        lambda();
    }
  };

} // namespace mitk