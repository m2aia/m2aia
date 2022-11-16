/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Lorenz Schwab
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#ifndef ExternalDockerModules_h
#define ExternalDockerModules_h

#include <berryISelectionListener.h>

#include <QmitkAbstractView.h>

#include "ui_ExternalDockerModulesControls.h"

/**
  \brief ExternalDockerModules

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class ExternalDockerModules : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:
  static const std::string VIEW_ID;

protected:
  virtual void CreateQtPartControl(QWidget *parent) override;

  virtual void SetFocus() override;

  void ExecuteModule();

  Ui::ExternalDockerModulesControls m_Controls;
private:
  // path to M2aia-Docker repository
  const std::string m_pathDocker = "/home/maia/Documents/maia/Docker/Images"; // trailing / is important
  // will get correct path / name appended at runtime
  std::string m_containerName = "m2aia-container";
};

#endif // ExternalDockerModules_h
