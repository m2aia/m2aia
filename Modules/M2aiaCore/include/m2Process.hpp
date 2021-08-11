
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
#include <cassert>
#include <functional>
#include <thread>
#include <vector>

namespace m2
{
  struct Process
  {
    static void Map(
      unsigned long int N,
      unsigned int T,
      const std::function<void(unsigned int threadId, unsigned int startIdx, unsigned int endIdx)> &worker)
    {
      std::vector<std::thread> threads;
      threads.clear();
      unsigned int n = N / T;

      if (N < 1)
        mitkThrow() << "The number of input unit is < 1!";

      if (T < 1)
        mitkThrow() << "The number of threads is < 1!";

      if (n == 0)
      { // recursively reduce threads to get non-zero n
        Map(N, T / 2, worker);
      }

      // start the workers
      unsigned int r = N % T;
      for (unsigned int t = 0; t < T; ++t)
      {
        if (t != (T - 1))
          threads.emplace_back(std::thread(worker, t, t * n, (t + 1) * n));
        else
          threads.emplace_back(std::thread(worker, t, t * n, (t + 1) * n + r));
      }

      // wait until the work is done
      for (auto &t : threads)
        t.join();
    }

    template <class ElementType, class BinaryReduceOperationFunctionType, class UnaryFinalizeOperationFunctionType>
    static std::vector<ElementType> Reduce(const std::vector<std::vector<ElementType>> &cont,
                                           BinaryReduceOperationFunctionType reduceOp,
                                           UnaryFinalizeOperationFunctionType finalOp)
    {
      std::vector<ElementType> resultCont(cont.front().size());
      for (const auto &v : cont)
      {
        for (unsigned k = 0; k < v.size(); ++k)
        {
          resultCont[k] = reduceOp(resultCont[k], v[k]);
        }
      }

      for (unsigned k = 0; k < resultCont.size(); ++k)
      {
        resultCont[k] = finalOp(resultCont[k]);
      }
      return resultCont;
    }
  };
} // namespace m2