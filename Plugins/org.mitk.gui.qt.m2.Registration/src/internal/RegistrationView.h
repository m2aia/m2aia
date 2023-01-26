/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef Registration_h
#define Registration_h


#include "ui_RegistrationViewControls.h"
#include <map>
#include <QmitkAbstractView.h>
#include <QDialog>
#include <QFutureWatcher>
#include <berryISelectionListener.h>


#include <mitkPointSet.h>
#include "RegistrationData.h"
#include <ui_ParameterFileEditorDialog.h>

class QmitkSingleNodeSelectionWidget;
class RegistrationDataWidget;

/**
  \brief Registration

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class RegistrationView : public QmitkAbstractView
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

  QString GetElastixPathFromPreferences() const;

  char m_ModalityId = 'A';
  QWidget *m_Parent;
  
  // struct RegistrationEntity{
  //   QmitkSingleNodeSelectionWidget * m_ImageSelection;
  //   QmitkSingleNodeSelectionWidget * m_ImageMaskSelection;
  //   QmitkSingleNodeSelectionWidget * m_PointSetSelection;
  //   std::vector<QmitkSingleNodeSelectionWidget *> m_Attachments;
  //   mitk::DataNode::Pointer m_ResultNode;
  //   std::vector<mitk::DataNode::Pointer> m_ResultAttachments;
  //   QSpinBox * m_Index;
  //   std::vector<std::string> m_Transformations;
  // };

  QFutureWatcher<void> m_RegistrationJob;
  Ui::RegistrationViewControls m_Controls;
  RegistrationDataWidget* m_FixedEntity;

  Ui::elxParameterFileEditor m_ParameterFileEditorControls;
  QDialog * m_ParameterFileEditor;
  std::map<int,std::vector<std::string>> m_ParameterFiles, m_DefaultParameterFiles;

  void StartRegistration();
  void Registration(RegistrationDataWidget * fixed, RegistrationDataWidget * moving);
  void AddNewModalityTab();

  public slots:
  void PostProcessReconstruction();

};

#endif // Registration_h
