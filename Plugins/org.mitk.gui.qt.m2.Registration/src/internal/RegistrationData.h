
/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#pragma once

#include <mitkImage.h>
#include <mitkPointSet.h>
#include <mitkBaseData.h>



struct RegistrationData : public mitk::BaseData{
    mitk::Image::Pointer m_Image;
    mitk::Image::Pointer m_Mask;
    mitk::PointSet::Pointer m_PointSet;
    std::string m_Name;
    
    std::vector<std::string> m_Transformations;
    RegistrationData * m_RelatedData;

    void SetRequestedRegionToLargestPossibleRegion() override {};
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override {return true;};
    bool VerifyRequestedRegion() override {return false;};
    void SetRequestedRegion(const itk::DataObject *) override {};
};

