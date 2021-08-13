/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef Registration2D_h
#define Registration2D_h

#include "ui_MovingModalityWidgetControls.h"
#include "ui_Registration2DControls.h"
#include <QmitkAbstractView.h>
#include <QDialog>
#include <berryISelectionListener.h>
#include <map>
#include <mitkPointSet.h>
#include <ui_ParameterFileEditorDialog.h>

class QmitkSingleNodeSelectionWidget;

/**
  \brief Registration2D

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class Registration2D : public QmitkAbstractView
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
  

  Ui::Registration2DControls m_Controls;
  std::map<char, std::vector<QmitkSingleNodeSelectionWidget *>> m_MovingModalities;

  QmitkSingleNodeSelectionWidget *m_FixedImageSingleNodeSelection;
  QmitkSingleNodeSelectionWidget *m_FixedPointSetSingleNodeSelection;
  Ui::elxParameterFileEditor m_ParameterFileEditorControls;
  QDialog * m_ParameterFileEditor;
  std::map<int,std::vector<std::string>> m_ParameterFiles, m_DefaultParameterFiles;

  void StartRegistration();
  void AddNewModalityTab();
};

#endif // Registration2D_h
