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

#include "itkObject.h"
#include "itkObjectFactory.h"
#include "itkSmartPointer.h"
#include "mitkCommon.h"
#include "mitkDataInteractor.h"
#include <M2aiaCoreExports.h>
#include <mitkDisplayInteractor.h>
#include <mitkInteractionEvent.h>
#include <mitkPointSet.h>
#include <mitkStateMachineAction.h>

namespace mitk
{
  // Inherit from DataInteratcor, this provides functionality of a state machine and configurable inputs.
  class M2AIACORE_EXPORT MSDataObjectInteractor : public DataInteractor
  {
  public:
    mitkClassMacro(MSDataObjectInteractor, DataInteractor) itkFactorylessNewMacro(Self) itkCloneMacro(Self)

      protected : MSDataObjectInteractor();
    ~MSDataObjectInteractor() override;

    void ConnectActionsAndFunctions() override;
    void ConfigurationChanged() override;

  private:
    void AddPoint(StateMachineAction *, InteractionEvent *);
  };
} // namespace mitk
