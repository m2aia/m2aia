/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2ElxRegistrationHelper.h>
#include <m2ElxUtil.h>
#include <m2TestingConfig.h>
#include <m2TestFixture.h>
#include <mitkImageCast.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkTestingMacros.h>

//#include <boost/algorithm/string.hpp>

class m2ElxUtilTestSuite : public m2::TestFixture
{
  CPPUNIT_TEST_SUITE(m2ElxUtilTestSuite);
  MITK_TEST(FindElastixExecutables);
  CPPUNIT_TEST_SUITE_END();

public:
  using ItkType4D = itk::Image<unsigned short, 4>;
  using ItkType3D = itk::Image<unsigned short, 3>;
  using ItkType2D = itk::Image<unsigned short, 2>;

  void FindElastixExecutables() { 
    
    CPPUNIT_ASSERT_ASSERTION_PASS(assert(m2::ElxUtil::Executable("elastix").find("elastix") != std::string::npos)); 
  }

   void CheckGetRegistrationOutput_10x10_NoThrowException()
  {
    mitk::Image::Pointer fixed, fixedMask, moving, movingMask;
    auto data = mitk::IOUtil::Load({GetTestDataFilePath("fixed.nrrd", M2AIA_DATA_DIR),
                                    GetTestDataFilePath("fixedMask.nrrd", M2AIA_DATA_DIR),
                                    GetTestDataFilePath("moving.nrrd", M2AIA_DATA_DIR),
                                    GetTestDataFilePath("movingMask.nrrd", M2AIA_DATA_DIR)});

    fixed = dynamic_cast<mitk::Image *>(data.at(0).GetPointer());
    fixedMask = dynamic_cast<mitk::Image *>(data.at(1).GetPointer());
    // fixedMask = nullptr;
    moving = dynamic_cast<mitk::Image *>(data.at(2).GetPointer());
    movingMask = dynamic_cast<mitk::Image *>(data.at(3).GetPointer());
    // movingMask = nullptr;
    const auto rigid = GetTestDataFilePath("rigid.txt", M2AIA_DATA_DIR);
    const auto deformable = GetTestDataFilePath("deformable.txt", M2AIA_DATA_DIR);

    m2::ElxRegistrationHelper helper;
    helper.SetImageData(fixed, moving);
    helper.SetFixedImageMaskData(fixedMask);
    
    {
      helper.SetRegistrationParameters({rigid, deformable});
      helper.GetRegistration();
      CPPUNIT_ASSERT_EQUAL(size_t(2), helper.GetTransformation().size());
    }

    {
      mitk::Image::Pointer d = helper.WarpImage(moving);
      const auto actualSpacing = d->GetGeometry()->GetSpacing()[2];
      CPPUNIT_ASSERT_DOUBLES_EQUAL(0.01, actualSpacing, mitk::eps);
    }
    {
      mitk::Image::Pointer dm = helper.WarpImage(movingMask, "unsigned short", 0);
      const auto actualSpacing = dm->GetGeometry()->GetSpacing()[2];
      CPPUNIT_ASSERT_DOUBLES_EQUAL(0.01, actualSpacing, mitk::eps);
    }
  }
};

MITK_TEST_SUITE_REGISTRATION(m2ElxUtil)
