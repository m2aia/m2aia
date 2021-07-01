/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef OpticalImageRegistration_h
#define OpticalImageRegistration_h

#include "ui_OpticalImageRegistrationControls.h"
#include "ui_MovingModalityWidgetControls.h"
#include <QmitkAbstractView.h>
#include <berryISelectionListener.h>
#include <mitkPointSet.h>
#include <map>

class QmitkSingleNodeSelectionWidget;

/**
  \brief OpticalImageRegistration

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class OpticalImageRegistration : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:
  static const std::string VIEW_ID;

protected:

  virtual void CreateQtPartControl(QWidget *parent) override;
  virtual void OnSelectionChanged(berry::IWorkbenchPart::Pointer part,
                                  const QList<mitk::DataNode::Pointer> &nodes) override;
  virtual void SetFocus() override;

  QString GetElastixPath() const;
  
  
  char m_ModalityId = 'A';

  Ui::OpticalImageRegistrationControls m_Controls;
  std::map<char, Ui::MovingModalityWidgetControls> m_MovingModalitiesControls;

  QmitkSingleNodeSelectionWidget *m_FixedImageSingleNodeSelection;
  QmitkSingleNodeSelectionWidget *m_MovingImageSingleNodeSelection;
  QmitkSingleNodeSelectionWidget *m_FixedPointSetSingleNodeSelection;
  QmitkSingleNodeSelectionWidget *m_MovingPointSetSingleNodeSelection;


  void AlignImages();  
  void StartRegistration();  
  void AddNewModalityTab();
};

#endif // OpticalImageRegistration_h
