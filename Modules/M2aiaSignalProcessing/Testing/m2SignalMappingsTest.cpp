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
#include <m2SignalCommon.h>
#include <mitkTestFixture.h>
#include <mitkTestingMacros.h>

//#include <boost/algorithm/string.hpp>

class m2SignalMappingsTestSuite : public mitk::TestFixture
{
  CPPUNIT_TEST_SUITE(m2SignalMappingsTestSuite);
  MITK_TEST(CreateSwitches);

  CPPUNIT_TEST_SUITE_END();

private:
  void Switch(m2::NormalizationStrategyType e)
  {
#pragma push
#ifdef _WIN32
#pragma warning(error : 4062)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch"
#endif
    switch (e)
    {
      case m2::NormalizationStrategyType::Median:
      case m2::NormalizationStrategyType::TIC:
      case m2::NormalizationStrategyType::Sum:
      case m2::NormalizationStrategyType::InFile:
      case m2::NormalizationStrategyType::None:
        // add your new case here
        {
          //////////////////////////////////////////////////////////////////////
          /////////////////// ATTENTION ////////////////////////////////////////
          //////////////////////////////////////////////////////////////////////
          // It looks like you have added a new enum value to the list.
          // Ensure that all enum values are well defined in the m2::SIGNAL_MAPPINGS.
          // m2::SIGNAL_MAPPINGS is defined in <m2SignalCommon.h>.
          // m2::SIGNAL_MAPPINGS is used to transform strings into unsigned int values.
          //////////////////////////////////////////////////////////////////////
          /////////////////// ATTENTION ////////////////////////////////////////
          //////////////////////////////////////////////////////////////////////
        }
    }
#pragma pop
#pragma GCC diagnostic pop
  }

  void Switch(m2::SmoothingType e)
  {
#pragma push
#ifdef _WIN32
#pragma warning(error : 4062)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch"
#endif
    switch (e)
    {
      case m2::SmoothingType::None:
      case m2::SmoothingType::Gaussian:
      case m2::SmoothingType::SavitzkyGolay:
        // add your new case here
        {
          //////////////////////////////////////////////////////////////////////
          /////////////////// ATTENTION ////////////////////////////////////////
          //////////////////////////////////////////////////////////////////////
          // It looks like you have added a new enum value to the list.
          // Ensure that all enum values are well defined in the m2::SIGNAL_MAPPINGS.
          // m2::SIGNAL_MAPPINGS is defined in <m2SignalCommon.h>.
          // m2::SIGNAL_MAPPINGS is used to transform strings into unsigned int values.
          //////////////////////////////////////////////////////////////////////
          /////////////////// ATTENTION ////////////////////////////////////////
          //////////////////////////////////////////////////////////////////////
        }
    }
#pragma pop
#pragma GCC diagnostic pop
  }

  void Switch(m2::RangePoolingStrategyType e)
  {
#pragma push
#ifdef _WIN32
#pragma warning(error : 4062)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch"
#endif
    switch (e)
    {
      case m2::RangePoolingStrategyType::None:
      case m2::RangePoolingStrategyType::Maximum:
      case m2::RangePoolingStrategyType::Sum:
      case m2::RangePoolingStrategyType::Median:
      case m2::RangePoolingStrategyType::Mean:
        // add your new case here
        {
          //////////////////////////////////////////////////////////////////////
          /////////////////// ATTENTION ////////////////////////////////////////
          //////////////////////////////////////////////////////////////////////
          // It looks like you have added a new enum value to the list.
          // Ensure that all enum values are well defined in the m2::SIGNAL_MAPPINGS.
          // m2::SIGNAL_MAPPINGS is defined in <m2SignalCommon.h>.
          // m2::SIGNAL_MAPPINGS is used to transform strings into unsigned int values.
          //////////////////////////////////////////////////////////////////////
          /////////////////// ATTENTION ////////////////////////////////////////
          //////////////////////////////////////////////////////////////////////
        }
    }
#pragma pop
#pragma GCC diagnostic pop
  }

  void Switch(m2::BaselineCorrectionType e)
  {
#pragma push
#ifdef _WIN32
#pragma warning(error : 4062)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch"
#endif
    switch (e)
    {
      case m2::BaselineCorrectionType::None:
      case m2::BaselineCorrectionType::TopHat:
      case m2::BaselineCorrectionType::Median:

        // add your new case here
        {
          //////////////////////////////////////////////////////////////////////
          /////////////////// ATTENTION ////////////////////////////////////////
          //////////////////////////////////////////////////////////////////////
          // It looks like you have added a new enum value to the list.
          // Ensure that all enum values are well defined in the m2::SIGNAL_MAPPINGS.
          // m2::SIGNAL_MAPPINGS is defined in <m2SignalCommon.h>.
          // m2::SIGNAL_MAPPINGS is used to transform strings into unsigned int values.
          //////////////////////////////////////////////////////////////////////
          /////////////////// ATTENTION ////////////////////////////////////////
          //////////////////////////////////////////////////////////////////////
        }
    }
#pragma pop
#pragma GCC diagnostic pop
  }

public:
  void CreateSwitches()
  {
    Switch(m2::NormalizationStrategyType::None);
    Switch(m2::SmoothingType::None);
    Switch(m2::RangePoolingStrategyType::None);
    Switch(m2::BaselineCorrectionType::None);
  }
};

MITK_TEST_SUITE_REGISTRATION(m2SignalMappings)
