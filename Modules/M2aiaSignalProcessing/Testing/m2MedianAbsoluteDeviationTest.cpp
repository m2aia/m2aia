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
#include <m2MedianAbsoluteDeviation.h>
#include <mitkTestFixture.h>
#include <mitkTestingMacros.h>
#include <random>

//#include <boost/algorithm/string.hpp>

class m2MedianAbsoluteDeviationTestSuite : public mitk::TestFixture
{
  CPPUNIT_TEST_SUITE(m2MedianAbsoluteDeviationTestSuite);
  MITK_TEST(ApplyMAD_SeededGaussianNoise_shouldReturnTrue);

  CPPUNIT_TEST_SUITE_END();

private:
public:
  void ApplyMAD_SeededGaussianNoise_shouldReturnTrue()
  {
    const double mean = 1.0;
    const double stddev = 0.33;
    std::default_random_engine generator;
    generator.seed(142191);
    std::normal_distribution<double> dist(mean, stddev);

    std::vector<double> signal;
    auto it = std::inserter(signal, std::begin(signal));
    for (int i = 0; i < 1000; ++i)
      it = dist(generator);

    double noiseLevel = m2::Signal::mad(signal);
    CPPUNIT_ASSERT(noiseLevel == .32093696751496925134716775573906488716602325439453125);
  }
};

MITK_TEST_SUITE_REGISTRATION(m2MedianAbsoluteDeviation)
