/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/


#ifndef m2CombineImagesView_h
#define m2CombineImagesView_h

#include <berryISelectionListener.h>

#include <QmitkAbstractView.h>
#include <QmitkSingleNodeSelectionWidget.h>
#include <QmitkMultiNodeSelectionWidget.h>

#include "ui_m2CombineImagesViewControls.h"

/**
  \brief m2CombineImagesView

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class m2CombineImagesView : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:
  static const std::string VIEW_ID;

protected:
  virtual void CreateQtPartControl(QWidget *parent) override;

  virtual void SetFocus() override;

  /// \brief Called when the user clicks the GUI button
  void CombineImages();

  Ui::m2CombineImagesViewControls m_Controls;
  QmitkMultiNodeSelectionWidget * m_SelectA;
  QmitkSingleNodeSelectionWidget * m_SelectB;
};

#endif // m2CombineImagesView_h
