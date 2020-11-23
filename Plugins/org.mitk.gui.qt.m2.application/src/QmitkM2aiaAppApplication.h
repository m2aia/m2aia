/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef QMITKM2aiaAppAPPLICATION_H_
#define QMITKM2aiaAppAPPLICATION_H_

#include <berryIApplication.h>

class QmitkM2aiaAppApplication : public QObject, public berry::IApplication
{
  Q_OBJECT
  Q_INTERFACES(berry::IApplication)

public:

  QmitkM2aiaAppApplication() {}
  ~QmitkM2aiaAppApplication() {}

  QVariant Start(berry::IApplicationContext*) override;
  void Stop() override;
};

#endif /*QMITKM2aiaAppAPPLICATION_H_*/
