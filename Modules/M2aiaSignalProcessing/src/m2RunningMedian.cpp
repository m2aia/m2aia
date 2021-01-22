/*===================================================================
MIT License

Copyright (c) 2018 CSEM SA

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
===================================================================*/
#pragma once

#include <algorithm>
#include <cstring>
#include <m2RunningMedian.h>
#include <stdlib.h>
#include <vector>

template <class T>
void m2::RunMedian::apply(const std::vector<T> &y, unsigned int s, std::vector<T> &output) noexcept
{
  MedfiltData data;
  s = s * 2 + 1;
  MedfiltNode *nodes = new MedfiltNode[s];

  medfilt_init(&data, nodes, s, y.front());

  auto oit = std::begin(output);
  for (const auto &v : y)
  {
    double min, mid, max;
    medfilt(&data, v, &mid, &min, &max);
    if (mid < 0)
      *oit = max;
    else
      *oit = mid;
    ++oit;
  }

  delete nodes;
}
