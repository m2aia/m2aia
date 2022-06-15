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
#include <mitkImageCast.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkTestFixture.h>
#include <mitkTestingMacros.h>

//#include <boost/algorithm/string.hpp>

class m2ElxUtilTestSuite : public mitk::TestFixture
{
  CPPUNIT_TEST_SUITE(m2ElxUtilTestSuite);
  MITK_TEST(FindElastixExecutables);
  MITK_TEST(CheckGetRegistrationInputs_10x10x10_ThrowException);
  MITK_TEST(CheckGetRegistrationInputs_10x10x1_NoThrowException);
  MITK_TEST(CheckGetRegistrationInputs_10x10_NoThrowException);
  MITK_TEST(CheckGetRegistrationOutput_10x10_NoThrowException);
  MITK_TEST(CheckGetRegistrationInputs_10x10x10x1_ThrowException);
  CPPUNIT_TEST_SUITE_END();

public:
  using ItkType4D = itk::Image<unsigned short, 4>;
  using ItkType3D = itk::Image<unsigned short, 3>;
  using ItkType2D = itk::Image<unsigned short, 2>;

  void FindElastixExecutables() { m2::ElxUtil::Executable("elastix"); }

  void CheckGetRegistrationInputs_10x10x10_ThrowException()
  {
    auto itkNoSlice3D = ItkType3D::New();
    itkNoSlice3D->SetRegions({{0, 0, 0}, {10, 10, 10}});
    itkNoSlice3D->Allocate(0);
    itkNoSlice3D->Initialize();
    auto mitkNoSlice3D = mitk::Image::New();
    mitkNoSlice3D->InitializeByItk(itkNoSlice3D.GetPointer());

    m2::ElxRegistrationHelper helper;
    CPPUNIT_ASSERT_THROW(helper.SetImageData(mitkNoSlice3D, mitkNoSlice3D), mitk::Exception);
  }

  void CheckGetRegistrationInputs_10x10x10x1_ThrowException()
  {
    auto itkNoSlice4D = ItkType4D::New();
    itkNoSlice4D->SetRegions({{0, 0, 0, 0}, {10, 10, 10, 1}});
    itkNoSlice4D->Allocate(0);
    itkNoSlice4D->Initialize();
    auto mitkNoSlice4D = mitk::Image::New();
    mitkNoSlice4D->InitializeByItk(itkNoSlice4D.GetPointer());

    m2::ElxRegistrationHelper helper;
    CPPUNIT_ASSERT_THROW(helper.SetImageData(mitkNoSlice4D, mitkNoSlice4D), mitk::Exception);
  }

  void CheckGetRegistrationInputs_10x10x1_NoThrowException()
  {
    auto itkSlice3D = ItkType3D::New();
    itkSlice3D->SetRegions({{0, 0, 0}, {10, 10, 1}});
    itkSlice3D->Allocate(0);
    itkSlice3D->Initialize();
    auto mitkSlice3D = mitk::Image::New();
    mitkSlice3D->InitializeByItk(itkSlice3D.GetPointer());

    m2::ElxRegistrationHelper helper;
    CPPUNIT_ASSERT_NO_THROW(helper.SetImageData(mitkSlice3D, mitkSlice3D));
  }

  void CheckGetRegistrationInputs_10x10_NoThrowException()
  {
    auto itkSlice2D = ItkType2D::New();
    itkSlice2D->SetRegions({{0, 0}, {10, 10}});
    itkSlice2D->Allocate(0);
    itkSlice2D->Initialize();
    auto mitkSlice2D = mitk::Image::New();
    mitkSlice2D->InitializeByItk(itkSlice2D.GetPointer());

    m2::ElxRegistrationHelper helper;
    CPPUNIT_ASSERT_THROW(helper.SetImageData(mitkSlice2D, mitkSlice2D), mitk::Exception);
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
