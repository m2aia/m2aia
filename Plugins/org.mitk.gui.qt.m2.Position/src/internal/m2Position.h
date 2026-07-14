/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/


#ifndef m2Position_h
#define m2Position_h

#include <berryISelectionListener.h>

#include <QmitkAbstractView.h>

#include "ui_m2PositionControls.h"

/**
  \brief m2Position

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class m2Position : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:
  static const std::string VIEW_ID;

protected:
  virtual void CreateQtPartControl(QWidget *parent) override;

  virtual void SetFocus() override;

  /// \brief called by QmitkFunctionality when DataManager's selection has changed
  virtual void OnSelectionChanged(berry::IWorkbenchPart::Pointer source,
                                  const QList<mitk::DataNode::Pointer> &nodes) override;

  /// \brief Called when the user clicks the GUI button
  void Rotate(float angleDeg);
  void RotateData(mitk::BaseData *data, float angleDeg, const mitk::Point3D &center);

  void Mirror(int w);
  void MirrorData(mitk::BaseData *data, int w);

  void Move(std::array<float,2> vec);
  void MoveData(mitk::BaseData *data, std::array<float,2> vec);

  void ApplySpacing();
  void ApplySpacingToData(mitk::BaseData *data, const mitk::Vector3D &spacing);

  void ApplyOrigin();
  void ApplyOriginToData(mitk::BaseData *data, const mitk::Point3D &origin);

  void RefreshGeometryInfo();

  Ui::m2PositionControls m_Controls;
};

#endif // m2Position_h
