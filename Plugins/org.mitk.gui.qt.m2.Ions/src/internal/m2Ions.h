/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#ifndef m2Ions_h
#define m2Ions_h

#include <berryISelectionListener.h>

#include <mitkImage.h>

#include <QMenu>
#include <QmitkAbstractView.h>
#include <qaction.h>
#include <qwidgetaction.h>

#include "ui_m2Ions.h"
#include <QColorDialog>
#include <QLabel>

#include <QmitkDataStorageComboBox.h>
#include <ctkRangeWidget.h>
#include <m2CommunicationService.h>
#include <m2MSImageBase.h>
#include <m2MultiSliceFilter.h>

/**
  \brief m2Ions

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class m2Ions : public QmitkAbstractView
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
  /*virtual void OnSelectionChanged(berry::IWorkbenchPart::Pointer source,
                                  const QList<mitk::DataNode::Pointer> &nodes) override;
*/
  void CalculateVisualization(m2::CommunicationService::NodesVectorType::Pointer nodes);
  void UpdateImageList(m2::CommunicationService::NodesVectorType::Pointer nodes);

  Ui::m2IonsControls m_Controls;

  QMenu *m_Menu;
  QWidgetAction *m_Include;
  QAction *m_PromoteToAllNodes;

private:
  using ColorType = float;
  using RBGAColorType = std::array<ColorType, 4>;

  void InitializeFilter(m2::MultiSliceFilter::Pointer);
  void PerformPCA(std::set<m2::IonImageReference *, m2::IonImageReference::Comp>);
  void PerformTsne(std::set<m2::IonImageReference *, m2::IonImageReference::Comp>);
  int SetM2FilterImages(m2::MassSpecVisualizationFilter::Pointer, std::set<m2::IonImageReference *, m2::IonImageReference::Comp>);

  m2::MSImageBase::Pointer MSImageBase;
  std::vector<std::array<ColorType, 4>> m_SelectedColors;


  // std::list<int> m_ValidVectorIndices;
  // std::vector<double> m_SelectedMzValues;
  // std::vector<mitk::Image::Pointer> m_SelectedIonImages;
  // mitk::DataNode::Pointer m_SelectedNode;

  std::map<const mitk::DataNode *, std::map<m2::IonImageReference::Pointer, mitk::Image::Pointer>> m_ContainerMap;
  unsigned int m_NumberOfComponents = 3;
  unsigned int m_Perplexity = 5;
  unsigned int m_Iterations = 250;

protected slots:
  void OnProcessingNodesReceived(const QString &, m2::CommunicationService::NodesVectorType::Pointer nodes);
};

#endif // m2Ions_h
