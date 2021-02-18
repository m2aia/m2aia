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
  MITK_TEST(ApplyErosion_RandomGaussianSignal_shouldReturnTrue);
  MITK_TEST(ApplyDilatation_RandomGaussianSignal_shouldReturnTrue);
  MITK_TEST(TopHat_RandomGaussianSignal_shouldReturnTrue);

  CPPUNIT_TEST_SUITE_END();

  /*
  Signal was generated once using the following procedure
    -----
	const double mean = 1.0;
    const double stddev = 0.33;
    std::default_random_engine generator;
    generator.seed(142191);
    std::normal_distribution<double> dist(mean, stddev);

    std::vector<double> signal;
    auto it = std::inserter(signal, std::begin(signal));
    for (int i = 0; i < 1000; ++i)
      it = dist(generator);
    -----
  */

private:
  std::vector<double> ReadDoubleVector(const std::string &fileNameInM2aiaDir, char delim = '\n')
  {
    std::vector<double> signal;
    std::ifstream f(GetTestDataFilePath(fileNameInM2aiaDir, M2AIA_DATA_DIR));
    std::string line;
    while (std::getline(f, line, delim))
    {
      signal.push_back(std::stoi(line));
    }
    return signal;
  }

public:
  void ApplyDilatation_RandomGaussianSignal_shouldReturnTrue()
  {
    std::vector<double> signal = ReadDoubleVector("signal.data");
    std::vector<double> expected = ReadDoubleVector("dilation.result");

    std::vector<double> result(signal.size());
    m2::Signal::Dilation(signal, 2, result);

    CPPUNIT_ASSERT(std::equal(std::begin(result), std::end(result), std::begin(expected)));
  }

  void ApplyErosion_RandomGaussianSignal_shouldReturnTrue()
  {
    std::vector<double> signal = ReadDoubleVector("signal.data");
    std::vector<double> expected = ReadDoubleVector("erosion.result");

    std::vector<double> result(signal.size());
    m2::Signal::Erosion(signal, 2, result);

    CPPUNIT_ASSERT(std::equal(std::begin(result), std::end(result), std::begin(expected)));
  }

  void TopHat_RandomGaussianSignal_shouldReturnTrue()
  {
    std::vector<double> signal = ReadDoubleVector("signal.data");
    std::vector<double> expected = ReadDoubleVector("tophat.result");

    std::vector<double> result(signal.size());
    m2::Signal::Erosion(signal, 2, result);
    m2::Signal::Dilation(result, 2, result);

    CPPUNIT_ASSERT(std::equal(std::begin(result), std::end(result), std::begin(expected)));
  }
};

MITK_TEST_SUITE_REGISTRATION(m2Morphology)
