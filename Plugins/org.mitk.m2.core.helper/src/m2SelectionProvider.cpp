/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "m2SelectionProvider.h"

namespace m2
{
	SelectionProvider::SelectionProvider() : _selection(nullptr) {}

  void SelectionProvider::AddSelectionChangedListener(berry::ISelectionChangedListener *listener)
  {
    _selectionEvents.AddListener(listener);
  }

  void SelectionProvider::RemoveSelectionChangedListener(berry::ISelectionChangedListener *listener)
  {
    _selectionEvents.RemoveListener(listener);
  }

  berry::ISelection::ConstPointer SelectionProvider::GetSelection() const
  {
    return berry::ISelection::ConstPointer(_selection);
  }

  void SelectionProvider::SetSelection(const berry::ISelection::ConstPointer &selection)
  {
    _selection = berry::ISelection::ConstPointer(selection.GetPointer());
	berry::SelectionChangedEvent::Pointer event(
		new berry::SelectionChangedEvent(berry::ISelectionProvider::Pointer(this), selection));
	_selectionEvents.selectionChanged(event);
  }

  /*void SelectionProvider::SetNodeSelection(SelectionProvider::NodeSelection::ConstPointer selection)
  {
    if (_selection != selection)
    {
      _selection = selection;
      berry::SelectionChangedEvent::Pointer event(
        new berry::SelectionChangedEvent(berry::ISelectionProvider::Pointer(this), selection));
      _selectionEvents.selectionChanged(event);
    }
  }

  void SelectionProvider::SetIonImageRefSelection(
	  SelectionProvider::IonImageReferenceSelection::ConstPointer selection)
  {
    if (_selection != selection)
    {
      _selection = selection;
      berry::SelectionChangedEvent::Pointer event(
        new berry::SelectionChangedEvent(berry::ISelectionProvider::Pointer(this), selection));
      _selectionEvents.selectionChanged(event);
    }
  }

  void SelectionProvider::SetOverViewSpectrumSelection(
	  SelectionProvider::IonImageReferenceSelection::ConstPointer selection)
  {
	  if (_selection != selection)
	  {
		  _selection = selection;
		  berry::SelectionChangedEvent::Pointer event(
			  new berry::SelectionChangedEvent(berry::ISelectionProvider::Pointer(this), selection));
		  _selectionEvents.selectionChanged(event);
	  }
  }*/

} // namespace m2
