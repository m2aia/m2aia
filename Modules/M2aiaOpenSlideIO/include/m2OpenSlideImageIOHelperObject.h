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

#include <M2aiaOpenSlideIOExports.h>
#include <itkOpenSlideImageIO.h>
#include <mitkBaseData.h>
#include <mitkImage.h>

namespace m2
{
  class M2AIAOPENSLIDEIO_EXPORT OpenSlideImageIOHelperObject : public mitk::BaseData
  {
  public:
    mitkClassMacro(OpenSlideImageIOHelperObject, mitk::BaseData);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

    ~OpenSlideImageIOHelperObject();
    OpenSlideImageIOHelperObject();

    virtual itk::OpenSlideImageIO *GetOpenSlideIO();

    //##Documentation
    //## @brief Set the RequestedRegion to the LargestPossibleRegion.
    void SetRequestedRegionToLargestPossibleRegion() override;

    //##Documentation
    //## @brief Determine whether the RequestedRegion is outside of the BufferedRegion.
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override;

    //##Documentation
    //## @brief Verify that the RequestedRegion is within the LargestPossibleRegion.
    bool VerifyRequestedRegion() override;

    //##Documentation
    //## @brief Set the requested region from this data object to match the requested
    //## region of the data object passed in as a parameter.
    void SetRequestedRegion(const itk::DataObject *data) override;

    //## Documentation
    //## @brief Reads meta data dictionary and parses level dependent informations
	void ParesOpenSlideLevelsToMap();

    //## Documentation
    //## @brief Returns the map of levels and associated details
    const std::map<unsigned int, std::map<std::string, std::string>> &GetLevelsMap() const;

  protected:
    itk::OpenSlideImageIO::Pointer m_OpenSlideIO = nullptr;
    std::map<unsigned int, std::map<std::string, std::string>> m_Levels;

  }; // namespace m2

} // namespace m2
