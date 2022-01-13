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
#include <berryPlatform.h>

// Qt
#include <QMessageBox>

// mitk
#include <mitkNodePredicateDataType.h>
#include <mitkProgressBar.h>

// m2aia
#include "m2Reconstruction3D.h"
#include "m2StackNameDialog.h"
#include <m2ElxRegistrationHelper.h>
#include <m2ElxUtil.h>
#include <m2ImzMLSpectrumImage.h>

const std::string m2Reconstruction3D::VIEW_ID = "org.mitk.views.m2.Reconstruction3D";

void m2Reconstruction3D::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Parent = parent;
  m_Controls.setupUi(parent);
  connect(m_Controls.btnUpdateList, &QPushButton::clicked, this, &m2Reconstruction3D::OnUpdateList);

  connect(m_Controls.btnStartStacking, SIGNAL(clicked()), this, SLOT(OnStartStacking()));

  {
    auto customList = new QListWidget();
    m_List1 = customList;
    m_Controls.dataSelection->addWidget(customList);

    customList->setDragEnabled(true);
    customList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    customList->setDragDropMode(QAbstractItemView::DragDrop);
    customList->setDefaultDropAction(Qt::MoveAction);
    customList->setAlternatingRowColors(true);
    customList->setSelectionMode(QAbstractItemView::MultiSelection);
    customList->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
    customList->setSortingEnabled(true);
  }
  {
    auto customList = new QListWidget();
    m_Controls.dataSelection->addWidget(customList);
    m_List2 = customList;
    customList->setDragEnabled(true);
    customList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    customList->setDragDropMode(QAbstractItemView::DragDrop);
    customList->setDefaultDropAction(Qt::MoveAction);
    customList->setAlternatingRowColors(true);
    customList->setSelectionMode(QAbstractItemView::MultiSelection);
    customList->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
    customList->setSortingEnabled(true);
  }
}

void m2Reconstruction3D::OnStartStacking()
{
  // helper function
  auto getImageDataById = [this](auto id, QListWidget *listWidget) -> DataTuple
  {
    auto referenceIndex = listWidget->item(id)->data(Qt::UserRole).toUInt();
    return m_referenceMap.at(referenceIndex);
  };


  // check input data
  const auto numItems = m_List1->count();
  const bool PerformMultimodalSteps = m_List1->count() == m_List2->count();
  if(!numItems){
    QMessageBox::warning(m_Parent, "No data available!", "The list for 3D reconstruction must not be empty!");
    return;
  }

  // name of the result stack
  std::string stackName;
  {
    m2StackNameDialog dialog(m_Parent);
    auto d = getImageDataById(0,m_List1);
    if(auto bp = d.image->GetProperty("x_range_center")){
      auto center = dynamic_cast<mitk::DoubleProperty *>(bp.GetPointer())->GetValueAsString();
      dialog.SetName(std::string("stack") + " " + center);
    }
    if(dialog.exec()){
      stackName = dialog.GetName();
    }else{
      MITK_WARN << "Cancle stacking.";
      return;
    }
    
  }

  // prepare stacks
  auto spectrumImageStack1 = m2::SpectrumImageStack::New(m2::MicroMeterToMilliMeter(m_Controls.spinBoxZSpacing->value()));
  auto spectrumImageStack2 = m2::SpectrumImageStack::New(m2::MicroMeterToMilliMeter(m_Controls.spinBoxZSpacing->value()));
  /*
   * Two modalities
   * M2 is optional
   * List size = 6
   * if size of M2 is not equal to size of M1; W2-path is ignored.
   * _M1____M2___________
   * |0|   |0|
   * |1|   |1|
   * |2|   |2| Start index
   * |3|   |3|
   * |4|   |4|
   * |5|   |5|
   * '''''''''''''''
   *
   * M1-M1 order: 2-1 1-0 2-3 3-4 4-5  --> W1
   * M2-W1 order: 2-2 1-1 0-0 3-3 4-4 5-5 --> W2
   */

  // prepare workbench

  
  if (PerformMultimodalSteps)
  {
    mitk::ProgressBar::GetInstance()->AddStepsToDo(numItems * 2 - 1);
  }
  else
  {
    mitk::ProgressBar::GetInstance()->AddStepsToDo(numItems - 1);
  }

  // prepare parameter files
  auto rigidParameters = m_Controls.textRigid->toPlainText().toStdString();
  m2::ElxUtil::ReplaceParameter(
    rigidParameters, "MaximumNumberOfIterations", std::to_string(m_Controls.RigidMaxIters->value()));

  auto deformableParameters = m_Controls.textDeformable->toPlainText().toStdString();
  m2::ElxUtil::ReplaceParameter(
    deformableParameters, "MaximumNumberOfIterations", std::to_string(m_Controls.DeformableMaxIters->value()));

  std::vector<std::string> parameter{rigidParameters, deformableParameters};

  const bool UseSubsequentOrdering = m_Controls.chkBxUseConsecutiveSequence->isChecked();
  const auto currentRow = m_List1->currentRow() < 0 ? numItems / 2 : m_List1->currentRow();

  // Initialize by fixed image of stack 1
  {
    auto M1 = getImageDataById(currentRow, m_List1);
    auto elxHelper = std::make_shared<m2::ElxRegistrationHelper>();
    elxHelper->SetImageData(M1.image, M1.image);
    elxHelper->SetRegistrationParameters({}); // identity
    spectrumImageStack1->Insert(currentRow, elxHelper);
  }

  auto registrationStep = [&](auto fixedId, auto movingId){
  

    const auto & transformer = spectrumImageStack1->GetSliceTransformers().at(fixedId);
    auto fixedImage = transformer->GetMovingImage();
    
    if(!transformer->GetTransformation().empty())
      fixedImage = transformer->WarpImage(fixedImage);
    
    auto movingData = getImageDataById(movingId, m_List1);
    auto elxHelper = std::make_shared<m2::ElxRegistrationHelper>();
    elxHelper->SetImageData(fixedImage, movingData.image);
    elxHelper->SetRegistrationParameters(parameter);
    elxHelper->GetRegistration();
    spectrumImageStack1->Insert(movingId, elxHelper);
   
  };

  for (int movingId = currentRow - 1; movingId >= 0; --movingId)
  {
    int fixedId = UseSubsequentOrdering ? movingId + 1 : currentRow;
    registrationStep(fixedId, movingId);
    mitk::ProgressBar::GetInstance()->Progress();
  }

  for (int movingId = currentRow + 1; movingId < numItems; ++movingId)
  {
    int fixedId = UseSubsequentOrdering ? movingId - 1 : currentRow;
    registrationStep(fixedId, movingId);
    mitk::ProgressBar::GetInstance()->Progress();
  }

  spectrumImageStack1->InitializeProcessor();
  spectrumImageStack1->InitializeGeometry();

  auto node = mitk::DataNode::New();
  node->SetData(spectrumImageStack1);
  node->SetName(stackName);
  GetDataStorage()->Add(node);
  // future release all connections and destroy itself
  // future->disconnect();
  // });


  // m_Controls.btnCancel->setEnabled(true);
}

void m2Reconstruction3D::OnUpdateList()
{
  auto all = this->GetDataStorage()->GetAll();
  m_List1->clear();
  m_List2->clear();
  m_referenceMap.clear();
  unsigned int i = 0;
  // iterate all objects in data storage and create a list widged item
  //  unsigned id = 1;
  for (mitk::DataNode::Pointer node : *all)
  {
    if (node.IsNull())
      continue;
    if (auto data = dynamic_cast<m2::SpectrumImageBase *>(node->GetData()))
    {
      auto res = this->GetDataStorage()->GetDerivations(node, mitk::TNodePredicateDataType<mitk::PointSet>::New());

      auto *item = new QListWidgetItem();
      item->setText((node->GetName()).c_str());
      item->setData(Qt::UserRole, QVariant::fromValue(i));

      DataTuple tuple;
      tuple.image = data;
      tuple.mask = data->GetMaskImage();
      // tuple.points = data->Get

      m_referenceMap[i] = tuple;

      ++i;
      m_List1->addItem(item);
    }
  }
}
