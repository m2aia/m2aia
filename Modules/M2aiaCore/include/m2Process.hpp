
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
#include <functional>
#include <thread>
#include <vector>

namespace m2
{
  struct Process
  {
    static void Map(unsigned long int N,
                    unsigned int T,
                    const std::function<void(unsigned int threadId, unsigned int startIdx, unsigned int endIdx)> &worker)
    {
      std::vector<std::thread> threadPool;
      threadPool.clear();
      unsigned int n = N / T;
      unsigned int r = N % T;
      for (unsigned int t = 0; t < T; ++t)
      {
        if (t != (T - 1))
          threadPool.emplace_back(std::thread(worker, t, t * n, (t + 1) * n));
        else
          threadPool.emplace_back(std::thread(worker, t, t * n, (t + 1) * n + r));
      }
      for (auto &t : threadPool)
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