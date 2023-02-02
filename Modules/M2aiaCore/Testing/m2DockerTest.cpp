/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2DockerHelper.h>
#include <cppunit/TestFixture.h>
#include <m2TestingConfig.h>
#include <mitkImageCast.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkTestFixture.h>
#include <mitkTestingMacros.h>

//#include <boost/algorithm/string.hpp>

class m2DockerTestSuite : public mitk::TestFixture
{
  CPPUNIT_TEST_SUITE(m2DockerTestSuite);
  MITK_TEST(FindDocker);
  MITK_TEST(RunHelloWorldContainer_NoThrow);
  CPPUNIT_TEST_SUITE_END();

public:
  void FindDocker(){
    CPPUNIT_ASSERT(m2::DockerHelper::CheckDocker());
  }

  void RunHelloWorldContainer_NoThrow(){
    m2::DockerHelper helper("hello-world");
    // helper.EnableAutoRemoveImage(true);
    CPPUNIT_ASSERT_NO_THROW(helper.GetResults());
  }

  
};

MITK_TEST_SUITE_REGISTRATION(m2Docker)
