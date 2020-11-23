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
namespace m2
{
  class MITK_M2_CORE_HELPER_EXPORT ComminicationService : public QObject
  {

	  Q_OBJECT

  public:
    static ComminicationService *Instance()
    {
      static ComminicationService *_service = nullptr;
      if (!_service)
        _service = new ComminicationService();
      return _service;
    }

  public:
  
  signals:
		void MassRangeSelected(qreal, qreal);
		void GrabIonImage(qreal, qreal);
		

  private:
  };

} // namespace m2

#endif /* BERRYQTSELECTIONPROVIDER_H_ */
