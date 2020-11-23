/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/


#ifndef ImageNavigator3D_h
#define ImageNavigator3D_h

#include <berryISelectionListener.h>

#include <QmitkAbstractView.h>

#include "ui_ImageNavigator3DControls.h"
#include <array>

#include <mitkIRenderWindowPart.h>

/**
  \brief ImageNavigator3D

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class ImageNavigator3D : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:
  static const std::string VIEW_ID;

protected:
  virtual void CreateQtPartControl(QWidget *parent) override;

  virtual void SetFocus() override;

  Ui::ImageNavigator3DControls m_Controls;


  std::array<int, 3> m_Orientations;
  void ChangeOrientation();

  mitk::IRenderWindowPart * m_IRenderWindowPart;
  void RenderWindowPartActivated(mitk::IRenderWindowPart* renderWindowPart);

  void OnRefetch();


};

#endif // ImageNavigator3D_h
