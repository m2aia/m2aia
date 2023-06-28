/*===================================================================

Mass Spectrometry Imaging applications for interactive
analysis in MITK (M2aia)

Copyright (c) Jonas Cordes, Hochschule Mannheim.
Division of Medical Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/

#include "m2DataStatistics.h"

#include <mitkLabelSetImage.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateProperty.h>

#include <m2SpectrumImageBase.h>
#include <m2IntervalVector.h>

void m2DataStatistics::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  m_Parent = parent;
  
  
  // spectrum image selector
  m_Controls.spectrumImageSelector->SetDataStorage(GetDataStorage());
  m_Controls.spectrumImageSelector->SetAutoSelectNewNodes(false);
  m_Controls.spectrumImageSelector->SetSelectionIsOptional(true);
  m_Controls.spectrumImageSelector->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::SpectrumImageBase>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.spectrumImageSelector->SetEmptyInfo(QString("Image selection"));
  m_Controls.spectrumImageSelector->SetPopUpTitel(QString("Image"));
  
  
  // label image selector
  m_Controls.labelImageSelector->SetDataStorage(GetDataStorage());
  m_Controls.labelImageSelector->SetAutoSelectNewNodes(false);
  m_Controls.labelImageSelector->SetSelectionIsOptional(true);
  m_Controls.labelImageSelector->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::LabelSetImage>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.labelImageSelector->SetEmptyInfo(QString("LabelSetImage selection"));
  m_Controls.labelImageSelector->SetPopUpTitel(QString("LabelSetImage"));
  
  // peak list selector
  m_Controls.peakListSelector->SetDataStorage(GetDataStorage());
  m_Controls.peakListSelector->SetAutoSelectNewNodes(false);
  m_Controls.peakListSelector->SetSelectionIsOptional(true);
  m_Controls.peakListSelector->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::IntervalVector>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.peakListSelector->SetEmptyInfo(QString("IntervalVector selection"));
  m_Controls.peakListSelector->SetPopUpTitel(QString("IntervalVector"));

}

void m2DataStatistics::UpdateTable(){
  m_Controls.table->clear();
  
  auto imageNode = m_Controls.spectrumImageSelector->GetSelectedNode();
  auto labelNode = m_Controls.labelImageSelector->GetSelectedNode();
  auto peakNode = m_Controls.peakListSelector->GetSelectedNode();

  if(!imageNode || !labelNode || !peakNode){
    return;
  }

  auto image = dynamic_cast<m2::SpectrumImageBase*>(imageNode->GetData());
  auto labels = dynamic_cast<mitk::LabelSetImage*>(labelNode->GetData());
  auto peaks = dynamic_cast<m2::IntervalVector*>(peakNode->GetData());

  if(!image || !labels || !peaks){
    return;
  }

  if(!mitk::Equal(*image->GetGeometry(), *labels->GetGeometry())){
    return;
  }



}