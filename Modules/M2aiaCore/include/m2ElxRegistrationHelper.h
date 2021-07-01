/*===================================================================
MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#pragma once

#include <M2aiaCoreExports.h>
#include <mitkImage.h>
#include <mitkPointSet.h>
#include <string>
#include <vector>

namespace m2
{
  class M2AIACORE_EXPORT ElxRegistrationHelper
  {
  protected:
    mitk::Image::Pointer m_FixedImage;
    mitk::Image::Pointer m_FixedMask;
    mitk::PointSet::Pointer m_FixedPoints;

    mitk::Image::Pointer m_MovingImage;
    mitk::Image::Pointer m_MovingMask;
    mitk::PointSet::Pointer m_MovingPoints;

    std::vector<std::string> m_Transformations;
    std::vector<std::string> m_RegistrationParameters = {""};

    bool m_UseMasksForRegistration = false;
    bool m_UsePointsForRegistration = false;
    bool m_RemoveWorkingDirectory = true;
    std::string m_WorkingDirectory = "";

    static bool CheckDimensions(mitk::Image *fixed, mitk::Image *moving);
    static mitk::Image::Pointer GetSliceData(mitk::Image *);

  public:
    void SetImageData(mitk::Image *fixed, mitk::Image *moving);
    void SetMaskData(mitk::Image *fixed, mitk::Image *moving);
    void SetPointData(mitk::PointSet *fixed, mitk::PointSet *moving);
    void SetDirectory(const std::string &dir);
    void SetRegistrationParameters(const std::vector<std::string> &);
    void SetRemoveWorkingdirectory(bool val);

    std::vector<std::string> GetRegistration();
    mitk::Image::Pointer WarpMask(mitk::Image *);
    mitk::Image::Pointer WarpImage(mitk::Image *);
    mitk::Image::Pointer WarpPoints(mitk::PointSet *);
  };

} // namespace m2