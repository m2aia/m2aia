/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include "mitkIOUtil.h"
#include <m2RunningMedian.h>
#include <m2TestingConfig.h>
#include <mitkTestFixture.h>
#include <mitkTestingMacros.h>
#include <numeric>
#include <algorithm>
#include <random>

//#include <boost/algorithm/string.hpp>

class m2RunningMedianTestSuite : public mitk::TestFixture
{
  CPPUNIT_TEST_SUITE(m2RunningMedianTestSuite);
  MITK_TEST(ApplyRunningMedian_SeededGaussianNoise_shouldReturnTrue);

  CPPUNIT_TEST_SUITE_END();

private:
public:
  void ApplyRunningMedian_SeededGaussianNoise_shouldReturnTrue()
  {
    /*const double mean = 1.0;
    const double stddev = 0.33;
    std::default_random_engine generator;
    generator.seed(142191);
    std::normal_distribution<double> dist(mean, stddev);*/

    std::vector<double> signal = {5, 5, 9, 5, 5, 5, 5, 0, 4, 4, 4, 6, 6, 6};
    std::vector<double> result = {5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 6, 6};

    std::vector<double> median(signal.size());
    m2::RunMedian::apply(signal, 1, median); // half window size = 1; so window size is 2*1+1
  
	auto res = std::mismatch(std::begin(median), std::end(median), std::begin(result));

	CPPUNIT_ASSERT(res.first == std::end(median));

  }
};

MITK_TEST_SUITE_REGISTRATION(m2RunningMedian)
