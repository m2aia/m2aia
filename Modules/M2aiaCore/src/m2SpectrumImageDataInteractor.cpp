/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2SpectrumImageDataInteractor.h>
#include <mitkMouseMoveEvent.h>

#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>
#include <m2SpectrumImageBase.h>
#include <mitkPointOperation.h>
#include <mitkRenderingManager.h>

m2::SpectrumImageDataInteractor::SpectrumImageDataInteractor() {}
m2::SpectrumImageDataInteractor::~SpectrumImageDataInteractor() {}

void m2::SpectrumImageDataInteractor::ConnectActionsAndFunctions()
{
  CONNECT_CONDITION("isoverpoint", IsOver);
  CONNECT_FUNCTION("addpoint", SelectSingleSpectrumByPoint);
}

void m2::SpectrumImageDataInteractor::ConfigurationChanged() {}

bool m2::SpectrumImageDataInteractor::IsOver(const mitk::InteractionEvent *){
  return false;
}

void m2::SpectrumImageDataInteractor::SelectSingleSpectrumByPoint(mitk::StateMachineAction *, mitk::InteractionEvent *interactionEvent)
{
  const mitk::BaseRenderer::Pointer sender = interactionEvent->GetSender();
  // auto renWindows = sender->GetRenderWindow()->GetAllRegisteredRenderWindows();
  auto *positionEvent = static_cast<mitk::InteractionPositionEvent *>(interactionEvent);
  mitk::Point3D pos = positionEvent->GetPositionInWorld();
  
  if (auto msiData = dynamic_cast<m2::SpectrumImageBase *>(this->GetDataNode()->GetData()))
  {
    auto indexImage = msiData->GetIndexImage();
    mitk::ImagePixelReadAccessor<m2::IndexImagePixelType, 3> acc(indexImage);
    itk::Index<3> index;
    indexImage->GetGeometry()->WorldToIndex(pos, index);
    auto id = acc.GetPixelByIndex(index);
    // modifies the node properties, so just listen to itk::ModifiedEvent, which is triggered 
    // on property changes, this will directly invoke an observer callback.
    // known observers in: m2Spectrum.cpp
    if(auto baseProp = this->GetDataNode()->GetProperty("m2.singlespectrum.id")){
      auto prop = dynamic_cast<mitk::StringProperty *>(baseProp);
      prop->SetValue(std::to_string(id).c_str());
    }
  }
}
