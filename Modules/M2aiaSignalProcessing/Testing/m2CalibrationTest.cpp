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
#include <m2Normalization.h>
#include <m2TestingConfig.h>
#include <mitkTestFixture.h>
#include <mitkTestingMacros.h>
#include <numeric>
#include <random>

//#include <boost/algorithm/string.hpp>

class m2CalibrationTestSuite : public mitk::TestFixture
{
  CPPUNIT_TEST_SUITE(m2CalibrationTestSuite);
  MITK_TEST(ApplyTIC_RandomGaussianSignal_shouldReturnTrue);

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
      signal.push_back(std::stod(line));
    }
    return signal;
  }

  template <class T>
  void WriteVector(
    const std::vector<T> & vec, const std::string &fileNameInM2aiaDir, char delim = '\n', std::function<double(T &)> &func = [](T &t) {
    return t; })
  {
    std::ofstream f(GetTestDataFilePath(fileNameInM2aiaDir, M2AIA_DATA_DIR));

    for (int i = 0; i < vec.size(); i++)
    {
      f << std::setprecision(128) << vec[i];
      if (i < vec.size() - 1)
        f << delim;
    }
  }

public:
  void ApplyTIC_RandomGaussianSignal_shouldReturnTrue()
  {
    /*{
      std::ofstream f(GetTestDataFilePath("quadratic.data", M2AIA_DATA_DIR));

      for (int i = 0; i < 1000; i++)
      {
        f << std::setprecision(128) << -0.00012 * i * i + 0.32 * i + 800;
        if (i < 1000 - 1)
          f << "\n";
      }
    }*/
    std::vector<double> signal = ReadDoubleVector("signal.data");
    std::vector<double> mzs = ReadDoubleVector("quadratic.data");
    std::vector<double> expected = ReadDoubleVector("quadratic_tic_calibration.result");

    std::vector<double> result(signal.size());
    double tic = m2::Signal::TotalIonCurrent(std::begin(mzs), std::end(mzs), std::begin(signal));
    MITK_INFO << tic;
    std::transform(std::begin(signal), std::end(signal), std::begin(result), [&tic](const auto &v) { return v / tic; });

    CPPUNIT_ASSERT(std::equal(std::begin(result), std::end(result), std::begin(expected)));
  }
};

MITK_TEST_SUITE_REGISTRATION(m2Calibration)
