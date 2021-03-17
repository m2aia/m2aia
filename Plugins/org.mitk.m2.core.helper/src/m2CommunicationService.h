/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef _MITK_M2_ABSTRACT_VIEW_H
#define _MITK_M2_ABSTRACT_VIEW_H

#include <org_mitk_m2_core_helper_Export.h>
#include <qobject.h>
#include <mitkDataNode.h>
#include <m2SpectrumImageBase.h>

namespace m2
{
  class MITK_M2_CORE_HELPER_EXPORT CommunicationService : public QObject
  {
    Q_OBJECT
  public:
    CommunicationService(QObject *parent = nullptr) : QObject(parent) {}
    static CommunicationService *Instance()
    {
      static CommunicationService *_service = nullptr;
      if (!_service)
        _service = new CommunicationService();
      return _service;
    }
	using NodesVectorType = itk::VectorContainer<unsigned, mitk::DataNode::Pointer>;
  public:
  signals:
	void SpectrumImageNodeAdded(const mitk::DataNode *);
    
	void RangeChanged(qreal, qreal);
    void GenerateImageData(qreal, qreal);
	
	void OverviewSpectrumChanged(const mitk::DataNode *, m2::OverviewSpectrumType);
	
	void RequestProcessingNodes(const QString &);
	void SendProcessingNodes(const QString &, NodesVectorType::Pointer);

  private:
  };

} // namespace m2

#endif /* BERRYQTSELECTIONPROVIDER_H_ */
