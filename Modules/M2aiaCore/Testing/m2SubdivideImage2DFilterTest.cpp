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

#include <m2TestingConfig.h>
#include <mitkTestFixture.h>
#include <mitkTestingMacros.h>
#include <itkCropImageFilter.h>
#include <m2SubdivideImage2DFilter.h>

//#include <boost/algorithm/string.hpp>

class m2SubdivideImage2DFilterTestSuite : public mitk::TestFixture
{
  CPPUNIT_TEST_SUITE(m2SubdivideImage2DFilterTestSuite);
  MITK_TEST(ApplyFilterToImageWithResiduals);

  CPPUNIT_TEST_SUITE_END();

private:
 
public:
  void ApplyFilterToImageWithResiduals()
  {
    auto v = mitk::IOUtil::Load(GetTestDataFilePath("histo_slice.nrrd",M2AIA_DATA_DIR));
    auto filter = m2::SubdivideImage2DFilter::New();
    filter->SetInput(static_cast<mitk::Image*>(v.back().GetPointer()));
    filter->SetTileWidth(unsigned(1) << 13);
    filter->SetTileHeight(unsigned(1) << 13);
    filter->Update();

    auto outputs = filter->GetOutputs();
    unsigned i = 0;
    for(auto I : outputs){
      MITK_INFO << static_cast<mitk::Image*>(I.GetPointer())->GetGeometry()->GetOrigin();
      mitk::IOUtil::Save(static_cast<mitk::Image *>(I.GetPointer()), "/home/jtfc/HS/data/ddd_" + std::to_string(i++) + ".nrrd");
    }

    
    
  }

  
};

MITK_TEST_SUITE_REGISTRATION(m2SubdivideImage2DFilter)
