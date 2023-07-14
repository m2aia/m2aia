/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include <m2SpectrumImageBase.h>
#include <m2SpectrumImageDataInteractor.h>
#include <mitkImageCast.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkMouseDoubleClickEvent.h>
#include <mitkNodePredicateDataType.h>
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

bool m2::SpectrumImageDataInteractor::IsOver(const mitk::InteractionEvent *)
{
  return false;
}

void m2::SpectrumImageDataInteractor::SetDataStorage(mitk::DataStorage *ds)
{
  m_DataStorage = ds;
}

mitk::DataStorage *m2::SpectrumImageDataInteractor::GetDataStorage() const
{
  return m_DataStorage;
}

bool m2::SpectrumImageDataInteractor::FilterEvents(mitk::InteractionEvent *interactionEvent, mitk::DataNode *)
{
  if (dynamic_cast<mitk::MouseDoubleClickEvent *>(interactionEvent))
  {
    // if (me->GetModifiers() == mitk::InteractionEvent::ShiftKey &&
    //     me->GetButtonStates() == mitk::InteractionEvent::LeftMouseButton)
    {
      const mitk::BaseRenderer::Pointer sender = interactionEvent->GetSender();
      auto *positionEvent = static_cast<mitk::InteractionPositionEvent *>(interactionEvent);
      mitk::Point3D pos = positionEvent->GetPositionInWorld();

      auto imageNodes = this->GetDataStorage()->GetSubset(mitk::TNodePredicateDataType<m2::SpectrumImageBase>::New());
      for (auto imageNode : *imageNodes)
      {
        auto image = dynamic_cast<m2::SpectrumImageBase *>(imageNode->GetData());
        if (image->GetGeometry()->IsInside(pos))
        {
          itk::Index<3> index;
          image->GetGeometry()->WorldToIndex(pos, index);
          auto indexImage = image->GetIndexImage();
          mitk::ImagePixelReadAccessor<m2::IndexImagePixelType, 3> acc(indexImage);
          auto id = acc.GetPixelByIndex(index);

          auto singleSpectrumNode = FindSingleSpectrumDataNode(imageNode);
          if (!singleSpectrumNode)
            continue;

          auto singleSpectrum = dynamic_cast<m2::IntervalVector *>(singleSpectrumNode->GetData());
          std::vector<double> xs, ys;
          image->GetSpectrum(id, xs, ys);
          singleSpectrum->GetIntervals().clear();
          auto inserter = std::back_inserter(singleSpectrum->GetIntervals());
          std::transform(std::begin(xs),
                         std::end(xs),
                         std::begin(ys),
                         inserter,
                         [](const auto &x, const auto &y) { return m2::Interval(x, y); });

          singleSpectrumNode->Modified();
          singleSpectrum->Modified();
        }
      }
      
      return false;
    }
  }

  return false;
}

mitk::DataNode *m2::SpectrumImageDataInteractor::FindSingleSpectrumDataNode(mitk::DataNode * node)
{
  auto derivations = this->GetDataStorage()->GetDerivations(node);
  for (auto node : *derivations)
  {
    if (node->GetName().find("SingleSpectrum") != std::string::npos)
    {
      return node;
    }
  }
  return nullptr;
}

void m2::SpectrumImageDataInteractor::SelectSingleSpectrumByPoint(mitk::StateMachineAction *, mitk::InteractionEvent *)
{
}
