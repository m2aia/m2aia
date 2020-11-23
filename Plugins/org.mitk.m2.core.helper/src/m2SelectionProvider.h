/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef _MITK_M2_SELECTION_PROVIDER_H
#define _MITK_M2_SELECTION_PROVIDER_H

#include <org_mitk_m2_core_helper_Export.h>

#include <berryISelectionProvider.h>

#include <QObject>

#include <m2SelectionT.hxx>
#include <mitkDataNode.h>


namespace m2
{

class MITK_M2_CORE_HELPER_EXPORT SelectionProvider: public QObject,
  public berry::ISelectionProvider
{
	Q_OBJECT	

public:
	template<class T>
	struct DataWrapper{
		const T node;
		int status = 0;

		inline bool operator==(const DataWrapper & obj) const
		{
			return this->node == obj.node && this->status == obj.status;
		}
	};

  berryObjectMacro(SelectionProvider);
  using NodeWrapper = DataWrapper<const mitk::DataNode *>;
  using NodeSelection = m2::SelectionT<NodeWrapper>;
  
  using RangeWrapper = DataWrapper<const std::pair<double,double>>;
  using RangeSelection = m2::SelectionT<RangeWrapper>;
  
  using OverViewSpectrumWrapper = DataWrapper<const unsigned int>;
  using OverViewSpectrumSelection = m2::SelectionT<OverViewSpectrumWrapper>;

  SelectionProvider();

  void AddSelectionChangedListener(berry::ISelectionChangedListener* listener) override;

  void RemoveSelectionChangedListener( berry::ISelectionChangedListener* listener) override;

  berry::ISelection::ConstPointer GetSelection() const override;
  void SetSelection(const berry::ISelection::ConstPointer& selection) override;
  /*void SetNodeSelection(NodeSelection::ConstPointer selection);
  void SetOverViewSpectrumSelection(OverViewSpectrumSelection::ConstPointer selection);
  void SetIonImageRefSelection(IonImageReferenceSelection::ConstPointer selection);
*/
protected:
  berry::ISelectionChangedListener::Events _selectionEvents;
  berry::ISelection::ConstPointer _selection;
};

}

#endif /* BERRYQTSELECTIONPROVIDER_H_ */
