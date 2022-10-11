/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateOr.h>
#include <mitkNodePredicateProperty.h>

#include <usModuleRegistry.h>

#include <QMessageBox>

#include "QmitkExampleView.h"
#include <mitkImage.h>


// Don't forget to initialize the VIEW_ID.
const std::string QmitkExampleView::VIEW_ID = "org.mitk.views.m2.exampleview";

void QmitkExampleView::CreateQtPartControl(QWidget* parent)
{
  // Setting up the UI is a true pleasure when using .ui files, isn't it?
  m_Controls.setupUi(parent);

  m_Controls.selectionWidget->SetDataStorage(this->GetDataStorage());
  m_Controls.selectionWidget->SetSelectionIsOptional(true);
  m_Controls.selectionWidget->SetEmptyInfo(QStringLiteral("Select an image"));
  m_Controls.selectionWidget->SetNodePredicate(mitk::NodePredicateAnd::New(
    mitk::TNodePredicateDataType<mitk::Image>::New(),
    mitk::NodePredicateNot::New(mitk::NodePredicateOr::New(
      mitk::NodePredicateProperty::New("helper object"),
      mitk::NodePredicateProperty::New("hidden object")))));

  // Wire up the UI widgets with our functionality.
  connect(m_Controls.selectionWidget, &QmitkSingleNodeSelectionWidget::CurrentSelectionChanged, this, &QmitkExampleView::OnImageChanged);
  connect(m_Controls.processImageButton, SIGNAL(clicked()), this, SLOT(ProcessSelectedImage()));

  // Make sure to have a consistent UI state at the very beginning.
  this->OnImageChanged(m_Controls.selectionWidget->GetSelectedNodes());
}

void QmitkExampleView::SetFocus()
{
  m_Controls.processImageButton->setFocus();
}

void QmitkExampleView::OnImageChanged(const QmitkSingleNodeSelectionWidget::NodeList&)
{
  this->EnableWidgets(m_Controls.selectionWidget->GetSelectedNode().IsNotNull());
}

void QmitkExampleView::EnableWidgets(bool enable)
{
  m_Controls.processImageButton->setEnabled(enable);
}

void QmitkExampleView::ProcessSelectedImage()
{
 
}
