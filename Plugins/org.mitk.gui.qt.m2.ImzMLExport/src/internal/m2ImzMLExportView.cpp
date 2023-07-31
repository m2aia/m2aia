/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include "m2ImzMLExportView.h"

// Qt
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>

// mitk image
#include <m2ImzMLImageIO.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2SpectrumImageBase.h>
#include <QmitkIOUtil.h>
#include <mitkImage.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateFunction.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>

const std::string m2ImzMLExportView::VIEW_ID = "org.mitk.views.m2.imzmlexport";

void m2ImzMLExportView::SetFocus() {}

void m2ImzMLExportView::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);

  m_Controls.cmbBxOutputMode->addItem("Continuous Profile",
                                      static_cast<unsigned>(m2::SpectrumFormat::ContinuousProfile));
  m_Controls.cmbBxOutputMode->addItem("Continuous Centroid",
                                      static_cast<unsigned>(m2::SpectrumFormat::ContinuousCentroid));

  m_Controls.cmbBxOutputDatatypeInt->addItem("Float", static_cast<unsigned>(m2::NumericType::Float));
  m_Controls.cmbBxOutputDatatypeInt->addItem("Double", static_cast<unsigned>(m2::NumericType::Double));

  m_Controls.cmbBxOutputDatatypeMz->addItem("Float", static_cast<unsigned>(m2::NumericType::Float));
  m_Controls.cmbBxOutputDatatypeMz->addItem("Double", static_cast<unsigned>(m2::NumericType::Double));

  m_Controls.imageSelection->SetDataStorage(GetDataStorage());
  m_Controls.imageSelection->SetAutoSelectNewNodes(true);
  m_Controls.imageSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::SpectrumImageBase>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))));
  m_Controls.imageSelection->SetSelectionIsOptional(true);
  m_Controls.imageSelection->SetEmptyInfo(QString("Image selection"));
  m_Controls.imageSelection->SetPopUpTitel(QString("Image"));

  auto isChildOfImageNode = [this](const mitk::DataNode *node)
  {
    if (auto imageNode = m_Controls.imageSelection->GetSelectedNode())
    {
      auto childNodes = GetDataStorage()->GetDerivations(imageNode);
      using namespace std;
      return find(begin(*childNodes), end(*childNodes), node) != end(*childNodes);
    }
    return false;
  };

  m_Controls.listSelection->SetDataStorage(GetDataStorage());
  m_Controls.listSelection->SetNodePredicate(
    mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<m2::IntervalVector>::New(),
                                mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object")),
                                mitk::NodePredicateFunction::New(isChildOfImageNode)));

  m_Controls.listSelection->SetAutoSelectNewNodes(true);
  m_Controls.listSelection->SetSelectionIsOptional(true);
  m_Controls.listSelection->SetEmptyInfo(QString("PeakList selection"));
  m_Controls.listSelection->SetPopUpTitel(QString("PeakList"));

  connect(m_Controls.btnExport,
          &QPushButton::clicked,
          this,
          [this, parent]()
          {
            if (auto node = this->m_Controls.imageSelection->GetSelectedNode())
            {
              if (auto listNode = this->m_Controls.listSelection->GetSelectedNode())
              {
                auto list = dynamic_cast<m2::IntervalVector *>(listNode->GetData());
                const auto name = QFileDialog::getSaveFileName(parent);

                const auto yDataType = static_cast<m2::NumericType>(m_Controls.cmbBxOutputDatatypeInt->currentData(Qt::UserRole).toUInt());
                const auto xDataType = static_cast<m2::NumericType>(m_Controls.cmbBxOutputDatatypeMz->currentData(Qt::UserRole).toUInt());
                const auto format = static_cast<m2::SpectrumFormat>(m_Controls.cmbBxOutputMode->currentData(Qt::UserRole).toUInt());
                

                m2::ImzMLImageIO io;
                io.SetIntervalVector(list);
                io.SetDataTypeXAxis(xDataType);
                io.SetDataTypeYAxis(yDataType);
                io.SetSpectrumFormat(format);
                io.SetOutputLocation(name.toStdString());
                io.mitk::AbstractFileIOWriter::SetInput(node->GetData());
                io.Write();
              }
            }
          });
}

void m2ImzMLExportView::NodeAdded(const mitk::DataNode *)
{
  
}
