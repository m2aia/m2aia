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


    /**
    *  @brief ConvertForElastixProcessing function takes a pointer to a mitk::Image as input and returns a pointer to a new mitk::Image object. 
    *  The function first checks if the input image pointer is not null, and if it is not, it proceeds to check the dimension of the image.
    *  - If the dimension of the image is 3 and the size in the Z dimension is 1, the function creates a new 2D image using ITK and copies the data, spacing, origin and direction from the input image to the new 2D image. The function then returns the new 2D image.
    *  - If the dimension of the image is 2, the function returns the input image as it is.
    *  - If the dimension of the image is 3 and the size in the Z dimension is greater than 1, the function returns the input image as it is.
    *  - If the dimension of the image is 4, the function creates a new 3D image and copies the data, spacing, origin and direction from the input image to the new 3D image. The function then returns the new 3D image.
    *  - If the input image pointer is null, the function throws an exception with the message "Image data is null!".
    *  @note that this function is not well-suited for handling 4D+t images, and only the first time step is selected.
    *  @param image pointer to a mitk::Image.
    *  @return pointer to a new mitk::Image object.
    */
    mitk::Image::Pointer ConvertForElastixProcessing(const mitk::Image *) const;

    /**
    *  @brief Get a 3D image slice from a 2D image.
    *  - If the input image is 2D, a new 3D image is created with the same properties as the input image, and the 2D image data is copied to the first slice of the new image.
    *  - If the input image is already 3D, the input image is returned.
    *  @param image The input image.
    *  @return mitk::Image::Pointer A 3D image slice.
    *  @throws mitk::Exception if the input image is null.
    */
    mitk::Image::Pointer ConvertForM2aiaProcessing(const mitk::Image *) const;

    void CreateWorkingDirectory() const;
    void RemoveWorkingDirectory() const;
    void SymlinkOrWriteNrrd(mitk::Image::Pointer image, std::string targetPath) const;

    std::function<void(std::string)> m_StatusFunction = [](std::string){};
    

  public:
    // Registration
    void SetImageData(mitk::Image *fixed, mitk::Image *moving);
    void SetFixedImageMaskData(mitk::Image *fixed);
    void SetPointData(mitk::PointSet *fixed, mitk::PointSet *moving);
    void SetDirectory(const std::string &dir);
    void SetRegistrationParameters(const std::vector<std::string> &);
    void SetRemoveWorkingDirectory(bool val);
    void SetAdditionalBinarySearchPath(const std::string &list);
    virtual ~ElxRegistrationHelper();
    void UseMovingImageSpacing(bool val){this->m_UseMovingImageSpacing = val;};

    void GetRegistration();
    std::vector<std::string> GetTransformation() const;
    void SetTransformations(const std::vector<std::string> & trafos);
    void SetStatusCallback(const std::function<void(std::string)> & callback);
    
    mitk::Image::Pointer GetFixedImage() const{
      return m_FixedImage;
    }

    mitk::Image::Pointer GetMovingImage() const{
      return m_MovingImage;
    }
    
    mitk::Image::Pointer WarpImage(const mitk::Image * image,
                                   const std::string &type = "float",
                                   const unsigned char &interpolationOrder = 3) const;
    mitk::Image::Pointer WarpPoints(mitk::PointSet *);
  };

} // namespace m2