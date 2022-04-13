/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#pragma once

#include <berryQtWorkbenchAdvisor.h>
#include <QmitkExtWorkbenchWindowAdvisor.h>

class QmitkM2aiaAppWorkbenchAdvisor : public berry::QtWorkbenchAdvisor
{
public:
  static const QString WELCOME_PERSPECTIVE_ID; // = "org.mitk.qt.M2aiaapp.defaultperspective"

  void Initialize(berry::IWorkbenchConfigurer::Pointer configurer) override;

  berry::WorkbenchWindowAdvisor *CreateWorkbenchWindowAdvisor(
    berry::IWorkbenchWindowConfigurer::Pointer configurer) override;

  QString GetInitialWindowPerspectiveId() override;
};

class QmitkM2aiaAppWorkbenchWindowAdvisor : public QmitkExtWorkbenchWindowAdvisor
{
public:
  QmitkM2aiaAppWorkbenchWindowAdvisor(berry::WorkbenchAdvisor *wbAdvisor,
                                      berry::IWorkbenchWindowConfigurer::Pointer configurer);
  void PostWindowOpen() override;
  void Initialize(berry::IWorkbenchConfigurer::Pointer configurer);
  
  QString GetInitialWindowPerspectiveId();

  void loadDataFromDisk(const QStringList &arguments, bool globalReinit);

};
