
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
#include <algorithm>
#include <cassert>
#include <functional>
#include <thread>
#include <vector>

#include <mitkExceptionMacro.h>

namespace m2
{
  struct Process
  {
    static void Map(
      unsigned long int N,
      unsigned int T,
      const std::function<void(unsigned int threadId, unsigned int startIdx, unsigned int endIdx)> &worker)
    {
      // Validate before dividing (T == 0 used to be a division by zero).
      if (N < 1)
        mitkThrow() << "The number of input unit is < 1!";

      if (T < 1)
        mitkThrow() << "The number of threads is < 1!";

      // Never start more workers than items: every worker gets a non-empty range
      // and each index in [0, N) is handed to exactly one worker.
      const unsigned int workers = static_cast<unsigned int>(std::min<unsigned long int>(N, T));
      const unsigned long int n = N / workers; // >= 1
      const unsigned long int r = N % workers;

      // start the workers
      std::vector<std::thread> threads;
      threads.reserve(workers);
      for (unsigned int t = 0; t < workers; ++t)
      {
        const unsigned long int start = t * n;
        const unsigned long int end = (t + 1 == workers) ? (t + 1) * n + r : (t + 1) * n;
        threads.emplace_back(worker, t, static_cast<unsigned int>(start), static_cast<unsigned int>(end));
      }

      // wait until the work is done
      for (auto &thread : threads)
        thread.join();
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