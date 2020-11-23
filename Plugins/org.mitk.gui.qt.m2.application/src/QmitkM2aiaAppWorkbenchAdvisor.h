/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/


#ifndef QMITKM2aiaAPPWORKBENCHADVISOR_H_
#define QMITKM2aiaAPPWORKBENCHADVISOR_H_

#include <berryQtWorkbenchAdvisor.h>

class QmitkM2aiaAppWorkbenchAdvisor : public berry::QtWorkbenchAdvisor
{

public:

  static const QString WELCOME_PERSPECTIVE_ID; // = "org.mitk.qt.M2aiaapp.defaultperspective"

  void Initialize(berry::IWorkbenchConfigurer::Pointer configurer) override;

  berry::WorkbenchWindowAdvisor* CreateWorkbenchWindowAdvisor(
        berry::IWorkbenchWindowConfigurer::Pointer configurer) override;

  QString GetInitialWindowPerspectiveId() override;

};

#endif /* QMITKM2aiaAPPWORKBENCHADVISOR_H_ */
