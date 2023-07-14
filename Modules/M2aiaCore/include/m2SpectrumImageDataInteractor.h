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
#include <mitkDataStorage.h>
#include <mitkInteractionEvent.h>
#include <mitkPointSet.h>
#include <mitkStateMachineAction.h>

namespace m2
{
  // Inherit from DataInteratcor, this provides functionality of a state machine and configurable inputs.
  class M2AIACORE_EXPORT SpectrumImageDataInteractor : public mitk::DataInteractor
  {
  public:
    mitkClassMacro(SpectrumImageDataInteractor, mitk::DataInteractor);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

    void SetDataStorage(mitk::DataStorage *ds);
    mitk::DataStorage *GetDataStorage() const;

  protected:
    SpectrumImageDataInteractor();
    ~SpectrumImageDataInteractor() override;

    void ConnectActionsAndFunctions() override;
    void ConfigurationChanged() override;
    bool FilterEvents(mitk::InteractionEvent *, mitk::DataNode *) override;

  private:
    void SelectSingleSpectrumByPoint(mitk::StateMachineAction *, mitk::InteractionEvent *);
    bool IsOver(const mitk::InteractionEvent *);
    mitk::DataNode * FindSingleSpectrumDataNode(mitk::DataNode *);
    
    mitk::DataStorage * m_DataStorage;
  };
} // namespace m2
