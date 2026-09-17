/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <cppunit/TestAssert.h>
#include <m2Process.hpp>
#include <m2TestFixture.h>
#include <mitkException.h>
#include <mitkTestingMacros.h>

#include <algorithm>
#include <mutex>
#include <tuple>
#include <vector>

class m2ProcessMapTestSuite : public m2::TestFixture
{
  CPPUNIT_TEST_SUITE(m2ProcessMapTestSuite);
  MITK_TEST(EveryIndexIsProcessedExactlyOnce);
  MITK_TEST(PartitionForLargeInputsIsUnchanged);
  MITK_TEST(InvalidArgumentsThrow);
  CPPUNIT_TEST_SUITE_END();

  using Call = std::tuple<unsigned int, unsigned int, unsigned int>; // thread id, start, end

  static std::vector<Call> RecordCalls(unsigned long int N, unsigned int T)
  {
    std::mutex mutex;
    std::vector<Call> calls;
    m2::Process::Map(N, T, [&](unsigned int t, unsigned int a, unsigned int b) {
      std::lock_guard<std::mutex> lock(mutex);
      calls.emplace_back(t, a, b);
    });
    std::sort(calls.begin(), calls.end());
    return calls;
  }

public:
  void EveryIndexIsProcessedExactlyOnce()
  {
    for (unsigned long int N = 1; N <= 100; ++N)
      for (unsigned int T : {1u, 2u, 3u, 7u, 8u, 20u, 24u, 64u})
      {
        const auto calls = RecordCalls(N, T);
        CPPUNIT_ASSERT(calls.size() <= std::min<unsigned long int>(N, T));
        std::vector<unsigned int> visits(N, 0);
        for (const auto &[t, a, b] : calls)
        {
          CPPUNIT_ASSERT(t < T);
          CPPUNIT_ASSERT(a < b && b <= N); // no empty or out-of-range work packages
          for (unsigned int i = a; i < b; ++i)
            ++visits[i];
        }
        CPPUNIT_ASSERT(std::all_of(visits.begin(), visits.end(), [](unsigned int v) { return v == 1; }));
      }
  }

  void PartitionForLargeInputsIsUnchanged()
  {
    // N >= T keeps the historical partition: N / T items per worker, the last worker takes the remainder.
    const auto calls = RecordCalls(50, 24);
    CPPUNIT_ASSERT_EQUAL(std::size_t(24), calls.size());
    for (unsigned int t = 0; t < 23; ++t)
      CPPUNIT_ASSERT(calls[t] == Call(t, 2 * t, 2 * t + 2));
    CPPUNIT_ASSERT(calls[23] == Call(23, 46, 50));
  }

  void InvalidArgumentsThrow()
  {
    const auto noop = [](unsigned int, unsigned int, unsigned int) {};
    CPPUNIT_ASSERT_THROW(m2::Process::Map(0, 4, noop), mitk::Exception);
    CPPUNIT_ASSERT_THROW(m2::Process::Map(10, 0, noop), mitk::Exception);
  }
};

MITK_TEST_SUITE_REGISTRATION(m2ProcessMap)
