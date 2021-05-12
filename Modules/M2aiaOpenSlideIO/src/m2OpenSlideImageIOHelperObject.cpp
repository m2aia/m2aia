/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#include <itkMetaDataObject.h>
#include <m2OpenSlideImageIOHelperObject.h>

m2::OpenSlideImageIOHelperObject::~OpenSlideImageIOHelperObject() {}
m2::OpenSlideImageIOHelperObject::OpenSlideImageIOHelperObject()
{
  if (!m_OpenSlideIO)
    m_OpenSlideIO = itk::OpenSlideImageIO::New();
}

itk::OpenSlideImageIO *m2::OpenSlideImageIOHelperObject::GetOpenSlideIO()
{
  return m_OpenSlideIO;
}

void m2::OpenSlideImageIOHelperObject::SetRequestedRegionToLargestPossibleRegion(){};

bool m2::OpenSlideImageIOHelperObject::RequestedRegionIsOutsideOfTheBufferedRegion()
{
  return false;
}

bool m2::OpenSlideImageIOHelperObject::VerifyRequestedRegion()
{
  return true;
}

void m2::OpenSlideImageIOHelperObject::SetRequestedRegion(const itk::DataObject * /*data*/) {}

void m2::OpenSlideImageIOHelperObject::ParesOpenSlideLevelsToMap()
{
  auto metadata = m_OpenSlideIO->GetMetaDataDictionary();

  for (auto &key : metadata.GetKeys())
  {
    auto valueObject = metadata.Get(key);
    if (const auto value_object = dynamic_cast<const itk::MetaDataObject<std::string> *>(valueObject))
    {
      const std::string value_string = value_object->GetMetaDataObjectValue();
      const std::string levelKeyStartPart = "openslide.level[";
      auto pos = key.find(levelKeyStartPart);
      if (pos != std::string::npos) // found
      {
        unsigned int i = std::stoi(key.substr(levelKeyStartPart.size(), 1)); // get level index
        std::string levelKey = key.substr(levelKeyStartPart.size() + 3);     // get level key
        m_Levels[i][levelKey] = value_string;
      }
    }
  }
}

const std::map<unsigned int, std::map<std::string, std::string>> &m2::OpenSlideImageIOHelperObject::GetLevelsMap() const
{
  return m_Levels;
}
