/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/


#ifndef m2PeakPickingView_h
#define m2PeakPickingView_h

#include <berryISelectionListener.h>

#include <QmitkAbstractView.h>
#include <QmitkSingleNodeSelectionWidget.h>

#include "ui_m2PeakPickingViewControls.h"
#include <m2CommunicationService.h>

/**
  \brief m2PeakPickingView

  \warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

  \sa QmitkAbstractView
  \ingroup ${plugin_target}_internal
*/
class m2PeakPickingView : public QmitkAbstractView
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
  void StartPeakPicking();

  Ui::m2PeakPickingViewControls m_Controls;
  QmitkSingleNodeSelectionWidget * m_MassSpecDataNodeSelectionWidget;
  m2::CommunicationService::NodesVectorType::Pointer m_ReceivedNodes = nullptr;
protected slots:
	void OnProcessingNodesReceived(const QString &, m2::CommunicationService::NodesVectorType::Pointer);
};

#endif // m2PeakPickingView_h
