/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#pragma once

#include <mitkBaseDataSerializer.h>
#include <M2aiaCoreIOExports.h>

namespace mitk
{

class M2AIACOREIO_EXPORT ImzMLSpectrumImageSerializer : public BaseDataSerializer
{
public:
  mitkClassMacro(ImzMLSpectrumImageSerializer, BaseDataSerializer);
  itkFactorylessNewMacro(Self);
  itkCloneMacro(Self);

  std::string Serialize() override;

protected:

  ImzMLSpectrumImageSerializer();
  ~ImzMLSpectrumImageSerializer() override;
};

}

