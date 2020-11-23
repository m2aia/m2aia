/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2CoreObjectFactory.h>

#include <m2ImzMLMassSpecImage.h>

#include <mitkLevelWindowProperty.h>
#include <mitkLookupTableProperty.h>

m2::CoreObjectFactory::CoreObjectFactory() : mitk::CoreObjectFactoryBase()
{
  static bool alreadyDone = false;
  if (!alreadyDone)
  {
    MITK_DEBUG << "ImagingMassSpectrometryObjectFactory c'tor" << std::endl;

    CreateFileExtensionsMap();

    alreadyDone = true;
  }
}

m2::CoreObjectFactory::~CoreObjectFactory() {}

mitk::Mapper::Pointer m2::CoreObjectFactory::CreateMapper(mitk::DataNode * /*node*/,
                                                                               MapperSlotId /*id*/)
{
  return nullptr;
}

void m2::CoreObjectFactory::SetDefaultProperties(mitk::DataNode * /*node*/) {}

std::string m2::CoreObjectFactory::GetFileExtensions()
{
  std::string fileExtension;
  this->CreateFileExtensions(m_FileExtensionsMap, fileExtension);
  return fileExtension.c_str();
}

mitk::CoreObjectFactoryBase::MultimapType m2::CoreObjectFactory::GetFileExtensionsMap()
{
  return m_FileExtensionsMap;
}

mitk::CoreObjectFactoryBase::MultimapType m2::CoreObjectFactory::GetSaveFileExtensionsMap()
{
  return m_SaveFileExtensionsMap;
}

void m2::CoreObjectFactory::CreateFileExtensionsMap() {}

std::string m2::CoreObjectFactory::GetSaveFileExtensions()
{
  std::string fileExtension;
  this->CreateFileExtensions(m_SaveFileExtensionsMap, fileExtension);
  return fileExtension.c_str();
}

struct Register_m2CoreObjectFactory
{
  Register_m2CoreObjectFactory() : m_Factory(m2::CoreObjectFactory::New())
  {
    mitk::CoreObjectFactory::GetInstance()->RegisterExtraFactory(m_Factory);
  }

  ~Register_m2CoreObjectFactory()
  {
    mitk::CoreObjectFactory::GetInstance()->UnRegisterExtraFactory(m_Factory);
  }
  m2::CoreObjectFactory::Pointer m_Factory;
};

static Register_m2CoreObjectFactory Register_m2CoreObjectFactory;
