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

#include "M2aiaCoreExports.h"
#include "mitkCoreObjectFactory.h"
#include "mitkCoreObjectFactoryBase.h"
#include <mitkAbstractFileIO.h>

namespace m2
{
  class M2AIACORE_EXPORT CoreObjectFactory : public mitk::CoreObjectFactoryBase
  {
  public:
    mitkClassMacro(CoreObjectFactory, mitk::CoreObjectFactoryBase);
    itkFactorylessNewMacro(Self) itkCloneMacro(Self) mitk::Mapper::Pointer
      CreateMapper(mitk::DataNode *node, MapperSlotId slotId) override;
    void SetDefaultProperties(mitk::DataNode *node) override;
	std::string GetFileExtensions() override;
    mitk::CoreObjectFactory::MultimapType GetFileExtensionsMap() override;
	std::string GetSaveFileExtensions() override;
    mitk::CoreObjectFactory::MultimapType GetSaveFileExtensionsMap() override;

  protected:
    CoreObjectFactory();
    ~CoreObjectFactory() override;
    void CreateFileExtensionsMap();
    MultimapType m_FileExtensionsMap;
    MultimapType m_SaveFileExtensionsMap;

  private:
    std::vector<mitk::AbstractFileIO *> m_FileIOs;
  };
} // namespace mitk
