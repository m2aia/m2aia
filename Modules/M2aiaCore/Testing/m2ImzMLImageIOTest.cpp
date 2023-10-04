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
#include <signal/m2Normalization.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2TestingConfig.h>
#include <m2TestFixture.h>
#include <mitkTestingMacros.h>
#include <numeric>
#include <random>

//#include <boost/algorithm/string.hpp>

class m2ImzMLImageIOTestSuite : public m2::TestFixture
{
  CPPUNIT_TEST_SUITE(m2ImzMLImageIOTestSuite);
  MITK_TEST(LoadTestData_shouldReturnTrue);
  MITK_TEST(InitializeImageAccess_shouldReturnTrue);

  CPPUNIT_TEST_SUITE_END();

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
    const std::vector<T> &vec,
    const std::string &fileNameInM2aiaDir,
    char delim = '\n')
  {
    std::ofstream f(GetTestDataFilePath(fileNameInM2aiaDir, M2AIA_DATA_DIR));

    for (unsigned int i = 0; i < vec.size(); i++)
    {
      f << std::setprecision(128) << vec[i];
      if (i < vec.size() - 1)
        f << delim;
    }
  }

public:
  void LoadTestData_shouldReturnTrue()
  {
    auto v = mitk::IOUtil::Load(GetTestDataFilePath("lipid.imzML", M2AIA_DATA_DIR));
    m2::ImzMLSpectrumImage::Pointer imzMLImage = dynamic_cast<m2::ImzMLSpectrumImage *>(v.back().GetPointer());
    CPPUNIT_ASSERT_ASSERTION_PASS(CPPUNIT_ASSERT(imzMLImage != nullptr));
  }

  void InitializeImageAccess_shouldReturnTrue()
  {
    auto v = mitk::IOUtil::Load(GetTestDataFilePath("lipid.imzML", M2AIA_DATA_DIR));
    m2::ImzMLSpectrumImage::Pointer imzMLImage = dynamic_cast<m2::ImzMLSpectrumImage *>(v.back().GetPointer());
    imzMLImage->SetNormalizationStrategy(m2::NormalizationStrategyType::None);
    imzMLImage->SetBaselineCorrectionStrategy(m2::BaselineCorrectionType::None);
    imzMLImage->SetSmoothingStrategy(m2::SmoothingType::None);
    imzMLImage->InitializeImageAccess();
    CPPUNIT_ASSERT_ASSERTION_PASS(CPPUNIT_ASSERT(imzMLImage != nullptr));

    std::vector<float> ints, mzs;
    imzMLImage->GetSpectrumFloat(0, mzs, ints);
    WriteVector(ints, "lipid_spectrum0.data");
    auto reference = ReadDoubleVector("lipid_spectrum0.data");

    using namespace std;
    CPPUNIT_ASSERT_EQUAL(true, equal(begin(ints), end(ints), begin(reference)));
	
  }
};

MITK_TEST_SUITE_REGISTRATION(m2ImzMLImageIO)
