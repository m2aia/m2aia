/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/


#ifndef QMITKWelcomePERSPECTIVE_H_
#define QMITKWelcomePERSPECTIVE_H_

#include <berryIPerspectiveFactory.h>

class QmitkWelcomePerspective : public QObject, public berry::IPerspectiveFactory
{
  Q_OBJECT
  Q_INTERFACES(berry::IPerspectiveFactory)

public:

  QmitkWelcomePerspective() {}
  ~QmitkWelcomePerspective() {}

  void CreateInitialLayout(berry::IPageLayout::Pointer /*layout*/) override;
};

#endif /* QMITKWelcomePERSPECTIVE_H_ */
