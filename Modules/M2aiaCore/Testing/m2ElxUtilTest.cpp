/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

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

  const std::array<unsigned short, 100> data = {
    0,  0, 0, 0, 10, 10, 0, 0, 0, 0,  0,  0, 0, 0, 10, 10, 0, 0, 0, 0,  0,  0, 0, 0, 10,
    10, 0, 0, 0, 0,  0,  0, 0, 0, 10, 10, 0, 0, 0, 0,  0,  0, 0, 0, 10, 10, 0, 0, 0, 0,
    0,  0, 0, 0, 10, 10, 0, 0, 0, 0,  0,  0, 0, 0, 10, 10, 0, 0, 0, 0,  0,  0, 0, 0, 10,
    10, 0, 0, 0, 0,  0,  0, 0, 0, 10, 10, 0, 0, 0, 0,  0,  0, 0, 0, 10, 10, 0, 0, 0, 0,
  };

  const std::array<unsigned short, 100> data2 = {
    0,  0,  10, 10, 0, 0, 0, 0, 0, 0, 0, 0,  0,  10, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0,  10, 10, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  10, 10, 0,  0, 0, 0, 0, 0, 0, 0, 0,  10, 10, 0,  0, 0, 0, 0, 0, 0, 0, 0, 10, 10, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    10, 10, 0,  0,  0, 0, 0, 0, 0, 0, 0, 10, 0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0, 0, 0, 0, 0, 0,
  };

  void FindElastixExecutables() { m2::ElxUtil::Executable("elastix"); }

  void CheckGetRegistrationInputs_10x10x10_ThrowException()
  {
    auto itkNoSlice3D = ItkType3D::New();
    itkNoSlice3D->SetLargestPossibleRegion({{0, 0, 0}, {10, 10, 10}});
    itkNoSlice3D->Allocate(0);
    itkNoSlice3D->Initialize();
    auto mitkNoSlice3D = mitk::Image::New();
    mitkNoSlice3D->InitializeByItk(itkNoSlice3D.GetPointer());

    CPPUNIT_ASSERT_THROW(m2::ElxUtil::GetRegistration(mitkNoSlice3D, mitkNoSlice3D, {""}), mitk::Exception);
  }

  void CheckGetRegistrationInputs_10x10x10x1_ThrowException()
  {
    auto itkNoSlice4D = ItkType4D::New();
    itkNoSlice4D->SetLargestPossibleRegion({{0, 0, 0, 0}, {10, 10, 10, 1}});
    itkNoSlice4D->Allocate(0);
    itkNoSlice4D->Initialize();
    auto mitkNoSlice4D = mitk::Image::New();
    mitkNoSlice4D->InitializeByItk(itkNoSlice4D.GetPointer());

    CPPUNIT_ASSERT_THROW(m2::ElxUtil::GetRegistration(mitkNoSlice4D, mitkNoSlice4D, {""}), mitk::Exception);
  }

  void CheckGetRegistrationInputs_10x10x1_NoThrowException()
  {
    auto itkNoSlice3D = ItkType3D::New();
    itkNoSlice3D->SetLargestPossibleRegion({{0, 0, 0}, {10, 10, 1}});
    itkNoSlice3D->Allocate(0);
    itkNoSlice3D->Initialize();
    auto mitkNoSlice3D = mitk::Image::New();
    mitkNoSlice3D->InitializeByItk(itkNoSlice3D.GetPointer());

    CPPUNIT_ASSERT_NO_THROW(m2::ElxUtil::GetRegistration(mitkNoSlice3D, mitkNoSlice3D, {""}));
  }

  void CheckGetRegistrationInputs_10x10_NoThrowException()
  {
    auto itkSlice2D = ItkType2D::New();
    itkSlice2D->SetLargestPossibleRegion({{0, 0}, {10, 10}});
    itkSlice2D->Allocate(0);
    itkSlice2D->Initialize();
    auto mitkSlice2D = mitk::Image::New();
    mitkSlice2D->InitializeByItk(itkSlice2D.GetPointer());

    CPPUNIT_ASSERT_NO_THROW(m2::ElxUtil::GetRegistration(mitkSlice2D, mitkSlice2D, {""}));
  }

  void CheckGetRegistrationOutput_10x10_NoThrowException()
  {
    auto itkFixed = ItkType2D::New();
    itkFixed->SetLargestPossibleRegion({{0, 0}, {10, 10}});
    itkFixed->Allocate();
    auto itkMoving = ItkType2D::New();
    itkMoving->SetLargestPossibleRegion({{0, 0}, {10, 10}});
    itkMoving->Allocate();

    auto fixed = mitk::Image::New();
    fixed->InitializeByItk(itkFixed.GetPointer());
    auto moving = mitk::Image::New();
    moving->InitializeByItk(itkMoving.GetPointer());

    {
      mitk::ImagePixelWriteAccessor<unsigned short, 2> fixedAcc(fixed);
      mitk::ImagePixelWriteAccessor<unsigned short, 2> movingAcc(moving);
      std::copy(data.begin(), data.end(), fixedAcc.GetData());
      std::copy(data2.begin(), data2.end(), movingAcc.GetData());
    }

    auto path = GetTestDataFilePath("rigid.txt", M2AIA_DATA_DIR);

    auto transformations = m2::ElxUtil::GetRegistration(fixed, moving, {path, path});
    // CPPUNIT_ASSERT_EQUAL(, transformations.empty());
  }
};

MITK_TEST_SUITE_REGISTRATION(m2ElxUtil)
