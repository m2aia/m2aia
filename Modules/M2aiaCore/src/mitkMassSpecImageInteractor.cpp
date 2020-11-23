/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <mitkMassSpecImageInteractor.h>
#include <mitkMouseMoveEvent.h>

#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>
#include <m2ImzMLMassSpecImage.h>
#include <m2MSImageBase.h>
#include <mitkPointOperation.h>
#include <mitkRenderingManager.h>

mitk::MSDataObjectInteractor::MSDataObjectInteractor() {}
mitk::MSDataObjectInteractor::~MSDataObjectInteractor() {}

void mitk::MSDataObjectInteractor::ConnectActionsAndFunctions()
{
  CONNECT_FUNCTION("addpoint", AddPoint);
}

void mitk::MSDataObjectInteractor::ConfigurationChanged() {}

void mitk::MSDataObjectInteractor::AddPoint(StateMachineAction *, InteractionEvent *interactionEvent)
{
  const BaseRenderer::Pointer sender = interactionEvent->GetSender();
  // auto renWindows = sender->GetRenderWindow()->GetAllRegisteredRenderWindows();
  auto *positionEvent = static_cast<InteractionPositionEvent *>(interactionEvent);
  Point3D pos = positionEvent->GetPositionInWorld();
  MITK_INFO << pos;

  auto msiData = dynamic_cast<m2::MSImageBase *>(this->GetDataNode()->GetData());
  if (msiData)
  {
    mitk::ImagePixelReadAccessor<m2::IndexImagePixelType, 3> acc(msiData->GetIndexImage());
//    auto msiIndex = acc.GetPixelByWorldCoordinates(pos);

    std::vector<double> ints, mzs;
    mitkThrow() << "singe spectrum grab is not implemented";
    //msiData->GrabSpectrum(msiIndex, mzs, ints);
  }
}
