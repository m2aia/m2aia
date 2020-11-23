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
#include "m2Reconstruction3D.h"

// Qt
#include <QMessageBox>
#include <QtConcurrent>

// mitk image
#include "m2LandmarkEvaluation.h"
#include <berryPlatform.h>
#include <itkFlatStructuringElement.h>
#include <itkGrayscaleDilateImageFilter.h>
#include <itkGrayscaleErodeImageFilter.h>
#include <itkMedianImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkNormalizeImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itksys/SystemTools.hxx>
#include <m2MSImageBase.h>
#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkImage3DSliceToImage2DFilter.h>
#include <mitkImageCast.h>
#include <mitkNodePredicateDataType.h>
#include <mitkProgressBar.h>
#include <mitkTransformixMSDataObjectStack.h>
const std::string m2Reconstruction3D::VIEW_ID = "org.mitk.views.m2.Reconstruction3D";

QString m2Reconstruction3D::GetTransformixPath() const
{
  berry::IPreferences::Pointer preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.ext.externalprograms");

  return preferences.IsNotNull() ? preferences->Get("transformix", "") : "";
}

QString m2Reconstruction3D::GetElastixPath() const
{
  berry::IPreferences::Pointer preferences =
    berry::Platform::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.ext.externalprograms");

  return preferences.IsNotNull() ? preferences->Get("elastix", "") : "";
}

void m2Reconstruction3D::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  connect(m_Controls.btnUpdateList, &QPushButton::clicked, this, &m2Reconstruction3D::OnUpdateList);

  connect(m_Controls.btnStartStacking, SIGNAL(clicked()), this, SLOT(OnStartStacking()));

  {
    auto customList = new QListWidget();
    m_list1 = customList;
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
    m_list2 = customList;
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

void m2Reconstruction3D::WarpPoints(mitk::Image *ionImage,
                                    mitk::PointSet *inPoints,
                                    std::vector<std::string> transformations)
{
  auto filter = mitk::Image3DSliceToImage2DFilter::New();
  filter->SetInput(ionImage);
  filter->Update();
  mitk::Image::Pointer I = filter->GetOutput();

  const auto baseDir = mitk::IOUtil::CreateTemporaryDirectory();

  auto baseDirPath = boost::filesystem::path(baseDir);
  baseDirPath = baseDirPath.lexically_normal();

  std::ofstream f((baseDirPath / "points.txt").string());
  f << "index\n";
  f << std::to_string(inPoints->GetPointSetSeriesSize()) << "\n";
  for (auto it = inPoints->Begin(); it != inPoints->End(); ++it)
  {
    auto &pnt = it->Value();
    itk::Index<3> index;
    mitk::Point3D world;
    world[0] = pnt[0];
    world[1] = pnt[1];
    world[2] = 0;

    I->GetGeometry()->WorldToIndex(world, index);
    f << index[0] << " " << index[1] << "\n";
  }
  f.close();

  QString transformix = GetTransformixPath();

  for (std::string trafo : transformations)
  {
    auto p = baseDirPath / "Transform.txt";
    std::ofstream f(p.c_str());
    f << trafo;
    f.close();

    std::stringstream ss;
    std::vector<std::string> cmd{transformix.toStdString(),
                                 "-def",
                                 (baseDirPath / "points.txt").string(),
                                 "-out",
                                 baseDirPath.string(),
                                 "-tp",
                                 (baseDirPath / ("Transform.txt")).string()};
    for (auto s : cmd)
      ss << s << " ";
    auto res = std::system(ss.str().c_str());
    if (res != 0)
      MITK_WARN << "Some error on system call of transformix";
    boost::filesystem::rename(baseDirPath / "outputpoints.txt", baseDirPath / "points.txt");
  }

  auto pointset = mitk::PointSet::New();
  {
    std::string line, word;
    std::ifstream f((baseDirPath / "points.txt").string());
    while (!f.eof())
    {
      std::getline(f, line);
      mitk::Point3D p;
      p[2] = 0;
      int i = 0;
      std::stringstream iss(line);
      while (iss.eof())
      {
        std::getline(iss, word);
        try
        {
          int v = std::stoi(word);
          p[i++] = v;
        }
        catch (std::exception e)
        {
          break;
        }
      }
      if (i == 2)
        pointset->InsertPoint(p);
    }
    f.close();
  }
}

void m2Reconstruction3D::ExportSlice(mitk::Image *input,
                                     const boost::filesystem::path &directory,
                                     const std::string &name,
                                     bool useNormalization,
                                     bool useInvertIntensities)
{
  const auto ApplyNormalizationFilter = [](auto I, auto &processImage) {
    using ImageType = typename std::remove_pointer<decltype(I)>::type;
    auto filter = itk::NormalizeImageFilter<ImageType, ImageType>::New();
    filter->SetInput(I);
    filter->Update();
    mitk::CastToMitkImage(filter->GetOutput(), processImage);
  };

  const auto ApplyInvertIntensityFilter = [](auto I, auto &processImage) {
    using ImageType = typename std::remove_pointer<decltype(I)>::type;
    auto filter = itk::InvertIntensityImageFilter<ImageType, ImageType>::New();
    filter->SetInput(I);
    filter->SetMaximum(1.0);
    filter->Update();
    mitk::CastToMitkImage(filter->GetOutput(), processImage);
  };

  const auto ApplyRescaleIntensityFilter = [](auto I, auto &processImage) {
    using ImageType = typename std::remove_pointer<decltype(I)>::type;
    auto filter = itk::RescaleIntensityImageFilter<ImageType, ImageType>::New();
    filter->SetInput(I);
    filter->SetOutputMinimum(0);
    filter->SetOutputMaximum(255);
    filter->Update();
    mitk::CastToMitkImage(filter->GetOutput(), processImage);
  };

//  const auto ApplyGrayscaleDilateIntensityFilter = [](auto I, auto &processImage) {
//    using ImageType = typename std::remove_pointer<decltype(I)>::type;
//    using StructuringElementType = itk::FlatStructuringElement<ImageType::ImageDimension>;
//    typename StructuringElementType::RadiusType elementRadius;
//    elementRadius.Fill(1);
//    StructuringElementType structuringElement = StructuringElementType::Box(elementRadius);
//    auto filter = itk::GrayscaleDilateImageFilter<ImageType, ImageType, StructuringElementType>::New();
//    filter->SetInput(I);
//    filter->SetKernel(structuringElement);
//    filter->Update();
//    mitk::CastToMitkImage(filter->GetOutput(), processImage);
//  };

//  const auto ApplyGrayscaleErodeIntensityFilter = [](auto I, auto &processImage) {
//    using ImageType = typename std::remove_pointer<decltype(I)>::type;
//    using StructuringElementType = itk::FlatStructuringElement<ImageType::ImageDimension>;
//    typename StructuringElementType::RadiusType elementRadius;
//    elementRadius.Fill(1);
//    StructuringElementType structuringElement = StructuringElementType::Ball(elementRadius);
//    auto filter = itk::GrayscaleErodeImageFilter<ImageType, ImageType, StructuringElementType>::New();
//    filter->SetInput(I);
//    filter->SetKernel(structuringElement);
//    filter->Update();
//    mitk::CastToMitkImage(filter->GetOutput(), processImage);
//  };

  auto filter = mitk::Image3DSliceToImage2DFilter::New();
  filter->SetInput(input);
  filter->Update();
  mitk::Image::Pointer image = filter->GetOutput();
  mitk::Image::Pointer resultImage;
  if (useNormalization)
  {
    AccessByItk_1(image, (ApplyNormalizationFilter), resultImage);
    image = resultImage;
    if (useInvertIntensities)
    {
      AccessByItk_1(image, (ApplyInvertIntensityFilter), resultImage);
      image = resultImage;
    }
  }
  //AccessByItk_1(image, (ApplyNormalizationFilter), resultImage);
  //image = resultImage;
  AccessByItk_1(image, (ApplyRescaleIntensityFilter), resultImage);
  image = resultImage;
  //AccessByItk_1(image, (ApplyGrayscaleDilateIntensityFilter), resultImage);
  //image = resultImage;
  //AccessByItk_1(image, (ApplyGrayscaleErodeIntensityFilter), resultImage);
  //image = resultImage;

  mitk::IOUtil::Save(image, (directory / name).string());
}

std::shared_ptr<m2Reconstruction3D::DataTupleWarpingResult> m2Reconstruction3D::WarpImage(const DataTuple &fixed,
                                                                                          const DataTuple &moving,
                                                                                          bool useNormalization,
                                                                                          bool useInvertIntensities,
                                                                                          bool omitDeformable)
{
  const auto ReplaceParameter = [](std::string &paramFileString, std::string what, std::string by) -> std::string {
    auto pos1 = paramFileString.find("(" + what);
    auto pos2 = paramFileString.find(')', pos1);
    paramFileString.replace(pos1, pos2 - pos1 + 1, "(" + what + " " + by + ")\n");
    return paramFileString;
  };

  const auto baseDir = mitk::IOUtil::CreateTemporaryDirectory();
  auto baseDirPath = boost::filesystem::path(baseDir);
  baseDirPath = baseDirPath.lexically_normal();

  // fixed and mask: image save images to disk
  ExportSlice(fixed.image, baseDirPath, "fixed.nrrd", useNormalization, useInvertIntensities);
  ExportSlice(fixed.mask, baseDirPath, "fixed_mask.nrrd", false, false);

  // moving and mask: save images to disk
  ExportSlice(moving.image, baseDirPath, "moving.nrrd", useNormalization, useInvertIntensities);
  ExportSlice(moving.mask, baseDirPath, "moving_mask.nrrd", false, false);

  // create folder structure
  QString elastix = GetElastixPath();
  QString transformix = GetTransformixPath();
  auto rigidDirPath = baseDirPath / "rigid";
  boost::filesystem::create_directory(rigidDirPath);
  auto rigidTransformDirPath = baseDirPath / "rigid_mask";
  boost::filesystem::create_directory(rigidTransformDirPath);
  auto nlTransformDirPath = baseDirPath / "nl_mask";
  boost::filesystem::create_directory(nlTransformDirPath);
  auto nlDirPath = baseDirPath / "nl";
  boost::filesystem::create_directory(nlDirPath);

  QProcess process;
  QStringList v;

  // output
  std::vector<std::string> transformations;
  std::vector<DataTuple> warpedImages;

  {
    // adaptions to  parameter file (rigid)
    std::ofstream elxRigid((rigidDirPath / "rigid.txt").string());
    auto param = m_Controls.textRigid->toPlainText().toStdString();
    param = ReplaceParameter(param, "MaximumNumberOfIterations", std::to_string(m_Controls.RigidMaxIters->value()));
    elxRigid << param;
    elxRigid.close();

    v << "-f" << (baseDirPath / "fixed.nrrd").string().c_str() << "-m" << (baseDirPath / "moving.nrrd").string().c_str()
      << "-out" << rigidDirPath.string().c_str() << "-p" << (rigidDirPath / "rigid.txt").string().c_str();

    if (m_Controls.chkBxUseFixedMask->isChecked())
      v << "-fMask" << (baseDirPath / "fixed_mask.nrrd").string().c_str();
    if (m_Controls.chkBxUseMovingMask->isChecked())
      v << "-mMask" << (baseDirPath / "moving_mask.nrrd").string().c_str();

    // run elastix (rigid)
    process.execute(elastix, v);

    // load trafo params (rigid)
    std::ifstream t((rigidDirPath / "TransformParameters.0.txt").string().c_str());
    std::string transformParameters;

    t.seekg(0, std::ios::end);
    transformParameters.reserve(t.tellg());
    t.seekg(0, std::ios::beg);
    transformParameters.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    // store trafo (rigid)
    transformations.push_back(transformParameters);

    // change output type and save trafo file
    transformParameters = ReplaceParameter(transformParameters, "ResultImagePixelType ", "\"unsigned short\"");
    std::ofstream p((rigidTransformDirPath / "TransformParameters.0.txt").string().c_str());
    p << transformParameters;
    p.close();

    // apply trafo to mask (rigid)
    v = QStringList();
    v << "-in" << (baseDirPath / "moving_mask.nrrd").string().c_str() << "-tp"
      << (rigidTransformDirPath / "TransformParameters.0.txt").string().c_str() << "-out"
      << rigidTransformDirPath.string().c_str();
    process.execute(transformix, v);

    // store image artifacts (rigid)
    auto warpedImage = mitk::IOUtil::Load((rigidDirPath / ("result.0.nrrd")).string());
    auto warpedMask = mitk::IOUtil::Load((rigidTransformDirPath / ("result.nrrd")).string());

    DataTuple tuple;
    tuple.image = dynamic_cast<mitk::Image *>(warpedImage[0].GetPointer());
    tuple.mask = dynamic_cast<mitk::Image *>(warpedMask[0].GetPointer());
    warpedImages.push_back(tuple);
  }

  if (!omitDeformable)
  {
    // adaptions to  parameter file (nl)
    std::ofstream elxDeformable((nlDirPath / "nl.txt").string());
    auto param = m_Controls.textDeformable->toPlainText().toStdString();
    param =
      ReplaceParameter(param, "MaximumNumberOfIterations", std::to_string(m_Controls.DeformableMaxIters->value()));
    elxDeformable << param;
    elxDeformable.close();

    v = QStringList();
    v << "-f" << (baseDirPath / "fixed.nrrd").string().c_str() << "-m"
      << (rigidDirPath / "result.0.nrrd").string().c_str() << "-out" << nlDirPath.string().c_str() << "-p"
      << (nlDirPath / "nl.txt").string().c_str();

    if (m_Controls.chkBxUseFixedMaskDeform->isChecked())
      v << "-fMask" << (baseDirPath / "fixed_mask.nrrd").string().c_str();
    if (m_Controls.chkBxUseMovingMaskDeform->isChecked())
      v << "-mMask" << (rigidTransformDirPath / "result.nrrd").string().c_str();

    process.execute(elastix, v);

    // load trafo params (nl)
    std::ifstream t((nlDirPath / "TransformParameters.0.txt").string().c_str());
    std::string transformParameters;

    t.seekg(0, std::ios::end);
    transformParameters.reserve(t.tellg());
    t.seekg(0, std::ios::beg);
    transformParameters.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    // store trafo (nl)
    transformations.push_back(transformParameters);

    // change output type and save trafo file
    transformParameters = ReplaceParameter(transformParameters, "ResultImagePixelType ", "\"unsigned short\"");
    std::ofstream p((nlTransformDirPath / "TransformParameters.0.txt").string().c_str());
    p << transformParameters;
    p.close();

    // apply trafo to mask (nl)
    v = QStringList();
    v << "-in" << (rigidTransformDirPath / "result.nrrd").string().c_str() << "-tp"
      << (nlTransformDirPath / "TransformParameters.0.txt").string().c_str() << "-out"
      << nlTransformDirPath.string().c_str();
    process.execute(transformix, v);

    // store image artifacts (rigid)

    auto warpedImage = mitk::IOUtil::Load((nlDirPath / ("result.0.nrrd")).string());
    auto warpedMask = mitk::IOUtil::Load((nlTransformDirPath / ("result.nrrd")).string());

    DataTuple tuple;
    tuple.image = dynamic_cast<mitk::Image *>(warpedImage[0].GetPointer());
    tuple.mask = dynamic_cast<mitk::Image *>(warpedMask[0].GetPointer());
    warpedImages.push_back(tuple);
  }

  //boost::filesystem::remove_all(baseDirPath);
  return std::make_shared<DataTupleWarpingResult>(transformations, warpedImages);
}

void m2Reconstruction3D::OnStartStacking()
{
  auto worker = [this]() -> std::vector<m2::TransformixMSDataObjectStack::Pointer> {
    auto getImageDataById = [this](auto id, QListWidget *listWidget) -> DataTuple {
      auto referenceIndex = listWidget->item(id)->data(Qt::UserRole).toUInt();
      return m_referenceMap.at(referenceIndex);
    };

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

    const auto UseNormalization = m_Controls.chkBxNormalization->isChecked();
    // applies only on fusion of second modality
    const auto UseInvertIntensities = m_Controls.chkBxInvertIntensities->isChecked();
    const auto OmitDeformableMultiModal = m_Controls.chkBxOmitDeformable->isChecked();

    const auto numItems = m_list1->count();
    const auto currentListRow = m_list1->currentRow() < 0 ? numItems / 2 : m_list1->currentRow();
    const auto listWidged = m_list1;
    const auto listWidged2 = m_list2;

    std::vector<DataTuple> warpedIonImages(numItems);

    m2::TransformixMSDataObjectStack::Pointer stack, stack2;
    stack = m2::TransformixMSDataObjectStack::New(GetTransformixPath().toStdString());
    stack->Resize(numItems);
    if (!stack->IsPathToTransformixValid())
      mitkThrow() << "Invalid transformix path!";

    if (listWidged2->count() == numItems)
    {
      mitk::ProgressBar::GetInstance()->AddStepsToDo(numItems * 2 - 1);
      stack2 = m2::TransformixMSDataObjectStack::New(GetTransformixPath().toStdString());
      stack2->Resize(numItems);
    }
    else
    {
      mitk::ProgressBar::GetInstance()->AddStepsToDo(numItems - 1);
    }

    warpedIonImages[currentListRow] = getImageDataById(currentListRow, listWidged);
    bool UseSubsequentOrdering = m_Controls.chkBxUseConsecutiveSequence->isChecked();

    auto referenceData = getImageDataById(currentListRow, listWidged);
    stack->Insert(currentListRow, dynamic_cast<m2::MSImageBase *>(referenceData.image.GetPointer()), {});

    if (stack2)
    {
      auto ionImage = getImageDataById(currentListRow, listWidged);
      auto ionImage2 = getImageDataById(currentListRow, listWidged2);
      auto result = WarpImage(ionImage, ionImage2, UseNormalization, UseInvertIntensities, OmitDeformableMultiModal);
      mitk::ProgressBar::GetInstance()->Progress();

      stack2->Insert(
        currentListRow, dynamic_cast<m2::MSImageBase *>(ionImage2.image.GetPointer()), result->transformations());
    }

    const auto registrationStep = [&](int movingIdx, int fixedIdx) {
      auto fixed = warpedIonImages[UseSubsequentOrdering == true ? fixedIdx : currentListRow];
      auto moving = getImageDataById(movingIdx, listWidged);
      auto result = WarpImage(fixed, moving, UseNormalization);

      stack->Insert(
        movingIdx, dynamic_cast<m2::MSImageBase *>(moving.image.GetPointer()), result->transformations());
      warpedIonImages[movingIdx] = result->images().back();

      mitk::ProgressBar::GetInstance()->Progress();

      if (stack2.IsNotNull())
      {
        auto ionImage2 = getImageDataById(movingIdx, listWidged2);
        auto result = WarpImage(
          warpedIonImages[movingIdx], ionImage2, UseNormalization, UseInvertIntensities, OmitDeformableMultiModal);
        stack2->Insert(
          movingIdx, dynamic_cast<m2::MSImageBase *>(ionImage2.image.GetPointer()), result->transformations());

        mitk::ProgressBar::GetInstance()->Progress();
      }
    };

    for (int i = currentListRow - 1; i >= 0; --i)
    {
      registrationStep(i, i + 1);
    }

    for (int i = currentListRow + 1; i < numItems; ++i)
    {
      registrationStep(i, i - 1);
    }

    // -------------- Do the work -------------- //

    auto zSpacing = m_Controls.spinBoxZSpacing->value() * 0.001;
    stack->InitializeImages(currentListRow, zSpacing);
    if (stack2.IsNotNull())
      stack2->InitializeImages(currentListRow, zSpacing);

    return {stack, stack2};
  }; // worker end

  // Copy QFutureWatcher shared pointer in lambda (captur list by copy) will keep the QFutureWatcher alive
  // until the finish callback is emitted and the lambda itself is destroyed
  auto future = std::make_shared<QFutureWatcher<std::vector<m2::TransformixMSDataObjectStack::Pointer>>>();

  connect(
    &(*future), &QFutureWatcher<std::vector<m2::TransformixMSDataObjectStack::Pointer>>::finished, future.get(), [=]() {
      unsigned int i = 0;
      for (auto s : future->result())
      {
        if (s)
        {
          auto node = mitk::DataNode::New();
          node->SetName("Stack_" + std::to_string(i));
          node->SetData(s);
          this->GetDataStorage()->Add(node);

          auto node2 = mitk::DataNode::New();
          node2->SetName("Stack_Mask_" + std::to_string(i));
          node2->SetData(s->GetMaskImage());
		  node2->SetVisibility(false);
          this->GetDataStorage()->Add(node2, node);

          auto node3 = mitk::DataNode::New();
          node3->SetName("Stack_Distance_" + std::to_string(i));
          node3->SetData(s->GetImageArtifacts()["distance"]);
		  node3->SetVisibility(false);
          this->GetDataStorage()->Add(node3, node);

         /* if (s->GetImageArtifacts().find("landmarks") != s->GetImageArtifacts().end())
          {
            auto node4 = mitk::DataNode::New();
            node4->SetName("Stack_Landmarks_" + std::to_string(i));
            node4->SetData(s->GetImageArtifacts()["landmarks"]);
			node4->SetVisibility(false);
            this->GetDataStorage()->Add(node4, node);

            auto warpedPoints = mitk::PointSet::New();
			auto landmarkImage = dynamic_cast<mitk::Image*>(s->GetImageArtifacts()["landmarks"].GetPointer());
            m2::LandMarkEvaluation::ImageToPointSet(landmarkImage, warpedPoints);
			warpedPoints->SetGeometry(s->GetGeometry());
			m2::LandMarkEvaluation::EvaluatePointSet(warpedPoints, 7);

            auto node5 = mitk::DataNode::New();
            node5->SetName("Stack_Landmarks_Points" + std::to_string(i));
            node5->SetData(warpedPoints);
			node5->SetFloatProperty("point 2D size", 0.025);
            this->GetDataStorage()->Add(node5, node);
          }*/
        }
        ++i;

        // auto node2 = mitk::DataNode::New();
        // node2->SetName("Stack_Index_" + std::to_string(i++));
        // node2->SetData(s->GetIndexImage());
        // this->GetDataStorage()->Add(node2);
      }

      // future release all connections and destroy itself
      future->disconnect();
    });

  future->setFuture(QtConcurrent::run(worker));
  // m_Controls.btnCancel->setEnabled(true);
}

void m2Reconstruction3D::OnCancelRegistration()
{
  // future->cancel();
  // m_Controls.btnCancel->setEnabled(false);
}

void m2Reconstruction3D::OnUpdateList()
{
  auto all = this->GetDataStorage()->GetAll();
  m_list1->clear();
  m_list2->clear();
  m_referenceMap.clear();
  unsigned int i = 0;
  // iterate all objects in data storage and create a list widged item
//  unsigned id = 1;
  for (mitk::DataNode::Pointer node : *all)
  {
    if (node.IsNull())
      continue;
    if (auto data = dynamic_cast<m2::ImzMLMassSpecImage *>(node->GetData()))
    {
      auto res = this->GetDataStorage()->GetDerivations(node, mitk::TNodePredicateDataType<mitk::PointSet>::New());

      auto *item = new QListWidgetItem();
      item->setText((node->GetName()).c_str());
      item->setData(Qt::UserRole, QVariant::fromValue(i));

      if (res->Size() == 1)
      {
        //auto pointset = dynamic_cast<mitk::PointSet *>(res->front()->GetData());

		//data->GetImageArtifacts()["landmarks"] = data->GetIndexImage()->Clone();
        /*data->GetImageArtifacts()["landmarks"]->Initialize(
          mitk::MakeScalarPixelType<m2::IndexImagePixelType>(), 3, data->GetDimensions());
        data->GetImageArtifacts()["landmarks"]->SetSpacing(data->GetGeometry()->GetSpacing());
        data->GetImageArtifacts()["landmarks"]->SetOrigin(data->GetGeometry()->GetOrigin());
*/
		/*auto landmarkImage = dynamic_cast<mitk::Image *>(data->GetImageArtifacts()["landmarks"].GetPointer());
        m2::LandMarkEvaluation::PointSetToImage(pointset, landmarkImage);
		auto ptsNode = mitk::DataNode::New();
		ptsNode->SetData(data->GetImageArtifacts()["landmarks"]);
		ptsNode->SetName(node->GetName() + "_landmarks");
		GetDataStorage()->Add(ptsNode, node);*/
      }

      DataTuple tuple;
      tuple.image = data;
      tuple.mask = data->GetMaskImage();

      m_referenceMap[i] = tuple;

      ++i;
      m_list1->addItem(item);
    }
  }
}
