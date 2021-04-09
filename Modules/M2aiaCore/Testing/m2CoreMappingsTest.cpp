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
#include <m2CoreCommon.h>
#include <mitkTestFixture.h>
#include <mitkTestingMacros.h>

//#include <boost/algorithm/string.hpp>

class m2CoreMappingsTestSuite : public mitk::TestFixture
{
  CPPUNIT_TEST_SUITE(m2CoreMappingsTestSuite);
  MITK_TEST(CreateSwitches);

  CPPUNIT_TEST_SUITE_END();

private:
  void Switch(m2::SpectrumFormatType e)
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
      case m2::SpectrumFormatType::ContinuousCentroid:
      case m2::SpectrumFormatType::ContinuousProfile:
      case m2::SpectrumFormatType::ProcessedCentroid:
      case m2::SpectrumFormatType::ProcessedProfile:
      case m2::SpectrumFormatType::None:
        // add your new case here
        {
          //////////////////////////////////////////////////////////////////////
          /////////////////// ATTENTION ////////////////////////////////////////
          //////////////////////////////////////////////////////////////////////
          // It looks like you have added a new enum value to the list.
          // Ensure that all enum values are well defined in the m2::CORE_MAPPINGS.
          // m2::CORE_MAPPINGS is defined in <m2CoreCommon.h>.
          // m2::CORE_MAPPINGS is used to transform strings into unsigned int values.
          //////////////////////////////////////////////////////////////////////
          /////////////////// ATTENTION ////////////////////////////////////////
          //////////////////////////////////////////////////////////////////////
        }
    }
#pragma pop
#pragma GCC diagnostic pop
  }

  void Switch(m2::OverviewSpectrumType e)
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
      case m2::OverviewSpectrumType::None:
      case m2::OverviewSpectrumType::Maximum:
      case m2::OverviewSpectrumType::Mean:
      case m2::OverviewSpectrumType::Median:
      case m2::OverviewSpectrumType::PeakIndicators:
      case m2::OverviewSpectrumType::Sum:
      case m2::OverviewSpectrumType::Variance:
        // add your new case here
        {
          //////////////////////////////////////////////////////////////////////
          /////////////////// ATTENTION ////////////////////////////////////////
          //////////////////////////////////////////////////////////////////////
          // It looks like you have added a new enum value to the list.
          // Ensure that all enum values are well defined in the m2::CORE_MAPPINGS.
          // m2::CORE_MAPPINGS is defined in <m2CoreCommon.h>.
          // m2::CORE_MAPPINGS is used to transform strings into unsigned int values.
          //////////////////////////////////////////////////////////////////////
          /////////////////// ATTENTION ////////////////////////////////////////
          //////////////////////////////////////////////////////////////////////
        }
    }
#pragma pop
#pragma GCC diagnostic pop
  }

  void Switch(m2::NumericType e)
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
      case m2::NumericType::Float:
      case m2::NumericType::Double:

        // add your new case here
        {
          //////////////////////////////////////////////////////////////////////
          /////////////////// ATTENTION ////////////////////////////////////////
          //////////////////////////////////////////////////////////////////////
          // It looks like you have added a new enum value to the list.
          // Ensure that all enum values are well defined in the m2::CORE_MAPPINGS.
          // m2::CORE_MAPPINGS is defined in <m2CoreCommon.h>.
          // m2::CORE_MAPPINGS is used to transform strings into unsigned int values.
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
    Switch(m2::SpectrumFormatType::None);
    Switch(m2::NumericType::Float);
    Switch(m2::OverviewSpectrumType::None);
    
  }
};

MITK_TEST_SUITE_REGISTRATION(m2CoreMappings)
