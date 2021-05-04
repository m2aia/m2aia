/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef mitkFastSymmetricForcesDemonsMultiResDefaultRegistrationAlgorithm_h
#define mitkFastSymmetricForcesDemonsMultiResDefaultRegistrationAlgorithm_h

#include "mapDiscreteElements.h"
#include "mapITKFastSymmetricForcesDemonsMultiResRegistrationAlgorithm.h"
#include "mapConfigure.h"

#include "m2FastSymmetricForcesDemonsMultiResDefaultRegistrationAlgorithm_ProfileResource.h"

namespace m2
{
  template <typename TImageType>
  using FastSymmetricForcesDemonsMultiResDefaultRegistrationAlgorithm = map::algorithm::boxed::ITKFastSymmetricForcesDemonsMultiResRegistrationAlgorithm<TImageType, map::algorithm::m2FastSymmetricForcesDemonsMultiResDefaultRegistrationAlgorithmUIDPolicy>;
}

#endif
