  /*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "m2ImzMLImageSerializer.h"
#include "mitkIOUtil.h"
#include <m2ImzMLSpectrumImage.h>

#include <itksys/SystemTools.hxx>

MITK_REGISTER_SERIALIZER(ImzMLSpectrumImageSerializer)

mitk::ImzMLSpectrumImageSerializer::ImzMLSpectrumImageSerializer()
{
}

mitk::ImzMLSpectrumImageSerializer::~ImzMLSpectrumImageSerializer()
{
}

std::string mitk::ImzMLSpectrumImageSerializer::Serialize()
{
  const auto * data = dynamic_cast<const m2::ImzMLSpectrumImage*>( m_Data.GetPointer() );
  if (!data)
  {
    MITK_ERROR << " Object at " << (const void*) this->m_Data
              << " is not an m2::ImzMLSpectrumImage. Cannot serialize as imzML image.";
    return "";
  }

  std::string filename( this->GetUniqueFilenameInWorkingDirectory() );
  filename += "_";
  filename += m_FilenameHint;
  filename += ".imzML";

  std::string fullname(m_WorkingDirectory);
  fullname += "/";
  fullname += itksys::SystemTools::ConvertToOutputPath(filename.c_str());

  try
  {
    mitk::IOUtil::Save(const_cast<m2::ImzMLSpectrumImage*>( data ),fullname);
  }
  catch ( const std::exception& e )
  {
    MITK_ERROR << " Error serializing object at " << (const void*) this->m_Data
              << " to "
              << fullname
              << ": "
              << e.what();
    return "";
  }

  return filename;
}

