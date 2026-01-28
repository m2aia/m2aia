/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkDataNodeCreateLabelSetRegionSpectraAction.h"

#include <mitkImageAccessByItk.h>
#include <mitkImageCast.h>
#include <mitkLookupTable.h>
#include <mitkLookupTableProperty.h>
#include <mitkLabelSetImage.h>
#include <mitkNodePredicateDataType.h>
#include <m2SpectrumImage.h>

#include <itkRescaleIntensityImageFilter.h>
#include <itkImageIOBase.h>

#include <QString>
#include <QAction>
#include <QMenu>

QmitkDataNodeCreateLabelSetRegionSpectraAction::QmitkDataNodeCreateLabelSetRegionSpectraAction(QWidget* parent, berry::IWorkbenchPartSite::Pointer workbenchPartSite): 
  QAction(parent),
  QmitkAbstractDataNodeAction(workbenchPartSite)
{
  InitializeAction();
}

QmitkDataNodeCreateLabelSetRegionSpectraAction::QmitkDataNodeCreateLabelSetRegionSpectraAction(QWidget* parent, berry::IWorkbenchPartSite* workbenchPartSite): 
  QAction(parent),
  QmitkAbstractDataNodeAction(berry::IWorkbenchPartSite::Pointer(workbenchPartSite))
{
  InitializeAction();
}

void QmitkDataNodeCreateLabelSetRegionSpectraAction::InitializeAction()
{
  setText(tr("Regions to Spectra"));
  setToolTip(tr("Create region-wise spectra for the selected LabelSetImage."));

  setMenu(new QMenu);
  connect(menu(), &QMenu::aboutToShow, this, &QmitkDataNodeCreateLabelSetRegionSpectraAction::OnMenuAboutShow);
}

void QmitkDataNodeCreateLabelSetRegionSpectraAction::OnMenuAboutShow()
{
  menu()->clear();
  QAction *action;

  auto selectedNodes = this->GetSelectedNodes();
  for (auto referenceNode : selectedNodes){
    auto res = this->m_DataStorage.Lock()->GetDerivations(referenceNode,
      mitk::TNodePredicateDataType<mitk::MultiLabelSegmentation>::New());
    if (res->empty()) continue;

    for(auto l : res->CastToSTLConstContainer()){
      auto labelSetImage = dynamic_cast<mitk::MultiLabelSegmentation *>(l->GetData());
      // auto spectrumImage = dynamic_cast<m2::SpectrumImage *>(referenceNode->GetData());
      
      action = menu()->addAction(l->GetName().c_str());
      connect(action, &QAction::triggered, [l, labelSetImage, this](){
        MITK_INFO << l->GetName().c_str();

        
        // auto spectra = OnApplyRegionsToSpectra(labelSetImage);
        // auto dataNodeNew = mitk::DataNode::New();
        // dataNodeNew->SetData(spectra);
        // dataNodeNew->SetVisibility(true);
        // dataNodeNew->SetName(labelSetImage->GetName() + "_Spectra");
        // this->m_DataStorage.Lock()->Add(dataNodeNew);
      });
    }
    
    // if (referenceNode.IsNotNull()){
    //   if (mitk::MultiLabelSegmentation::Pointer labelSetImage = dynamic_cast<mitk::MultiLabelSegmentation *>(referenceNode->GetData())){
    //     action = menu()->addAction("Regions to Spectra");
    //     connect(action, &QAction::triggered, [labelSetImage, this](){
    //       auto spectra = OnApplyRegionsToSpectra(labelSetImage);
    //       auto dataNodeNew = mitk::DataNode::New();
    //       dataNodeNew->SetData(spectra);
    //       dataNodeNew->SetVisibility(true);
    //       dataNodeNew->SetName(referenceNode->GetName() + "_Spectra");
    //       this->m_DataStorage.Lock()->Add(dataNodeNew);
    //     });
    //   }
    // }
  }


  // for (const auto &type : types)
  // {
  //   QString name = QString::fromStdString();
  //   name.replace("_", " ");

  //   action = menu()->addAction(name);
  //   action->setCheckable(true);
  //   connect(action, &QAction::triggered, [type, this]() {
  //     auto selectedNodes = this->GetSelectedNodes();
  //     for (auto referenceNode : selectedNodes)
  //     {
  //       if (referenceNode.IsNotNull())
  //       {
  //         if (mitk::Image::Pointer image = dynamic_cast<mitk::Image *>(referenceNode->GetData()))
  //         {       
  //           auto newImage = OnApplyCastImage(image, type);
  //           auto dataNodeNew = mitk::DataNode::New();
  //           auto lut = mitk::LookupTable::New();
  //           lut->SetType(mitk::LookupTable::LookupTableType::GRAYSCALE_TRANSPARENT);
  //           dataNodeNew->SetProperty("LookupTable", mitk::LookupTableProperty::New(lut));
  //           dataNodeNew->SetData(newImage);
  //           dataNodeNew->SetVisibility(false);
  //           auto dummyName = referenceNode->GetName() + "_" + itk::ImageIOBase::GetComponentTypeAsString(type);
  //           dataNodeNew->SetName(dummyName);
  //           this->m_DataStorage.Lock()->Add(dataNodeNew);
  //         }
  //       }
  //     }
  //   });
  // }
}
