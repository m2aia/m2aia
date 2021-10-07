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
#include <functional>

namespace m2
{
  /**
   * @brief This class manages file base
   *
   */
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
    std::vector<std::string> m_RegistrationParameters = {}; // forces elastix default
    std::string m_BinarySearchPath = "";

    bool m_UseMasksForRegistration = false;
    bool m_UsePointsForRegistration = false;
    bool m_RemoveWorkingDirectory = true;
    mutable std::string m_WorkingDirectory = "";
    mutable std::string m_ExternalWorkingDirectory = "";
    bool m_UseMovingImageSpacing = false;

    bool CheckDimensions(const mitk::Image *image) const;
    mitk::Image::Pointer GetSlice2DData(const mitk::Image *) const;
    mitk::Image::Pointer GetSlice3DData(const mitk::Image *) const;
    void CreateWorkingDirectory() const;
    void RemoveWorkingDirectory() const;

    std::function<void(std::string)> m_StatusFunction = [](std::string){};
    

  public:
    // Registration
    void SetImageData(mitk::Image *fixed, mitk::Image *moving);
    void SetMaskData(mitk::Image *fixed, mitk::Image *moving);
    void SetPointData(mitk::PointSet *fixed, mitk::PointSet *moving);
    void SetDirectory(const std::string &dir);
    void SetRegistrationParameters(const std::vector<std::string> &);
    void SetRemoveWorkingDirectory(bool val);
    void SetAdditionalBinarySearchPath(const std::string &list);
    virtual ~ElxRegistrationHelper();
    void UseMovingImageSpacing(bool val){this->m_UseMovingImageSpacing = val;};

    void GetRegistration();
    std::vector<std::string> GetTransformation() const;
    void SetStatusCallback(const std::function<void(std::string)> & callback);
    
    mitk::Image::Pointer GetFixedImage() const{
      return m_FixedImage;
    }

    mitk::Image::Pointer GetMovingImage() const{
      return m_MovingImage;
    }
    
    mitk::Image::Pointer WarpImage(const mitk::Image::Pointer image,
                                   const std::string &type = "float",
                                   const unsigned char &interpolationOrder = 3) const;
    mitk::Image::Pointer WarpPoints(mitk::PointSet *);
  };

} // namespace m2