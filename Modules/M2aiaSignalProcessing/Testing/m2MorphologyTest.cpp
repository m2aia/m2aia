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
#include <algorithm>
#include <m2Morphology.h>
#include <m2TestingConfig.h>
#include <mitkTestFixture.h>
#include <mitkTestingMacros.h>
#include <numeric>
#include <random>

//#include <boost/algorithm/string.hpp>

class m2MorphologyTestSuite : public mitk::TestFixture
{
  CPPUNIT_TEST_SUITE(m2MorphologyTestSuite);
  MITK_TEST(ApplyDilatation_SeededGaussianNoise_shouldReturnTrue);

  CPPUNIT_TEST_SUITE_END();

private:
public:
  void ApplyDilatation_SeededGaussianNoise_shouldReturnTrue()
  {
    const double mean = 1.0;
    const double stddev = 2.0;
    std::default_random_engine generator;
    generator.seed(142191);
    std::normal_distribution<double> dist(mean, stddev);

    std::vector<double> signal;
    for (int i = 0; i < 1000; ++i)
      signal.push_back(int(dist(generator) + 0.5));

    std::vector<double> result(signal.size());
    m2::Signal::Dilation(signal, 2, result);

    std::ofstream f(GetTestDataFilePath("dilation.result", M2AIA_DATA_DIR));
    std::ofstream d(GetTestDataFilePath("signal.data", M2AIA_DATA_DIR));
    f << result[0];
    d << signal[0];
    for (int i = 1; i < 1000; ++i)
    {
      f << "," << result[i];
      d << "," << signal[i];
      
    }
  }
};

MITK_TEST_SUITE_REGISTRATION(m2Morphology)
