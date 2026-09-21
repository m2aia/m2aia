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
#include <QFileDialog>
#include <QMessageBox>
#include <QThreadPool>
#include <QVBoxLayout>
#include <QtConcurrent>

// mitk
#include <itkNrrdImageIO.h>
#include <mitkImageAccessByItk.h>
#include <mitkImageCast.h>
#include <mitkNodePredicateDataProperty.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateOr.h>
#include <mitkProgressBar.h>
#include <mitkStringProperty.h>
// #include <mitkImageWriter.h>
#include <mitkIOUtil.h>
#include <mitkItkImageIO.h>

// m2aia
#include "m2Reconstruction3D.h"
#include <Qm2NameDialog.h>
#include <m2DataNodePredicates.h>
#include <m2ElxRegistrationHelper.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2UIUtils.h>
#include <signal/m2SignalCommon.h>

const std::string m2Reconstruction3D::VIEW_ID = "org.mitk.views.m2.reconstruction3D";

void m2Reconstruction3D::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Parent = parent;
  m_Controls.setupUi(parent);

  m_Controls.centroidListSelector->SetDataStorage(GetDataStorage());
  m_Controls.centroidListSelector->SetSelectionIsOptional(true);
  m_Controls.centroidListSelector->SetAutoSelectNewNodes(false);
  m_Controls.centroidListSelector->SetEmptyInfo(QString("Centroid Spectrum"));
  m_Controls.centroidListSelector->SetPopUpTitel(QString("Centroid Spectrum"));
  m_Controls.centroidListSelector->SetNodePredicate(
    mitk::NodePredicateAnd::New(m2::DataNodePredicates::NoActiveHelper,
                                mitk::NodePredicateOr::New(m2::DataNodePredicates::IsOverviewSpectrum,
                                                           m2::DataNodePredicates::IsCentroidSpectrum,
                                                           m2::DataNodePredicates::IsProfileSpectrum)));

  m_Controls.stackImageSelector->SetDataStorage(GetDataStorage());
  m_Controls.stackImageSelector->SetSelectionIsOptional(true);
  m_Controls.stackImageSelector->SetAutoSelectNewNodes(false);
  m_Controls.stackImageSelector->SetEmptyInfo(QString("Stack Image"));
  m_Controls.stackImageSelector->SetPopUpTitel(QString("Stack Image"));
  m_Controls.stackImageSelector->SetNodePredicate(
    mitk::NodePredicateAnd::New(m2::DataNodePredicates::NoActiveHelper, m2::DataNodePredicates::IsSpectrumImageStack));

  connect(m_Controls.btnUpdateList, &QPushButton::clicked, this, &m2Reconstruction3D::OnUpdateList);
  connect(m_Controls.btnStartStacking, SIGNAL(clicked()), this, SLOT(OnStartStacking()));
  connect(m_Controls.btnStartStackExport, SIGNAL(clicked()), this, SLOT(OnStartExport()));

  {
    m_List1 = m_Controls.modality1ListWidget;
    m_List1->setDragEnabled(true);
    m_List1->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_List1->setDragDropMode(QAbstractItemView::NoDragDrop);
    // m_List1->setDefaultDropAction(Qt::MoveAction);
    m_List1->setAlternatingRowColors(true);
    m_List1->setSelectionMode(QAbstractItemView::MultiSelection);
    m_List1->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
    m_List1->setSortingEnabled(true);
  }
  {
    m_List2 = m_Controls.modality2ListWidget;
    m_List2->setDragEnabled(true);
    m_List2->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_List2->setDragDropMode(QAbstractItemView::NoDragDrop);
    // m_List2->setDefaultDropAction(Qt::MoveAction);
    m_List2->setAlternatingRowColors(true);
    m_List2->setSelectionMode(QAbstractItemView::MultiSelection);
    m_List2->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
    m_List2->setSortingEnabled(true);
  }

  QComboBox *imageTypeSelection = m_Controls.imageTypeSelection;
  for (auto type : m2::NormalizationStrategyTypeList)
  {
    imageTypeSelection->addItem(QString::fromStdString(m2::to_string(type)), QVariant::fromValue(to_underlying(type)));
  }

  connect(m_Controls.btnRemove,
          &QAbstractButton::clicked,
          this,
          [this]()
          {
            for (auto item : m_List1->selectedItems())
            {
              delete m_List1->takeItem(m_List1->row(item));
            }

            for (auto item : m_List2->selectedItems())
            {
              delete m_List2->takeItem(m_List2->row(item));
            }
          });

  connect(m_Controls.btnSwitch,
          &QAbstractButton::clicked,
          this,
          [this]()
          {
            for (auto item : m_List1->selectedItems())
            {
              m_List2->addItem(m_List1->takeItem(m_List1->row(item)));
            }

            for (auto item : m_List2->selectedItems())
            {
              m_List1->addItem(m_List2->takeItem(m_List2->row(item)));
            }
          });

  // Install the registration parameter widget into the UI placeholder
  m_ElxParamWidget = new Qm2ElxParameterWidget(parent);
  auto *elxLayout = new QVBoxLayout(m_Controls.elxParamWidget);
  elxLayout->setContentsMargins(0, 0, 0, 0);
  elxLayout->addWidget(m_ElxParamWidget);
}

m2Reconstruction3D::DataTuple m2Reconstruction3D::GetImageDataById(unsigned int id, QListWidget *listWidget)
{
  auto referenceIndex = listWidget->item(id)->data(Qt::UserRole).toUInt();
  return m_referenceMap.at(referenceIndex);
}

std::shared_ptr<m2::ElxRegistrationHelper> m2Reconstruction3D::RegistrationStep(
  const DataTuple &fixedData,
  std::shared_ptr<m2::ElxRegistrationHelper> fixedTransformer,
  const DataTuple &movingData,
  const std::vector<std::string> &parameters,
  m2::NormalizationStrategyType normType)
{
  mitk::Image::Pointer fixedImage = fixedData.image;
  mitk::Image::Pointer movingImage = movingData.image;

  if (normType != m2::NormalizationStrategyType::None)
  {
    if (auto sImage = dynamic_cast<m2::SpectrumImage *>(fixedData.image.GetPointer()))
      fixedImage = sImage->GetNormalizationImage(normType);

    if (auto sImage = dynamic_cast<m2::SpectrumImage *>(movingData.image.GetPointer()))
      movingImage = sImage->GetNormalizationImage(normType);
  }

  // check if a transformer exist for fixed image and apply
  if (fixedTransformer && !fixedTransformer->GetTransformation().empty())
    fixedImage = fixedTransformer->WarpImage(fixedImage);

  // start of the registration procedure
  auto elxHelper = std::make_shared<m2::ElxRegistrationHelper>();
  elxHelper->SetImageData(fixedImage, movingImage);
  elxHelper->SetRegistrationParameters(parameters);
  MITK_INFO << "Registering " << movingData.node->GetName() << " to " << fixedData.node->GetName();
  elxHelper->GetRegistration();

  // workaround to use NormalizationImages
  if (normType != m2::NormalizationStrategyType::None)
  {
    if (fixedTransformer && !fixedTransformer->GetTransformation().empty())
      fixedImage = fixedTransformer->WarpImage(fixedData.image);
    elxHelper->SetImageData(fixedImage, movingData.image);
  }

  return elxHelper;
}

std::vector<std::string> m2Reconstruction3D::GetParameters()
{
  return m_ElxParamWidget->GetParameters();
}

void m2Reconstruction3D::OnStartStacking()
{
  if (m_ReconstructionFutureWatcher.isRunning())
    return;

  // check input data
  const auto numItems = m_List1->count();
  const bool doMultiModalImageRegistration = m_List1->count() == m_List2->count();

  if (!numItems)
  {
    QMessageBox::warning(m_Parent, "No data available!", "The list for 3D reconstruction must not be empty!");
    return;
  }

  // name of the result stack
  std::array<std::string, 2> stackNames;
  std::array<QListWidget *, 2> lists = {m_List1, m_List2};
  for (int i = 0; i < 1 + doMultiModalImageRegistration; ++i)
  {
    auto d = GetImageDataById(0, lists[i]);

    Qm2NameDialog dialog(m_Parent);
    if (auto bp = d.image->GetProperty("m2aia.xs.selection.center"))
    {
      auto center = dynamic_cast<mitk::DoubleProperty *>(bp.GetPointer())->GetValueAsString();
      dialog.SetName(std::string("stack") + " " + center);
    }
    if (dialog.exec())
    {
      stackNames[i] = dialog.GetName();
    }
    else
    {
      MITK_WARN << "Cancel stacking.";
      return;
    }
  }
  auto stackSize = m_List1->count();
  double spacingZ = m2::MicroMeterToMilliMeter(m_Controls.spinBoxZSpacing->value());
  // prepare stacks
  auto spectrumImageStack1 = m2::SpectrumImageStack::New(stackSize, spacingZ);
  m2::SpectrumImageStack::Pointer spectrumImageStack2;
  if (doMultiModalImageRegistration)
    spectrumImageStack2 = m2::SpectrumImageStack::New(stackSize, spacingZ);
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

  // collect all inputs on the GUI thread, the worker must not access widgets or members
  std::vector<DataTuple> slices1, slices2;
  for (int i = 0; i < numItems; ++i)
  {
    slices1.push_back(GetImageDataById(i, m_List1));
    if (doMultiModalImageRegistration)
      slices2.push_back(GetImageDataById(i, m_List2));
  }
  const auto parameters = GetParameters();
  const auto normType =
    static_cast<m2::NormalizationStrategyType>(m_Controls.imageTypeSelection->currentData().toInt());
  const bool UseSubsequentOrdering = !m_Controls.chkBxCoRegistrationToSelected->isChecked();
  const int currentRow = m_List1->currentRow() < 0 ? numItems / 2 : m_List1->currentRow();

  // set by the worker if the reconstruction fails
  auto errorMessage = std::make_shared<std::string>();

  // prepare workbench
  mitk::ProgressBar::GetInstance()->AddStepsToDo(numItems - 1);
  m_Controls.btnStartStacking->setEnabled(false);
  m_Controls.btnUpdateList->setEnabled(false);

  // disconnect all signals
  disconnect(&m_ReconstructionFutureWatcher, &QFutureWatcher<void>::finished, nullptr, nullptr);
  disconnect(&m_ReconstructionFutureWatcher, &QFutureWatcher<void>::progressValueChanged, nullptr, nullptr);

  // Handle finished
  connect(&m_ReconstructionFutureWatcher,
          &QFutureWatcher<void>::finished,
          this,
          [stackNames, spectrumImageStack1, spectrumImageStack2, errorMessage, this]()
          {
            mitk::ProgressBar::GetInstance()->Reset();
            ResetToInitialState();

            // the stacks are only initialized if the worker ran through
            if (!errorMessage->empty())
            {
              QMessageBox::critical(m_Parent, "Reconstruction failed", QString::fromStdString(*errorMessage));
              return;
            }

            QMessageBox::information(m_Parent, "Reconstruction Complete", "Reconstruction complete!");

            auto node = mitk::DataNode::New();
            node->SetData(spectrumImageStack1);
            node->SetName(stackNames[0]);
            GetDataStorage()->Add(node);

            // auto transformers = spectrumImageStack1->GetSliceTransformers();
            // unsigned int i = 0;
            // for(auto t: transformers){
            //   if(t){
            //     auto node2 = mitk::DataNode::New();
            //     node2->SetData(t->GetDeformationField());
            //     node2->SetName("Transform " + std::to_string(i));
            //     node2->SetVisibility(false);
            //     node2->SetBoolProperty("helper object", true);
            //     GetDataStorage()->Add(node2);
            //   }
            // }

            if (spectrumImageStack2)
            {
              node = mitk::DataNode::New();
              node->SetData(spectrumImageStack2);
              node->SetName(stackNames[1] + "(2)");
              GetDataStorage()->Add(node);

              // auto transformers = spectrumImageStack2->GetSliceTransformers();
              // unsigned int i = 0;
              // for(auto t: transformers){
              //   if(t){
              //     auto node2 = mitk::DataNode::New();
              //     node2->SetData(t->GetDeformationField());
              //     node2->SetName("Transform " + std::to_string(i));
              //     node2->SetVisibility(false);
              //     node2->SetBoolProperty("helper object", true);
              //     GetDataStorage()->Add(node2);
              //   }
              // }
            }
          });

  connect(&m_ReconstructionFutureWatcher,
          &QFutureWatcher<void>::progressValueChanged,
          this,
          [&](int) { mitk::ProgressBar::GetInstance()->Progress(); });

  m_ReconstructionFutureWatcher.setFuture(QtConcurrent::run(
    [slices1,
     slices2,
     parameters,
     normType,
     UseSubsequentOrdering,
     currentRow,
     numItems,
     spectrumImageStack1,
     spectrumImageStack2,
     errorMessage]()
    {
      try
      {
        // Initialize by fixed image of stack 1
        {
          const auto &M1 = slices1[currentRow];
          auto elxHelper = std::make_shared<m2::ElxRegistrationHelper>();
          elxHelper->SetImageData(M1.image, M1.image);
          elxHelper->SetRegistrationParameters({});
          spectrumImageStack1->Insert(currentRow, elxHelper);
        }

        // Initialize stack 2
        if (spectrumImageStack2)
        {
          auto elxHelper = RegistrationStep(slices1[currentRow], nullptr, slices2[currentRow], parameters, normType);
          spectrumImageStack2->Insert(currentRow, elxHelper);
        }

        for (int movingId = currentRow - 1; movingId >= 0; --movingId)
        {
          // stack 1
          int fixedId = UseSubsequentOrdering ? movingId + 1 : currentRow;
          auto fixedTransformer = spectrumImageStack1->GetSliceTransformers().at(fixedId);
          auto elxHelper =
            RegistrationStep(slices1[fixedId], fixedTransformer, slices1[movingId], parameters, normType);
          spectrumImageStack1->Insert(movingId, elxHelper);

          // stack 2
          if (spectrumImageStack2)
          {
            elxHelper = RegistrationStep(slices1[movingId], elxHelper, slices2[movingId], parameters, normType);
            spectrumImageStack2->Insert(movingId, elxHelper);
          }
        }

        for (int movingId = currentRow + 1; movingId < numItems; ++movingId)
        {
          // stack 1
          int fixedId = UseSubsequentOrdering ? movingId - 1 : currentRow;
          auto fixedTransformer = spectrumImageStack1->GetSliceTransformers().at(fixedId);
          auto elxHelper =
            RegistrationStep(slices1[fixedId], fixedTransformer, slices1[movingId], parameters, normType);
          spectrumImageStack1->Insert(movingId, elxHelper);

          // stack 2
          if (spectrumImageStack2)
          {
            elxHelper = RegistrationStep(slices1[movingId], elxHelper, slices2[movingId], parameters, normType);
            spectrumImageStack2->Insert(movingId, elxHelper);
          }
        }

        spectrumImageStack1->InitializeProcessor();
        spectrumImageStack1->InitializeGeometry();

        if (spectrumImageStack2)
        {
          spectrumImageStack2->InitializeProcessor();
          spectrumImageStack2->InitializeGeometry();
        }
      }
      catch (const std::exception &e)
      {
        *errorMessage = e.what();
        MITK_ERROR << "3D reconstruction failed: " << e.what();
      }
      catch (...)
      {
        *errorMessage = "Unknown error during 3D reconstruction.";
        MITK_ERROR << *errorMessage;
      }
    }));
}

void m2Reconstruction3D::ResetToInitialState()
{
  m_List1->clear();
  m_List2->clear();
  m_referenceMap.clear();
  m_Controls.btnStartStacking->setEnabled(true);
  m_Controls.btnUpdateList->setEnabled(true);
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
    // results of previous reconstructions are no input slices
    if (dynamic_cast<m2::SpectrumImageStack *>(node->GetData()))
      continue;
    if (auto data = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
    {
      auto res = this->GetDataStorage()->GetDerivations(node, mitk::TNodePredicateDataType<mitk::PointSet>::New());

      auto *item = new QListWidgetItem();
      item->setText((node->GetName()).c_str());
      item->setData(Qt::UserRole, QVariant::fromValue(i));

      DataTuple tuple;
      tuple.node = node;
      tuple.image = data;
      tuple.mask = data->GetMultilabelSegmentation()->GetGroupImage(0);
      // tuple.points = data->Get

      m_referenceMap[i] = tuple;

      ++i;
      m_List1->addItem(item);
    }
  }
}

void m2Reconstruction3D::OnStartExport()
{
  if (m_ExportProcessFutureWatcher.future().isRunning())
    return;

  auto centroidsNode = m_Controls.centroidListSelector->GetSelectedNode();
  auto stackImageNode = m_Controls.stackImageSelector->GetSelectedNode();

  if (!centroidsNode || !stackImageNode)
  {
    QMessageBox::warning(m_Parent, "No data available!", "Please select a centroid spectrum and a stack image.");
    return;
  }

  // disconnect all signals
  disconnect(&m_ExportProcessFutureWatcher, &QFutureWatcher<void>::finished, nullptr, nullptr);
  disconnect(&m_ExportProcessFutureWatcher, &QFutureWatcher<void>::progressValueChanged, nullptr, nullptr);

  QString dir = QFileDialog::getExistingDirectory(
    m_Parent, tr("Select Directory"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (dir.isEmpty())
    return;

  // Handle finished
  connect(&m_ExportProcessFutureWatcher,
          &QFutureWatcher<void>::finished,
          this,
          [&]()
          {
            QMessageBox::information(m_Parent, "Export Complete", "All images have been exported successfully.");
            mitk::ProgressBar::GetInstance()->Reset();
          });

  connect(&m_ExportProcessFutureWatcher,
          &QFutureWatcher<void>::progressValueChanged,
          this,
          [&](int) { mitk::ProgressBar::GetInstance()->Progress(); });

  // auto intervals = dynamic_cast<m2::IntervalVector*>(centroidsNode->GetData());
  // mitk::ProgressBar::GetInstance()->AddStepsToDo(intervals->GetIntervals().size());
  // auto stackImage = dynamic_cast<m2::SpectrumImageStack*>(stackImageNode->GetData());
  // auto intervals = dynamic_cast<m2::IntervalVector*>(centroidsNode->GetData());
  // for(std::shared_ptr<m2::ElxRegistrationHelper> t : stackImage->GetSliceTransformers()){
  // if(t->GetDeformationField()){
  // auto prop = t->GetMovingImage()->GetPropertyList()->GetProperty("path");
  // if(prop){
  //   auto path = prop->GetValueAsString();
  //   if(path.empty()) continue;

  // auto pos = path.find_last_of("/");
  // if(pos != std::string::npos){
  // auto fileName = path.substr(pos + 1);
  // auto fileNameWithoutExtension = fileName.substr(0, fileName.find_last_of("."));
  // mitk::Image::Pointer deformationfield = t->GetDeformationField();
  // auto node = mitk::DataNode::New();
  // node->SetData(deformationfield);
  // node->SetName(fileNameWithoutExtension + ".def");
  // auto ref = GetDataStorage()->GetNamedNode(fileNameWithoutExtension);
  // GetDataStorage()->Add(node, ref);

  // }
  // }
  //   }
  // }

  // Start the export
  m_ExportProcessFutureWatcher.setFuture(QtConcurrent::run(
  [dir, centroidsNode, stackImageNode](){
    QFutureInterface<void> futureInterface;

    auto stackImage = dynamic_cast<m2::SpectrumImageStack *>(stackImageNode->GetData());
    auto intervals = dynamic_cast<m2::IntervalVector *>(centroidsNode->GetData());

    std::ofstream logs(dir.toStdString() + "/logs.txt");
    logs << "Exporting stack image to " << dir.toStdString() << std::endl;
    logs << "Number of intervals: " << intervals->GetIntervals().size() << std::endl;
    logs << "3D Recon. at m/z " << stackImageNode->GetName() << std::endl;
    logs << "Baseline correction: "
         << m2::BaselineCorrectionTypeNames.at(to_underlying(stackImage->GetBaselineCorrectionStrategy())) << std::endl;
    logs << "Baseline correction half window size: " << stackImage->GetBaseLineCorrectionHalfWindowSize() << std::endl;
    logs << "Normalization strategy: "
         << m2::NormalizationStrategyTypeNames.at(to_underlying(stackImage->GetNormalizationStrategy())) << std::endl;
    logs << "Smoothing strategy: " << m2::SmoothingTypeNames.at(to_underlying(stackImage->GetSmoothingStrategy()))
         << std::endl;
    logs << "Smoothing half window size: " << stackImage->GetSmoothingHalfWindowSize() << std::endl;
    logs << "Intensity transformation strategy: "
         << m2::IntensityTransformationTypeNames.at(to_underlying(stackImage->GetIntensityTransformationStrategy()))
         << std::endl;
    logs << "Image smoothing strategy: "
         << m2::ImageNormalizationStrategyTypeNames.at(to_underlying(stackImage->GetImageSmoothingStrategy()))
         << std::endl;
    logs << "Image normalization strategy: "
         << m2::ImageNormalizationStrategyTypeNames.at(to_underlying(stackImage->GetImageNormalizationStrategy()))
         << std::endl;

    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    logs << "Export started at " << std::ctime(&currentTime) << std::endl;

    mitk::IOUtil::Save(stackImage->GetMultilabelSegmentation()->GetGroupImage(0),
                       dir.toStdString() + "/MaskImage.nrrd");
    unsigned i = 0;
    unsigned n = stackImage->GetSliceTransformers().size();
    for (std::shared_ptr<m2::ElxRegistrationHelper> t : stackImage->GetSliceTransformers())
    {
      if (t->GetDeformationField())
      {
        std::string movingPath;
        auto m2aiaDataPathProp = t->GetMovingImage()->GetProperty("m2aia.IO.path");
        auto dataPathProp = t->GetMovingImage()->GetProperty("path");
        if (m2aiaDataPathProp)
          movingPath = m2aiaDataPathProp->GetValueAsString();
        else if (dataPathProp)
          movingPath = dataPathProp->GetValueAsString();
        else
        {
          MITK_WARN << "No path property found for moving image. Skipping deformation field export";
          continue;
        }



        auto pos = movingPath.find_last_of("/");
        if (pos != std::string::npos)
        {
          auto fileName = movingPath.substr(pos + 1);
          auto fileNameWithoutExtension = fileName.substr(0, fileName.find_last_of("."));
          mitk::Image::Pointer deformationfield = t->GetDeformationField();

          deformationfield->SetProperty("m2aia.stack.index",
                                        mitk::StringProperty::New(std::to_string(++i) + "/" + std::to_string(n)));
          // deformationfield->SetMetaDataDictionary(metaData);
          // itk::NrrdImageIO::Pointer io = itk::NrrdImageIO::New();
          // io->SetFileName(dir.toStdString() + "/" + fileNameWithoutExtension + ".def.nrrd");
          // io->Write
          // auto & dict = io->GetMetaDataDictionary();
          // itk::EncapsulateMetaData<std::string>(dict, "m2aia.stack.index", std::to_string(i) + "/" +
          // std::to_string(n));

          // mitk::ItkImageIO io2(io);
          // io2.SetOutputLocation(dir.toStdString() + "/" + fileNameWithoutExtension + ".def.nrrd");
          // io2.mitk::AbstractFileIOWriter::SetInput(deformationfield);
          // io2.Write();

          mitk::IOUtil::Save(deformationfield, dir.toStdString() + "/" + fileNameWithoutExtension + ".def.nrrd");

          // AccessByItk(deformationfield, ([&](auto I){
          //   auto & dict = I->GetMetaDataDictionary();
          //   itk::EncapsulateMetaData<std::string>(dict, "m2aia.stack.index", std::to_string(i) + "/" +
          //   std::to_string(n)); mitk::CastToMitkImage(I, deformationfield);
          // }));
        }
      }
    }

    futureInterface.setProgressRange(0, intervals->GetIntervals().size());

    futureInterface.reportStarted();
    int progress = 0; 
    auto localStackImage = mitk::Image::New();
    localStackImage->Initialize(stackImage);


    for (auto I : intervals->GetIntervals())
    {
    auto center = I.x.mean();
    // tolerance in 5 ppm
    auto tol = center * 5e-6;

    stackImage->GetImage(center, tol, nullptr, localStackImage);

    auto fileName = dir + "/Stack3D_" + QString("%1").arg(center, 6, 'f', 2) + ".nrrd";
    mitk::IOUtil::Save(localStackImage, fileName.toStdString());

    if (futureInterface.isCanceled())
    {
      qDebug() << "Task canceled!";
      break;
    }
    futureInterface.setProgressValue(++progress);
    }

    now = std::chrono::system_clock::now();
    currentTime = std::chrono::system_clock::to_time_t(now);
    logs << "Export finished at " << std::ctime(&currentTime) << std::endl;
    futureInterface.reportFinished();
}));
}
