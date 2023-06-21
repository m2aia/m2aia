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
  void Switch(m2::SpectrumFormat e)
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
      case m2::SpectrumFormat::ContinuousCentroid:
      case m2::SpectrumFormat::ContinuousProfile:
      case m2::SpectrumFormat::ProcessedCentroid:
      case m2::SpectrumFormat::ProcessedProfile:
      case m2::SpectrumFormat::Profile:
      case m2::SpectrumFormat::Centroid:
      case m2::SpectrumFormat::None:
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

  void Switch(m2::SpectrumType e)
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
      case m2::SpectrumType::None:
      case m2::SpectrumType::Maximum:
      case m2::SpectrumType::Mean:
      case m2::SpectrumType::Median:
      case m2::SpectrumType::Sum:
      case m2::SpectrumType::Variance:
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
    Switch(m2::SpectrumFormat::None);
    Switch(m2::NumericType::Float);
    Switch(m2::SpectrumType::None);
    
  }
};

MITK_TEST_SUITE_REGISTRATION(m2CoreMappings)
