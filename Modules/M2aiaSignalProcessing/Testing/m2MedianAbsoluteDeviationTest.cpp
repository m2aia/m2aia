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

    std::vector<double> signal = {5, 5, 9, 5, 5, 5, 5, 0, 4, 4, 4, 6, 6, 6};
    double noiseLevel = m2::Signal::mad(signal);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(double(1.4825999999999999), noiseLevel, mitk::eps);
  }
};

MITK_TEST_SUITE_REGISTRATION(m2MedianAbsoluteDeviation)
