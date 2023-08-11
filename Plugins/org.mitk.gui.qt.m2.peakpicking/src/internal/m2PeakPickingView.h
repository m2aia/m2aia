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

#include "ui_m2PeakPickingViewControls.h"
#include <QMetaObject>
#include <QmitkAbstractView.h>
#include <QmitkSingleNodeSelectionWidget.h>
#include <berryISelectionListener.h>

#include <m2SpectrumImage.h>
#include <m2IntervalVector.h>

#include <m2UIUtils.h>


namespace itk
{
  template <typename TPixelType, unsigned int Dimension>
  class VectorImageToImageAdaptor;

}

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
  Ui::m2PeakPickingViewControls m_Controls;

  m2::UIUtils::NodesVectorType::Pointer m_ReceivedNodes = nullptr;
  // using PeakVectorType = m2::SpectrumImage::IntervalVectorType;
  // PeakVectorType m_PeakList;
  QMetaObject::Connection m_Connection;
  mitk::DataNode::Pointer CreatePeakList(const mitk::DataNode *parent, std::string name);
  std::vector<m2::Interval> PeakPicking(const std::vector<double> &xs, const std::vector<double> &ys);

  void SetGroupProcessProfileSpectraEnabled(bool v);
  void SetGroupProcessCentroidSpectraEnabled(bool v);
  mitk::DataNode *GetParent(mitk::DataNode *) const;

 
  /**
   * Find the parent node of a given node
  */
  mitk::DataNode::Pointer GetParent(const mitk::DataNode *node);

  /**
   * check if nodes contains a node partially match the given name.
  */
  mitk::DataNode::Pointer GetNode(std::string substring);

  /**
   * check if derivations contains a node partially match the given name.
  */
  mitk::DataNode::Pointer GetDerivations(const mitk::DataNode * parentNode, std::string substring);


  protected slots:
    
    void OnCurrentTabChanged(unsigned int);

    void OnStartPeakPickingOverview();
    void OnStartPeakPickingImage();
    void OnStartCombineLists();
    void OnStartExportImages();
    void OnStartPeakBinning();
  };

#endif // m2PeakPickingView_h
