/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef m2ImzMLExportView_h
#define m2ImzMLExportView_h

#include "ui_m2ImzMLExportViewControls.h"
#include <QmitkAbstractView.h>
#include <berryISelectionListener.h>

/**
  \brief m2ImzMLExportView

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class m2ImzMLExportView : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:
  static const std::string VIEW_ID;

protected:
  
  virtual void CreateQtPartControl(QWidget *parent) override;

  virtual void SetFocus() override;

  virtual void NodeAdded(const mitk::DataNode *) override;

  Ui::m2ImzMLExportViewControls m_Controls;

protected slots:
  void UpdateExportSettingsAllNodes();
  void UpdateExportSettings(const mitk::DataNode *node);
};

#endif // m2ImzMLExportView_h
