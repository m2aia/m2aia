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
  Q_OBJECT
public:
  static const std::string VIEW_ID;
protected:
  virtual void CreateQtPartControl(QWidget *parent) override;
  virtual void SetFocus() override;
  void Execute();
  Ui::ExternalDockerModulesControls m_Controls;
};

#endif // ExternalDockerModules_h
