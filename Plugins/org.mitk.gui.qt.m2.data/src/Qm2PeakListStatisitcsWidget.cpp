/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "Qm2PeakListStatisitcsWidget.h"



Qm2PeakListStatisitcsWidget::Qm2PeakListStatisitcsWidget(QWidget *parent) : QWidget(parent)
{
  m_Controls.setupUi(this);

  // m_Controls.imageSelector->SetDataStorage(GetDataStorage());
  // m_Controls.imageSelector->SetAutoSelectNewNodes(false);
  // m_Controls.imageSelector->SetNodePredicate(

  //   mitk::NodePredicateAnd::New(mitk::NodePredicateDataType::New("LabelSetImage"),
  //                               mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  // m_Controls.imageSelector->SetSelectionIsOptional(true);
  // m_Controls.imageSelector->SetEmptyInfo(QString("LabelSetImage selection"));
  // m_Controls.imageSelector->SetPopUpTitel(QString("LabelSetImage"));
}


