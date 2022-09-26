/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#ifndef QMITK_ConvertToMultiLabelSegmentation_H
#define QMITK_ConvertToMultiLabelSegmentation_H

#include "mitkIContextMenuAction.h"

#include "org_mitk_gui_qt_m2_PeakPicking_Export.h"

#include "vector"
#include "mitkDataNode.h"
#include "mitkImage.h"

class M2PEAKPICKING_EXPORT QmitkExportComponentAction : public QObject, public mitk::IContextMenuAction
{
  Q_OBJECT
  Q_INTERFACES(mitk::IContextMenuAction)

public:

  QmitkExportComponentAction();
  virtual ~QmitkExportComponentAction() override;

  //interface methods
  void Run( const QList<mitk::DataNode::Pointer>& selectedNodes ) override;
  void SetDataStorage(mitk::DataStorage* dataStorage) override;
  void SetFunctionality(berry::QtViewPart* functionality) override;
  void SetSmoothed(bool smoothed) override;
  void SetDecimated(bool decimated) override;

private:

  mitk::Image::Pointer ExportComponentImage(const mitk::Image * img, unsigned int i);
  typedef QList<mitk::DataNode::Pointer> NodeList;

  mitk::DataStorage::Pointer m_DataStorage;
};

#endif // QMITK_ConvertToMultiLabelSegmentation_H
