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

#include "m2DataView.h"

#include "Qm2OpenSlideImageIOHelperDialog.h"
#include <QColorDialog>
#include <QComboBox>
#include <QInputDialog>
#include <QmitkRenderWindow.h>
#include <QMainWindow>
#include <QStatusBar>
#include <QtConcurrent>
#include <boost/format.hpp>
#include <itkRescaleIntensityImageFilter.h>
#include <itksys/SystemTools.hxx>
#include <m2SpectrumContainerImage.h>
#include <m2ImzMLSpectrumImage.h>
#include <m2IntervalVector.h>
#include <m2SpectrumImage.h>
#include <m2SpectrumImageDataInteractor.h>
#include <m2Process.hpp>
#include <m2SpectrumImageStack.h>
#include <m2ShiftMapImageFilter.h>
#include <m2SubdivideImage2DFilter.h>
#include <m2UIUtils.h>
#include <mitkColorProperty.h>
#include <mitkCoreServices.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>
#include <mitkImageCast.h>
#include <mitkIOUtil.h>
#include <mitkLayoutAnnotationRenderer.h>
#include <mitkLookupTableProperty.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateFunction.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateOr.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateProperty.h>
#include <mitkPointSet.h>
#include <mitkPointSetDataInteractor.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkImageVtkMapper2D.h>
#include <mitkImageAccessByItk.h>
#include <m2ImzMLImageIO.h>
#include <regex>
// #include <Qm2AssociatedFilesDialog.h>

const std::string m2DataView::VIEW_ID = "org.mitk.views.m2.data";

void m2DataView::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  m_Parent = parent;

  InitBaselineCorrectionControls();
  InitNormalizationControls();
  InitRangePoolingControls();
  InitSmoothingControls();
  InitIntensityTransformationControls();
  InitImageNormalizationControls();
  InitImageSmoothingControls();
  InitMIRControls();
  m_Controls.groupBoxMIR->setVisible(true); // hide MIR controls until we have a better understanding of the use case and the necessary controls

  auto serviceRef = m2::UIUtils::Instance();
  connect(serviceRef, SIGNAL(UpdateImage(qreal, qreal)), this, SLOT(OnGenerateImageData(qreal, qreal)));

  connect(m_Controls.btnCreateImage,
          &QAbstractButton::clicked,
          this,
          [&] { OnGenerateImageData(m_Controls.spnBxMz->value(), FROM_GUI); });

  // step through
  // signals are triggered by key strokes (arrows) from spectrum view (Spectrum.ccp)
  auto UIUtilsObject = m2::UIUtils::Instance();
  connect(UIUtilsObject, SIGNAL(PreviousImage()), this, SLOT(OnCreatePrevImage()));
  connect(UIUtilsObject, SIGNAL(NextImage()), this, SLOT(OnCreateNextImage()));
  connect(UIUtilsObject, SIGNAL(PreviousPeakImage()), this, SLOT(OnCreatePrevPeakImage()));
  connect(UIUtilsObject, SIGNAL(NextPeakImage()), this, SLOT(OnCreateNextPeakImage()));
  connect(UIUtilsObject, SIGNAL(IncreaseTolerance()), this, SLOT(OnIncreaseTolerance()));
  connect(UIUtilsObject, SIGNAL(DecreaseTolerance()), this, SLOT(OnDecreaseTolerance()));

  connect(m_Controls.btnCreateShiftMap, SIGNAL(clicked()), this, SLOT(OnCreateShiftMap()));


  // Position list/history
  // connect(m_Controls.listWidgetPositions, &QTableWidget::cellDoubleClicked, this, [this](int row, int){
  //   auto text = m_Controls.listWidgetPositions->item(row, 0)->text();
  //   if (text == "Empty")
  //     return;
  //   if (row == 0){
  //     m_Controls.listWidgetPositions->insertRow(0);
  //     m_Controls.listWidgetPositions->setItem(0, 0, new QTableWidgetItem("Empty")); 
  //   }
  //   else
  //   {
  //     auto x = text.split(" ")[1].toDouble();
  //     auto tol = text.split(" ")[3].toDouble();
  //     this->OnGenerateImageData(x, tol);
  //   }
  // });
  

  // Settings (show hlper objects)
  using namespace mitk;

  const auto toggleByType = [&](bool isChecked, m2::NormalizationStrategyType type)
  {
    const auto a = TNodePredicateDataType<m2::SpectrumImageStack>::New();
    const auto b = TNodePredicateDataType<m2::ImzMLSpectrumImage>::New();
    const auto c = TNodePredicateDataType<m2::SpectrumContainerImage>::New();
    const auto predicate = NodePredicateOr::New(a, b, c);
    const auto nodes = this->GetDataStorage()->GetSubset(predicate);
    for (const auto &node : *nodes)
    {
      if (auto image = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
      {
        std::string name = node->GetName() + "." + m2::to_string(type);

        const auto derivations = this->GetDataStorage()->GetDerivations(node);
        for (const auto &dNode : *derivations)
        {
          if (dNode->GetName().find(name) != std::string::npos)
          {
            dNode->SetVisibility(isChecked);

            if (!isChecked)// to keep the data manger clean hide the DataNode by tagging it as helper object
              dNode->SetBoolProperty("helper object", true);
            else
            {
              if (!image->GetNormalizationImageStatus(type))
                 image->InitializeNormalizationImage(type);
              // if it should be visible in the data storage remove the helper object property
              dNode->RemoveProperty("helper object");

              // update level window
              mitk::LevelWindow lw;
              dNode->GetLevelWindow(lw);
              lw.SetAuto(image->GetNormalizationImage(type));
              const auto max = lw.GetRangeMax();
              lw.SetWindowBounds(0, max);
              dNode->SetLevelWindow(lw);
            }
            this->RequestRenderWindowUpdate();
          }
        }
      }
    }
  };

  const auto toggleByName = [&](bool isChecked, const char *postfix, bool updateHelperProperty = true)
  {
    const auto a = TNodePredicateDataType<m2::SpectrumImageStack>::New();
    const auto b = TNodePredicateDataType<m2::ImzMLSpectrumImage>::New();
    const auto c = TNodePredicateDataType<m2::SpectrumContainerImage>::New();
    const auto predicate = NodePredicateOr::New(a, b, c);
    const auto nodes = this->GetDataStorage()->GetSubset(predicate);
    for (const auto &node : *nodes)
    {
      const auto derivations = this->GetDataStorage()->GetDerivations(node);
      for (const auto &dNode : *derivations)
      {
        if (dNode->GetName().find(postfix) != std::string::npos)
        {
          dNode->SetVisibility(isChecked);
          if (updateHelperProperty)
          {
            if (!isChecked)
              dNode->SetBoolProperty("helper object", true);
            else
              dNode->RemoveProperty("helper object");
          }
          this->RequestRenderWindowUpdate();
        }
      }
    }
  };

  // connect checkboxes to handle visibility of helper objects in the DataManager
  int i = 1;
  for (m2::NormalizationStrategyType type : m2::NormalizationStrategyTypeList)
  {
    auto ckBox = new QCheckBox(("Show " + m2::to_string(type) + " normalization image").c_str(), m_Controls.settings);
    QHBoxLayout *layout = (QHBoxLayout *)(m_Controls.settings->layout());
    layout->insertWidget(layout->indexOf(m_Controls.line) + i, ckBox);
    ckBox->setObjectName(("ckBoxNormalizationImage" + m2::to_string(type)).c_str());

    connect(ckBox, &QCheckBox::toggled, this, [toggleByType, type](int v) { toggleByType(v, type); });
    ++i;
  }


  connect(m_Controls.showIndexImages,
          &QCheckBox::toggled,
          this,
          [toggleByName](bool isChecked) { toggleByName(isChecked, ".index"); });
  connect(m_Controls.showMaskImages,
          &QCheckBox::toggled,
          this,
          [toggleByName](bool isChecked) { toggleByName(isChecked, ".mask"); });
  
  connect(m2::UIUtils::Instance(),
          &m2::UIUtils::RequestTolerance,
          this,
          [this](float x, float &tol)
          {
            tol = m_Controls.spnBxTol->value();
            if (m_Controls.rbtnTolPPM->isChecked())
              tol = m2::PartPerMillionToFactor(tol) * x;
          });

  // Imaging controls
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();

  auto UpdateNodeSettings = [this](){
    auto nodesToProcess = m2::UIUtils::AllNodes(GetDataStorage());
    ApplySettingsToNodes(nodesToProcess);
  };

  connect(m_Controls.spnBxTol,
          qOverload<double>(&QDoubleSpinBox::valueChanged),
          this,
          [this, preferences, UpdateNodeSettings](int)
          {
            auto value = m_Controls.spnBxTol->value();
            preferences->PutFloat("m2aia.signal.Tolerance", value);
            UpdateNodeSettings();
          });

  connect(m_Controls.CBNormalization,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          [this, preferences, UpdateNodeSettings](int)
          {
            auto value = m_Controls.CBNormalization->currentData().toUInt();
            preferences->PutInt("m2aia.signal.NormalizationStrategy", value);
            UpdateCentroidNormalizationWarning();
          });

  connect(m_Controls.CBTransformation,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          [this, preferences, UpdateNodeSettings](int)
          {
            auto value = m_Controls.CBTransformation->currentData().toUInt();
            preferences->PutInt("m2aia.signal.IntensityTransformationStrategy", value);
            UpdateNodeSettings();
          });

  connect(m_Controls.CBImagingStrategy,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          [this, preferences, UpdateNodeSettings](int)
          {
            auto value = m_Controls.CBImagingStrategy->currentData().toUInt();
            preferences->PutInt("m2aia.signal.RangePoolingStrategy", value);
            UpdateNodeSettings();
          });

  connect(m_Controls.CBSmoothing,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          [this, preferences, UpdateNodeSettings](int)
          {
            auto value = m_Controls.CBSmoothing->currentData().toUInt();
            preferences->PutInt("m2aia.signal.SmoothingStrategy", value);
            UpdateNodeSettings();
          });

  connect(m_Controls.CBImageNormalization,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          [this, preferences, UpdateNodeSettings](int)
          {
            auto value = m_Controls.CBImageNormalization->currentData().toUInt();
            preferences->PutInt("m2aia.signal.ImageNormalizationStrategy", value);
            UpdateNodeSettings();
          });

  connect(m_Controls.CBImageSmoothing,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          [this, preferences, UpdateNodeSettings](int)
          {
            auto value = m_Controls.CBImageSmoothing->currentData().toUInt();
            preferences->PutInt("m2aia.signal.ImageSmoothingStrategy", value);
            UpdateNodeSettings();
          });


          

  // default values
  m_Controls.spnBxTol->setValue(preferences->GetFloat("m2aia.signal.Tolerance", 75));
  
  m_Controls.CBNormalization->setCurrentIndex(
    preferences->GetInt("m2aia.signal.NormalizationStrategy", to_underlying(m2::NormalizationStrategyType::None)));
  
  m_Controls.CBTransformation->setCurrentIndex(preferences->GetInt("m2aia.signal.IntensityTransformationStrategy",
                                                                   to_underlying(m2::NormalizationStrategyType::None)));
  m_Controls.CBSmoothing->setCurrentIndex(
    preferences->GetInt("m2aia.signal.SmoothingStrategy", to_underlying(m2::NormalizationStrategyType::None)));
  
  m_Controls.CBImagingStrategy->setCurrentIndex(
    preferences->GetInt("m2aia.signal.RangePoolingStrategy", to_underlying(m2::NormalizationStrategyType::Mean)));
  
  m_Controls.CBImageNormalization->setCurrentIndex(
    preferences->GetInt("m2aia.signal.ImageNormalizationStrategy", to_underlying(m2::ImageNormalizationStrategyType::None)));
  
  m_Controls.CBImageNormalization->setCurrentIndex(
    preferences->GetInt("m2aia.signal.ImageSmoothingStrategy", to_underlying(m2::ImageSmoothingStrategyType::None)));

  // Make sure, that data nodes added before this view
  // is initialized are handled correctly!!
  auto nodes = this->GetDataStorage()->GetAll();
  unsigned int count = 0;
  for (auto n : *nodes)
  {
    if (auto I = dynamic_cast<m2::ImzMLSpectrumImage *>(n->GetData()))
    {
      if (I->GetImageAccessInitialized())
      {
        n->SetVisibility(false);
      }
      else
      {
        count++;
        n->SetStringProperty("UpdateRequired", "uninitialized imzML");
      }
    }
  }

  if (count)
  {
    auto future = QtConcurrent::run([parent, this]() { QThread::currentThread()->sleep(1); });

    auto watcher = std::make_shared<QFutureWatcher<void>>();
    watcher->setFuture(future);
    connect(watcher.get(),
            &QFutureWatcher<void>::finished,
            [watcher, this]() mutable
            {
              auto nodes = this->GetDataStorage()->GetAll();

              for (auto n : *nodes)
              {
                if (n->GetProperty("UpdateRequired"))
                {
                  auto I = dynamic_cast<m2::SpectrumImage *>(n->GetData());
                  this->ApplySettingsToImage(I);
                  this->GetDataStorage()->Remove(n);
                  this->GetDataStorage()->Add(n);
                }
              }
              mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
              this->RequestRenderWindowUpdate();
              watcher->disconnect();
              watcher.reset();
            });
  }


  connect(&m_ResetPreventDataStorageOverload, &QFutureWatcher<void>::finished, this, [this]() {
    mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());
    this->RequestRenderWindowUpdate();
  });
}

void m2DataView::InitToleranceControls()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto defaultValue = preferences->GetFloat("m2aia.signal.Tolerance", 75.0);
  m_Controls.spnBxTol->setValue(defaultValue);
}

void m2DataView::InitNormalizationControls()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto defaultValue =
    preferences->GetInt("m2aia.signal.NormalizationStrategy", to_underlying(m2::NormalizationStrategyType::None));
  auto cb = Controls()->CBNormalization;
  for (unsigned int i = 0; i < m2::NormalizationStrategyTypeNames.size(); ++i)
    cb->addItem(m2::NormalizationStrategyTypeNames[i].c_str(), {i});
  cb->setCurrentIndex(defaultValue);
}

m2::NormalizationStrategyType m2DataView::GuiToNormalizationStrategyType()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto value =
    preferences->GetInt("m2aia.signal.NormalizationStrategy", to_underlying(m2::NormalizationStrategyType::None));
  return static_cast<m2::NormalizationStrategyType>(value);
}

void m2DataView::InitImageNormalizationControls(){
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto defaultValue =
    preferences->GetInt("m2aia.signal.ImageNormalizationStrategy", to_underlying(m2::ImageNormalizationStrategyType::None));
  auto cb = Controls()->CBImageNormalization;
  for (unsigned int i = 0; i < m2::ImageNormalizationStrategyTypeNames.size(); ++i)
    cb->addItem(m2::ImageNormalizationStrategyTypeNames[i].c_str(), {i});
  cb->setCurrentIndex(defaultValue);
}


m2::ImageNormalizationStrategyType m2DataView::GuiToImageNormalizationStrategyType()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto value =
    preferences->GetInt("m2aia.signal.ImageNormalizationStrategy", to_underlying(m2::ImageNormalizationStrategyType::None));
  return static_cast<m2::ImageNormalizationStrategyType>(value);
}

m2::ImageSmoothingStrategyType m2DataView::GuiToImageSmoothingStrategyType()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto value =
    preferences->GetInt("m2aia.signal.ImageSmoothingStrategy", to_underlying(m2::ImageSmoothingStrategyType::None));
  return static_cast<m2::ImageSmoothingStrategyType>(value);
}

void m2DataView::InitImageSmoothingControls()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto defaultValue =
    preferences->GetInt("m2aia.signal.ImageSmoothingStrategy", to_underlying(m2::ImageSmoothingStrategyType::None));
  auto cb = Controls()->CBImageSmoothing;
  for (unsigned int i = 0; i < m2::ImageSmoothingStrategyTypeNames.size(); ++i)
    cb->addItem(m2::ImageSmoothingStrategyTypeNames[i].c_str(), {i});
  cb->setCurrentIndex(defaultValue);
}


void m2DataView::InitMIRControls()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();

  // ── Vector normalisation check box ────────────────────────────────────────
  {
    bool defaultVal = preferences->GetBool("m2aia.mir.VectorNormalization", false);
    m_Controls.ckMIRVectorNormalization->setChecked(defaultVal);
    connect(m_Controls.ckMIRVectorNormalization, &QCheckBox::toggled, this,
            [this, preferences](bool checked)
            {
              preferences->PutBool("m2aia.mir.VectorNormalization", checked);
            });
  }

  // ── Derivative combo box ──────────────────────────────────────────────────
  {
    auto defaultVal = preferences->GetInt(
      "m2aia.mir.DerivativeStrategy",
      to_underlying(m2::MIRDerivativeType::None));
    auto *cb = m_Controls.CBMIRDerivative;
    for (unsigned int i = 0; i < m2::MIRDerivativeTypeNames.size(); ++i)
      cb->addItem(m2::MIRDerivativeTypeNames[i].c_str(), {i});
    cb->setCurrentIndex(defaultVal);
    connect(cb, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this, preferences](int)
            {
              preferences->PutInt("m2aia.mir.DerivativeStrategy",
                                  m_Controls.CBMIRDerivative->currentData().toInt());
            });
  }
}

m2::MIRDerivativeType m2DataView::GuiToMIRDerivativeType()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto value = preferences->GetInt(
    "m2aia.mir.DerivativeStrategy",
    to_underlying(m2::MIRDerivativeType::None));
  return static_cast<m2::MIRDerivativeType>(value);
}

void m2DataView::InitIntensityTransformationControls()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto defaultValue = preferences->GetInt("m2aia.signal.IntensityTransformationStrategy",
                                          to_underlying(m2::IntensityTransformationType::None));
  // Combo Box
  auto cb = Controls()->CBTransformation;
  for (unsigned int i = 0; i < m2::IntensityTransformationTypeNames.size(); ++i)
    cb->addItem(m2::IntensityTransformationTypeNames[i].c_str(), {i});

  cb->setCurrentIndex(defaultValue);
}

m2::IntensityTransformationType m2DataView::GuiToIntensityTransformationStrategyType()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto value = preferences->GetInt("m2aia.signal.IntensityTransformationStrategy",
                                   to_underlying(m2::IntensityTransformationType::None));
  return static_cast<m2::IntensityTransformationType>(value);
}

void m2DataView::InitRangePoolingControls()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto defaultValue =
    preferences->GetInt("m2aia.signal.RangePoolingStrategy", to_underlying(m2::RangePoolingStrategyType::Mean));
  auto cb = Controls()->CBImagingStrategy;
  for (unsigned int i = 0; i < m2::RangePoolingStrategyTypeNames.size(); ++i)
    cb->addItem(m2::RangePoolingStrategyTypeNames[i].c_str(), {i}); // add i as data

  cb->setCurrentIndex(defaultValue);
}

m2::RangePoolingStrategyType m2DataView::GuiToRangePoolingStrategyType()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto value =
    preferences->GetInt("m2aia.signal.RangePoolingStrategy", to_underlying(m2::RangePoolingStrategyType::None));
  return static_cast<m2::RangePoolingStrategyType>(value);
}

void m2DataView::InitSmoothingControls()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto defaultValue = preferences->GetInt("m2aia.signal.SmoothingStrategy", to_underlying(m2::SmoothingType::None));
  auto cb = Controls()->CBSmoothing;
  for (unsigned int i = 0; i < m2::SmoothingTypeNames.size(); ++i)
    cb->addItem(m2::SmoothingTypeNames[i].c_str(), {i});
  cb->setCurrentIndex(defaultValue);

  // Spin Box
  m_Controls.spnBxSmoothing->setValue(preferences->GetInt("m2aia.signal.SmoothingValue", 2));
  connect(m_Controls.spnBxSmoothing,
          qOverload<int>(&QSpinBox::valueChanged),
          this,
          [this, preferences](int)
          { preferences->PutInt("m2aia.signal.SmoothingValue", m_Controls.spnBxSmoothing->value()); });
}

m2::SmoothingType m2DataView::GuiToSmoothingStrategyType()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto value = preferences->GetInt("m2aia.signal.SmoothingStrategy", to_underlying(m2::SmoothingType::None));
  return static_cast<m2::SmoothingType>(value);
}

void m2DataView::InitBaselineCorrectionControls()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto defaultValue =
    preferences->GetInt("m2aia.signal.BaselineCorrectionStrategy", to_underlying(m2::BaselineCorrectionType::None));
  auto cb = Controls()->CBBaselineCorrection;
  for (unsigned int i = 0; i < m2::BaselineCorrectionTypeNames.size(); ++i)
    cb->addItem(m2::BaselineCorrectionTypeNames[i].c_str(), {i});
  cb->setCurrentIndex(defaultValue);

  connect(m_Controls.CBBaselineCorrection,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          [this, preferences](int)
          {
            auto value = m_Controls.CBBaselineCorrection->currentData().toUInt();
            preferences->PutInt("m2aia.signal.BaselineCorrectionStrategy", value);
          });

  // Spin Box
  m_Controls.spnBxBaseline->setValue(preferences->GetInt("m2aia.signal.BaselineCorrectionValue", 50));
  connect(m_Controls.spnBxBaseline,
          qOverload<int>(&QSpinBox::valueChanged),
          this,
          [this, preferences](int)
          { preferences->PutInt("m2aia.signal.BaselineCorrectionValue", m_Controls.spnBxBaseline->value()); });
}

m2::BaselineCorrectionType m2DataView::GuiToBaselineCorrectionStrategyType()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  auto *preferences = preferencesService->GetSystemPreferences();
  auto value =
    preferences->GetInt("m2aia.signal.BaselineCorrectionStrategy", to_underlying(m2::BaselineCorrectionType::None));
  return static_cast<m2::BaselineCorrectionType>(value);
}

void m2DataView::OnDecreaseTolerance()
{
  m_Controls.spnBxTol->setValue(m_Controls.spnBxTol->value() * 0.9);
  OnGenerateImageData(m_Controls.spnBxMz->value(), FROM_GUI);
}

void m2DataView::OnIncreaseTolerance()
{
  m_Controls.spnBxTol->setValue(m_Controls.spnBxTol->value() * 1.1);
  OnGenerateImageData(m_Controls.spnBxMz->value(), FROM_GUI);
}

void m2DataView::OnCreateNextImage()
{
  auto center = m_Controls.spnBxMz->value();
  auto offset = m_Controls.spnBxTol->value();
  if (m_Controls.rbtnTolPPM->isChecked())
    offset = m2::PartPerMillionToFactor(offset) * .5 * center;

  double candidate = center + offset;

  // Collect per-node [xMin, xMax] ranges from all visible spectrum nodes.
  auto allNodes = m2::UIUtils::AllNodes(GetDataStorage());
  bool inRange = false;
  double nearestXMin = std::numeric_limits<double>::max(); // smallest xMin > center
  for (const auto &n : *allNodes)
  {
    if (auto img = dynamic_cast<m2::SpectrumImage *>(n->GetData()))
    {
      const double lo = img->GetPropertyValue<double>("m2aia.xs.min");
      const double hi = img->GetPropertyValue<double>("m2aia.xs.max");
      if (candidate >= lo && candidate <= hi)
      {
        inRange = true;
        break;
      }
      // Track the nearest range that starts above center (for gap-jumping).
      if (lo > center && lo < nearestXMin)
        nearestXMin = lo;
    }
  }

  // If the candidate lands in a gap, jump to the start of the next range instead.
  if (!inRange && nearestXMin != std::numeric_limits<double>::max())
    candidate = nearestXMin;

  this->OnGenerateImageData(candidate, FROM_GUI);
}

void m2DataView::OnCreatePrevImage()
{
  auto center = m_Controls.spnBxMz->value();
  auto offset = m_Controls.spnBxTol->value();
  if (m_Controls.rbtnTolPPM->isChecked())
    offset = m2::PartPerMillionToFactor(offset) * .5 * center;

  double candidate = center - offset;

  // Collect per-node [xMin, xMax] ranges from all visible spectrum nodes.
  auto allNodes = m2::UIUtils::AllNodes(GetDataStorage());
  bool inRange = false;
  double nearestXMax = std::numeric_limits<double>::lowest(); // largest xMax < center
  for (const auto &n : *allNodes)
  {
    if (auto img = dynamic_cast<m2::SpectrumImage *>(n->GetData()))
    {
      const double lo = img->GetPropertyValue<double>("m2aia.xs.min");
      const double hi = img->GetPropertyValue<double>("m2aia.xs.max");
      if (candidate >= lo && candidate <= hi)
      {
        inRange = true;
        break;
      }
      // Track the nearest range that ends below center (for gap-jumping).
      if (hi < center && hi > nearestXMax)
        nearestXMax = hi;
    }
  }

  // If the candidate lands in a gap, jump to the end of the previous range instead.
  if (!inRange && nearestXMax != std::numeric_limits<double>::lowest())
    candidate = nearestXMax;

  this->OnGenerateImageData(candidate, FROM_GUI);
}

void m2DataView::OnCreateNextPeakImage()
{
  auto predicate = mitk::TNodePredicateDataType<m2::IntervalVector>::New();
  auto processableNodes = GetDataStorage()->GetSubset(predicate)->CastToSTLConstContainer();

  auto center = m_Controls.spnBxMz->value();
  auto tolerance = m_Controls.spnBxTol->value();
  if (m_Controls.rbtnTolPPM->isChecked())
    tolerance = m2::PartPerMillionToFactor(tolerance) * .5 * center;

  // Helper: collect the nearest peak strictly beyond 'threshold' across all centroid vectors.
  const auto collectNext = [&processableNodes](double threshold) -> std::vector<m2::Interval>
  {
    std::vector<m2::Interval> result;
    for (auto node : processableNodes)
    {
      if (auto iv = dynamic_cast<m2::IntervalVector *>(node->GetData()))
      {
        if (!(to_underlying(iv->GetType()) & to_underlying(m2::SpectrumFormat::Centroid)))
          continue;
        const auto &intervals = iv->GetIntervals();
        auto it = std::min_element(intervals.begin(), intervals.end(),
          [threshold](const m2::Interval &a, const m2::Interval &b)
          {
            if (a.x.mean() <= threshold) return false;
            if (b.x.mean() <= threshold) return true;
            return a.x.mean() < b.x.mean();
          });
        if (it != intervals.end() && it->x.mean() > threshold)
          result.push_back(*it);
      }
    }
    return result;
  };

  // First try: skip the full tolerance window (normal behaviour).
  auto candidates = collectNext(center + tolerance);

  // Fallback: if nothing found beyond center+tolerance, search strictly > center
  // so we can cross a gap that is smaller than the tolerance.
  if (candidates.empty())
    candidates = collectNext(center);

  auto nearestElement = std::min_element(candidates.begin(), candidates.end(),
    [](const m2::Interval &a, const m2::Interval &b){ return a.x.mean() < b.x.mean(); });

  if (nearestElement == candidates.end())
    return;

  this->OnGenerateImageData(nearestElement->x.mean(), FROM_GUI);
}

void m2DataView::OnCreatePrevPeakImage()
{
  auto predicate = mitk::TNodePredicateDataType<m2::IntervalVector>::New();
  auto processableNodes = GetDataStorage()->GetSubset(predicate)->CastToSTLConstContainer();

  auto center = m_Controls.spnBxMz->value();
  auto tolerance = m_Controls.spnBxTol->value();
  if (m_Controls.rbtnTolPPM->isChecked())
    tolerance = m2::PartPerMillionToFactor(tolerance) * .5 * center;

  // Helper: collect the nearest peak strictly before 'threshold' across all centroid vectors.
  const auto collectPrev = [&processableNodes](double threshold) -> std::vector<m2::Interval>
  {
    std::vector<m2::Interval> result;
    for (auto node : processableNodes)
    {
      if (auto iv = dynamic_cast<m2::IntervalVector *>(node->GetData()))
      {
        if (!(to_underlying(iv->GetType()) & to_underlying(m2::SpectrumFormat::Centroid)))
          continue;
        const auto &intervals = iv->GetIntervals();
        auto it = std::min_element(intervals.begin(), intervals.end(),
          [threshold](const m2::Interval &a, const m2::Interval &b)
          {
            if (a.x.mean() >= threshold) return false;
            if (b.x.mean() >= threshold) return true;
            return a.x.mean() > b.x.mean();
          });
        if (it != intervals.end() && it->x.mean() < threshold)
          result.push_back(*it);
      }
    }
    return result;
  };

  // First try: skip the full tolerance window (normal behaviour).
  auto candidates = collectPrev(center - tolerance);

  // Fallback: if nothing found before center-tolerance, search strictly < center
  // so we can cross a gap that is smaller than the tolerance.
  if (candidates.empty())
    candidates = collectPrev(center);

  auto nearestElement = std::min_element(candidates.begin(), candidates.end(),
    [](const m2::Interval &a, const m2::Interval &b){ return a.x.mean() > b.x.mean(); });

  if (nearestElement == candidates.end())
    return;

  this->OnGenerateImageData(nearestElement->x.mean(), FROM_GUI);
}

void m2DataView::ApplySettingsToNodes(m2::UIUtils::NodesVectorType::Pointer v)
{
  for (auto dataNode : *v)
  {
    if (auto data = dynamic_cast<m2::SpectrumImage *>(dataNode->GetData()))
      ApplySettingsToImage(data);
  }
}

void m2DataView::ApplySettingsToImage(m2::SpectrumImage *data)
{
  if (data)
  {
    data->SetNormalizationStrategy(GuiToNormalizationStrategyType());
    data->SetBaselineCorrectionStrategy(GuiToBaselineCorrectionStrategyType());
    data->SetSmoothingStrategy(GuiToSmoothingStrategyType());
    data->SetRangePoolingStrategy(GuiToRangePoolingStrategyType());
    data->SetIntensityTransformationStrategy(GuiToIntensityTransformationStrategyType());

    data->SetImageNormalizationStrategy(GuiToImageNormalizationStrategyType());
    data->SetImageSmoothingStrategy(GuiToImageSmoothingStrategyType());

    data->SetSmoothingHalfWindowSize(m_Controls.spnBxSmoothing->value());
    data->SetBaseLineCorrectionHalfWindowSize(m_Controls.spnBxBaseline->value());
    data->SetUseToleranceInPPM(m_Controls.rbtnTolPPM->isChecked());

    // MIR-specific settings – only applied when the data is a SpectrumContainerImage
    // in MIR modality.  Other image types ignore these controls.
    if (auto mirData = dynamic_cast<m2::SpectrumContainerImage *>(data))
    {
      mirData->SetMIRVectorNormalization(m_Controls.ckMIRVectorNormalization->isChecked());
      mirData->SetMIRDerivativeStrategy(GuiToMIRDerivativeType());
    }

    // Initialize normalization image
    auto type = data->GetNormalizationStrategy();
    if(!data->GetNormalizationImageStatus(type))
      data->InitializeNormalizationImage(type);
  }
}

void m2DataView::OnGenerateImageData(mitk::DataNode::Pointer node,
                                 qreal xRangeCenter,
                                 qreal xRangeTol,
                                 bool emitRangeChanged)
{
  // tol < 0 indicates "use gui tol"
  if (xRangeTol < 0)
  {
    xRangeTol = Controls()->spnBxTol->value();
    bool isPpm = Controls()->rbtnTolPPM->isChecked();
    xRangeTol = isPpm ? m2::PartPerMillionToFactor(xRangeTol) * xRangeCenter : xRangeTol;
  }

  if (emitRangeChanged)
    emit m2::UIUtils::Instance()->RangeChanged(xRangeCenter, xRangeTol);

  this->m_Controls.spnBxMz->setValue(xRangeCenter);

  

  if (m2::SpectrumImage::Pointer data = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
  {
    const auto xMin = data->GetPropertyValue<double>("m2aia.xs.min");
    const auto xMax = data->GetPropertyValue<double>("m2aia.xs.max");
    if (xRangeCenter > xMax || xRangeCenter < xMin)
      return;

    ApplySettingsToImage(data);
    if (!data->IsInitialized())
      mitkThrow() << "Trying to grab an ion image but data access was not initialized properly!";

    mitk::Image::Pointer maskImage = data->GetMultilabelSegmentation()->GetGroupImage(0);

    // The smartpointer will stay alive until all captured copies are relesed. Additional
    // all connected signals must be disconnected to make sure that the future is not kept
    // alive after the 'finished-callback' is processed.
    auto future = std::make_shared<QFutureWatcher<mitk::Image::Pointer>>();

    //*************** Worker Finished Callback ******************//
    // capture holds a copy of the smartpointer, so it will stay alive. Make the lambda mutable to
    // allow the manipulation of captured varaibles that are copied by '='.
    const auto futureFinished = [future, node, this]() mutable
    {
      auto image = future->result();
      UpdateLevelWindow(node);
      // UpdateSpectrumImageTable(node);
      node->SetProperty("m2aia.xs.selection.center", image->GetProperty("m2aia.xs.selection.center"));
      node->SetProperty("m2aia.xs.selection.tolerance", image->GetProperty("m2aia.xs.selection.tolerance"));
      this->RequestRenderWindowUpdate();
      future->disconnect();
    };

    //*************** Worker Block******************//
    const auto futureWorker = [xRangeCenter, xRangeTol, data, maskImage, this]()
    {
      // m2::Timer t("Create image @[" + std::to_string(xRangeCenter) + " " + std::to_string(xRangeTol) + "]");
      if (m_InitializeNewNode)
      {
        auto geom = data->GetGeometry()->Clone();
        auto image = mitk::Image::New();
        image->Initialize(mitk::MakeScalarPixelType<m2::DisplayImagePixelType>(), *geom);
        data->GetImage(xRangeCenter, xRangeTol, maskImage, image);
        return image;
      }
      else
      {
        data->GetImage(xRangeCenter, xRangeTol, maskImage, data);
        mitk::Image::Pointer imagePtr = data.GetPointer();
        return imagePtr;
      }
    };

    //*************** Start Worker ******************//

    connect(future.get(), &QFutureWatcher<mitk::Image::Pointer>::finished, future.get(), futureFinished);
    future->setFuture(QtConcurrent::run(&m_pool, futureWorker));
  }
}


void m2DataView::OnCreateShiftMap()
{
// get the selection
  auto nodesToProcess = m2::UIUtils::AllNodes(GetDataStorage());

  if (nodesToProcess->empty())
    return;

  // process all nodes
  for (mitk::DataNode::Pointer dataNode : *nodesToProcess)
    if (m2::ImzMLSpectrumImage::Pointer data = dynamic_cast<m2::ImzMLSpectrumImage *>(dataNode->GetData()))
    {

      auto shiftMapFilter = m2::ShiftMapImageFilter::New();

      shiftMapFilter->SetInput(data);
      shiftMapFilter->GenerateData();
      
      auto node = mitk::DataNode::New();
      node->SetData(shiftMapFilter->GetOutput(1));
      node->SetName(dataNode->GetName() + ".shift");
      GetDataStorage()->Add(node, dataNode);

    }
}


void m2DataView::OnGenerateImageData(qreal xRangeCenter, qreal xRangeTol)
{
  // get the selection
  auto nodesToProcess = m2::UIUtils::AllNodes(GetDataStorage());


  if (nodesToProcess->empty())
    return;

  if (xRangeTol < 0){
    xRangeTol = Controls()->spnBxTol->value();
    bool isPpm = Controls()->rbtnTolPPM->isChecked();
    xRangeTol = isPpm ? m2::PartPerMillionToFactor(xRangeTol) * xRangeCenter : xRangeTol;
  }

  
  emit m2::UIUtils::Instance()->RangeChanged(xRangeCenter, xRangeTol);
  this->m_Controls.spnBxMz->setValue(xRangeCenter);
  auto flag = std::make_shared<std::atomic<bool>>(0);

  QString labelText = str(boost::format("%.4f +/- %.2f Da") % xRangeCenter % xRangeTol).c_str();

  if (nodesToProcess->size())
  {
    auto node = nodesToProcess->front();
    if (auto image = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
    {
      std::string xLabel = image->GetSpectrumType().XAxisLabel;
      labelText = "" + QString(xLabel.c_str()) + " " + labelText;
    }
  }
  
  this->UpdateTextAnnotations(labelText.toStdString());

  // set GUI settings to the selected nodes
  ApplySettingsToNodes(nodesToProcess);

  // process all nodes
  for (mitk::DataNode::Pointer dataNode : *nodesToProcess)
    if (m2::SpectrumImage::Pointer data = dynamic_cast<m2::SpectrumImage *>(dataNode->GetData()))
      OnGenerateImageData(dataNode, xRangeCenter, xRangeTol, false); // do not emit
}

void m2DataView::UpdateSpectrumImageTable(const mitk::DataNode *node)
{
  if (dynamic_cast<m2::SpectrumImage *>(node->GetData()))
  {
    // auto widgets = m_Parent->findChildren<Qm2ImageColorWidget *>("Qm2ImageColorWidget");
    // MITK_INFO << widgets.count();
    //  if(!widgets.count() || widgets.front()->visibilityCheckBox()->isChecked()){
    //    MITK_INFO << widgets.back();
    //    auto c = m_Controls.verticalLayout->count();
    //    m_Controls.verticalLayout->insertWidget(c-1-widgets.count(), new Qm2ImageColorWidget(m_Parent));

    //  }
  }
  //   auto item = m_Controls.tableWidget->item(0, 1);
  //   std::string xLabel = image->GetPropertyValue<std::string>("x_label");
  //   double center = image->GetPropertyValue<double>("m2aia.xs.selection.center");
  //   double tol = image->GetPropertyValue<double>("m2aia.xs.selection.tolerance");
  //   QString labelText = str(boost::format(xLabel + " %.2f +/- %.2f Da") % center % tol).c_str();
  //   item->setText(labelText);

  // }

  // item = m_Controls.tableWidget->item(0, 0)->setData()
}

void m2DataView::OnRenderSpectrumImages(double min, double max)
{
  Q_UNUSED(min);
  Q_UNUSED(max);
}

void m2DataView::UpdateTextAnnotations(std::string text)
{
  if (auto mainWindow = qobject_cast<QMainWindow *>(QApplication::activeWindow()))
  {
    
    if (auto statusBar = mainWindow->statusBar())
{
      statusBar->showMessage(QString::fromStdString(text));
    }
  }
}

mitk::DataNode::Pointer m2DataView::FindChildNodeRegex(mitk::DataNode::Pointer &parent, std::string regexString)
{
  auto deriv = this->GetDataStorage()->GetDerivations(parent.GetPointer(), nullptr, true);
  try
  {
    for (auto p : *deriv)
    {
      if (std::regex_match(p->GetName(), std::regex{regexString.c_str()}))
        return p;
    }
  }
  catch (std::exception &e)
  {
    MITK_WARN << e.what();
  }
  return nullptr;
}

void m2DataView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*part*/, const QList<mitk::DataNode::Pointer> &nodes)
{
  if (!nodes.empty())
  {
    auto node = nodes.front();
    if (auto image = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
    {
      QString labelText = str(boost::format("%.2f +/- %.2f Da") % image->GetCurrentX() % image->GetTolerance()).c_str();

      labelText += "\n";
      if (nodes.size() == 1)
        labelText += node->GetName().c_str();

      this->UpdateTextAnnotations(labelText.toStdString());
    }
  }
}

void m2DataView::UpdateLevelWindow(const mitk::DataNode *node)
{
  if (auto msImageBase = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
  {
    mitk::LevelWindow lw;
    node->GetLevelWindow(lw);
    lw.SetAuto(msImageBase);
    if (m_Controls.CBUseFixedLevel->isChecked())
    {
      lw.SetLevelWindow(m_Controls.spnBxLevel->value(), m_Controls.spnBxWindow->value());
    }
    const_cast<mitk::DataNode *>(node)->SetLevelWindow(lw);
  }
}

void m2DataView::NodeAdded(const mitk::DataNode *node)
{
  if (dynamic_cast<mitk::PointSet *>(node->GetData()))
  {
    PointSetNodeAdded(node);
  }
  else if (dynamic_cast<m2::OpenSlideImageIOHelperObject *>(node->GetData()))
  {
    OpenSlideImageNodeAdded(node);
  }
  else if (auto data = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
  {
    // !! Primary initialization !!
    this->ApplySettingsToImage(data);
    data->InitializeImageAccess();
    // this->RequestRenderWindowUpdate();

    SpectrumImageNodeAdded(node);
    auto xs = data->GetXAxis();

    if(m_Controls.spnBxMz->value() == 0)
      this->OnGenerateImageData(xs[xs.size()/2], FROM_GUI);
    else
      this->OnGenerateImageData(m_Controls.spnBxMz->value(), FROM_GUI);
  }
}

void m2DataView::PointSetNodeAdded(const mitk::DataNode *node)
{
  // Find a reference image to derive the physical spacing for point sizing.
  // Prefer m2::SpectrumImage; fall back to any mitk::Image in the storage.
  double spacingX = 1.0;

  auto spectrumPredicate = mitk::NodePredicateFunction::New(
    [](const mitk::DataNode *n) -> bool {
      return dynamic_cast<m2::SpectrumImage *>(n->GetData()) != nullptr;
    });

  auto candidates = GetDataStorage()->GetSubset(spectrumPredicate);
  if (candidates->empty())
    candidates = GetDataStorage()->GetSubset(
      mitk::TNodePredicateDataType<mitk::Image>::New());

  if (!candidates->empty())
  {
    if (auto img = dynamic_cast<mitk::Image *>(candidates->front()->GetData()))
      spacingX = img->GetGeometry()->GetSpacing()[0];
  }

  const float pointSize3D = static_cast<float>(4.0 * spacingX);
  const float pointSize2D = static_cast<float>(4.0 * spacingX);

  auto *mutableNode = const_cast<mitk::DataNode *>(node);
  mutableNode->ReplaceProperty("pointsize",      mitk::FloatProperty::New(pointSize3D)); // 3-D sphere diameter (world coords)
  mutableNode->ReplaceProperty("point 2D size",  mitk::FloatProperty::New(pointSize2D)); // 2-D pixel radius

  // Adapt the interactor hit-test radius to match the visual point size.
  // SetAccuracy takes a world-space radius; pointSize3D is a diameter, so accuracy = pointSize3D / 2.
  const float accuracy = pointSize3D / 2.0f;
  auto existingInteractor =
    dynamic_cast<mitk::PointSetDataInteractor *>(mutableNode->GetDataInteractor().GetPointer());
  if (existingInteractor)
  {
    existingInteractor->SetAccuracy(accuracy);
  }
  else
  {
    auto interactor = mitk::PointSetDataInteractor::New();
    interactor->LoadStateMachine("PointSet.xml");
    interactor->SetEventConfig("PointSetConfig.xml");
    interactor->SetAccuracy(accuracy);
    interactor->SetDataNode(mutableNode);
    mutableNode->SetDataInteractor(interactor);
  }
}

void m2DataView::OpenSlideImageNodeAdded(const mitk::DataNode *node)
{
  if (auto openSlideIOHelper = dynamic_cast<m2::OpenSlideImageIOHelperObject *>(node->GetData()))
  {
    const auto name = node->GetName();
    auto dialog = new Qm2OpenSlideImageIOHelperDialog(m_Parent);
    dialog->SetOpenSlideImageIOHelperObject(openSlideIOHelper);
    auto result = dialog->exec();
    if (result == QDialog::Accepted)
    {
      try
      {
        auto data = dialog->GetData();
        auto mx = dialog->GetMirrorX();
        auto my = dialog->GetMirrorY();
        // auto preview = dialog->GetPreviwData();
        // mitk::DataNode::Pointer parent = nullptr;
        // if (preview)
        // {
        // parent = mitk::DataNode::New();
        // parent->SetName(
        //   itksys::SystemTools::GetFilenameWithoutExtension(openSlideIOHelper->GetOpenSlideIO()->GetFileName()));
        // parent->SetData(dialog->GetPreviwData());
        // this->GetDataStorage()->Add(parent);
        // }
        if (data.size() == 1)
        {
          auto filter = m2::SubdivideImage2DFilter::New();
          filter->SetMirrorX(mx);
          filter->SetMirrorY(my);
          filter->SetInput(data.back());
          filter->SetTileHeight((unsigned int)(1) << 13);
          filter->SetTileWidth((unsigned int)(1) << 13);
          filter->Update();

          const auto nX = filter->GetNumberOfTilesInX();
          const auto nY = filter->GetNumberOfTilesInY();
          MITK_INFO << nX << " " << nY;

          unsigned int k = 0;
          for (auto I : filter->GetOutputs())
          {
            auto node = mitk::DataNode::New();
            node->SetData(I);
            node->SetName(name + "_tile_" + std::to_string(k));
            node->SetVisibility(k < 2, nullptr);
            this->GetDataStorage()->Add(node);
            ++k;
          }
        }
        else
        {
          unsigned int i = 0;
          for (auto &I : data)
          {
            auto node = mitk::DataNode::New();
            node->SetData(I);
            node->SetName(
              itksys::SystemTools::GetFilenameWithoutExtension(openSlideIOHelper->GetOpenSlideIO()->GetFileName()) +
              "_" + std::to_string(i++));
            // node->SetVisibility(false, nullptr);
            this->GetDataStorage()->Add(node);
          }
        }
      }
      catch (std::exception &e)
      {
        MITK_ERROR << "Rendering Error" << e.what();
      }
    }
    // remove IO helper object from DS
    GetDataStorage()->Remove(node);
    delete dialog;
  }
}

void m2DataView::UpdateCentroidNormalizationWarning()
{
  // Check whether any loaded centroid dataset is combined with a spectrum-based normalization
  // (TIC, Sum, Mean, Max, RMS) that computes its factor from stored peaks only.
  const auto normValue = static_cast<m2::NormalizationStrategyType>(
    m_Controls.CBNormalization->currentData().toUInt());

  const bool spectrumBasedNorm =
    normValue == m2::NormalizationStrategyType::TIC ||
    normValue == m2::NormalizationStrategyType::Sum ||
    normValue == m2::NormalizationStrategyType::Mean ||
    normValue == m2::NormalizationStrategyType::Max ||
    normValue == m2::NormalizationStrategyType::RMS;

  bool hasCentroidData = false;
  if (spectrumBasedNorm)
  {
    auto allNodes = GetDataStorage()->GetAll();
    for (auto &n : *allNodes)
    {
      if (auto img = dynamic_cast<m2::SpectrumImage *>(n->GetData()))
      {
        const auto fmt = img->GetSpectrumType().Format;
        if (any(fmt & m2::SpectrumFormat::Centroid))
        {
          hasCentroidData = true;
          break;
        }
      }
    }
  }

  if (spectrumBasedNorm && hasCentroidData)
  {
    m_Controls.lblCentroidNormalizationWarning->setText(
      "<span style='color:#cc6600;'>"
      "&#9888; Spectrum-based normalization (TIC/Sum/Mean/Max/RMS) is active on centroid data. "
      "Normalization factors are computed only from stored peaks, not the full spectrum. "
      "Consider using &lt;None&gt; or &lt;External&gt; instead. Press F1 for details."
      "</span>");
    m_Controls.lblCentroidNormalizationWarning->setVisible(true);
  }
  else
  {
    m_Controls.lblCentroidNormalizationWarning->setVisible(false);
  }
}

void m2DataView::ImzMLImageNodeAdded(const mitk::DataNode *) {}

void m2DataView::FsmImageNodeAdded(const mitk::DataNode *)
{
  //
}

void m2DataView::SpectrumImageNodeAdded(const mitk::DataNode *node)
{
  UpdateCentroidNormalizationWarning();
  

  auto nodes = GetDataStorage()->GetAll();
  // resolve name-conflicts
  if (std::any_of(nodes->begin(),
                  nodes->end(),
                  [node](auto f) { return f != node && f->GetName().compare(node->GetName()) == 0; }))
  {
    bool ok;
    QString text = QInputDialog::getText(
      this->m_Parent, tr("Name conflict"), tr("Postfix:"), QLineEdit::Normal, QDir::home().dirName(), &ok);
    const_cast<mitk::DataNode *>(node)->SetName((node->GetName() + "_" + text.toStdString()).c_str());
  }

  auto lut = mitk::LookupTable::New();
  lut->SetType(mitk::LookupTable::LookupTableType::CIVIDS_TRANSPARENT);
  const_cast<mitk::DataNode *>(node)->SetProperty("LookupTable", mitk::LookupTableProperty::New(lut));
  const_cast<mitk::DataNode *>(node)->SetBoolProperty("binary", false);

  // add nodes to data storage (default: helper objects)
  if (auto spectrumImage = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
  {
    // -------------- add data interactor --------------

    std::string inputLocation;
    node->GetStringProperty("MITK.IO.reader.inputlocation", inputLocation);

    auto interactor = m2::SpectrumImageDataInteractor::New();
    interactor->LoadStateMachine("PointSet.xml");
    interactor->SetEventConfig("PointSetConfig.xml");
    interactor->EnableInteraction(false);
    interactor->SetDataNode(const_cast<mitk::DataNode *>(node));
    interactor->SetDataStorage(GetDataStorage());
    interactor->EnableInteraction(true);
    const_cast<mitk::DataNode *>(node)->SetDataInteractor(interactor);

    m2::DefaultNodeProperties(node);

    // -------------- add Mask to datastorage --------------
    auto nodeName = itksys::SystemTools::GetFilenameWithoutLastExtension(inputLocation) + ".mask";
    auto helperNode = mitk::DataNode::New();
    helperNode->SetName(nodeName);
    helperNode->SetVisibility(m_Controls.showMaskImages->isChecked());
    helperNode->SetData(spectrumImage->GetMultilabelSegmentation());
    helperNode->SetStringProperty("m2aia.helper.image.name", "mask");

    // add hidden to DS
    helperNode->SetBoolProperty("helper object", true);
    this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
    // consideration of the check boxes
    emit m_Controls.showMaskImages->toggled(m_Controls.showMaskImages->isChecked());

    // -------------- add ShiftImage to datastorage --------------
    if(spectrumImage->GetShiftImage()){
      nodeName = itksys::SystemTools::GetFilenameWithoutLastExtension(inputLocation) + ".shift";
      helperNode = mitk::DataNode::New();
      helperNode->SetName("ShiftImage");
      helperNode->SetVisibility(false);
      helperNode->SetData(spectrumImage->GetShiftImage());
      helperNode->SetStringProperty("m2aia.helper.image.name", "ShiftImage");
      this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
    }

    auto parentImage = dynamic_cast<const mitk::Image *>(node->GetData());

    std::vector<std::string> suffixes = {".tSNE", ".PCA", ".UMAP"};
    for (const auto &suffix : suffixes)
    {
      nodeName = itksys::SystemTools::GetFilenameWithoutLastExtension(inputLocation) + suffix;
      auto fileName = itksys::SystemTools::GetFilenamePath(inputLocation) + "/" + nodeName + ".nrrd";
      if (itksys::SystemTools::FileExists(fileName))
      {
        auto childData = mitk::IOUtil::Load(fileName)[0];
        auto childImage = dynamic_cast<mitk::Image *>(childData.GetPointer());
        m2::ImzMLImageIO::ValidateChildImage(childImage, parentImage);

        helperNode = mitk::DataNode::New();
        helperNode->SetName(nodeName);
        helperNode->SetVisibility(false);
        helperNode->SetData(childImage);
        helperNode->SetStringProperty("m2aia.helper.image.name", (suffix.substr(1) + "Image").c_str());
        this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
      }
    }

    // -------------- add Normalization to datastorage --------------
    for (auto type : m2::NormalizationStrategyTypeList)
    {
      auto image = spectrumImage->GetNormalizationImage(type);
      auto name = node->GetName() + "." + m2::to_string(type);

      helperNode = mitk::DataNode::New();
      helperNode->SetName(node->GetName() + "." + m2::to_string(type));
      helperNode->SetBoolProperty("binary", false);
      helperNode->SetBoolProperty("helper object", true);
      helperNode->SetData(image); 
      helperNode->SetStringProperty("m2aia.helper.image.normalization.name", name.c_str());

      // add hidden to DS
      this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
      

      if(auto checkBox = m_Controls.settings->findChild<QCheckBox *>(("ckBoxNormalizationImage" + m2::to_string(type)).c_str()))
      emit checkBox->toggled(checkBox->isChecked());
    }
    
    nodeName = itksys::SystemTools::GetFilenameWithoutLastExtension(inputLocation) + ".def";
    auto fileName = itksys::SystemTools::GetFilenamePath(inputLocation) + "/" + nodeName + ".nrrd";
    if(itksys::SystemTools::FileExists(fileName)){
      auto images = mitk::IOUtil::Load(fileName);
      std::string index;
      auto metaData = images[0]->GetMetaDataDictionary();
      itk::ExposeMetaData<std::string>(metaData, "m2aia.stack.index", index);
      helperNode = mitk::DataNode::New();
      helperNode->SetName(nodeName);
      helperNode->SetVisibility(true);
      helperNode->SetData(images[0]);
      helperNode->SetStringProperty("m2aia.helper.image.name", "defImage");
      this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
    }


    // -------------- add Index to datastorage --------------
    nodeName = itksys::SystemTools::GetFilenameWithoutLastExtension(inputLocation) + ".index";
    helperNode = mitk::DataNode::New();
    helperNode->SetName(nodeName);
    helperNode->SetVisibility(m_Controls.showIndexImages->isChecked());
    helperNode->SetData(spectrumImage->GetIndexImage());
    helperNode->SetStringProperty("m2aia.helper.image.name", "index");
    helperNode->SetBoolProperty("binary", false);
    // add hidden to DS
    helperNode->SetBoolProperty("helper object", true);
    this->GetDataStorage()->Add(helperNode, const_cast<mitk::DataNode *>(node));
    emit m_Controls.showIndexImages->toggled(m_Controls.showIndexImages->isChecked());

   

    // -------------- add Spectra to datastorage --------------
    // color for plots in spectrum view

    const auto AddSpectrum = [&node, this](std::string name,
                                          m2::SpectrumFormat type,
                                          std::string info,
                                          const std::vector<double> xs,
                                          const std::vector<double> ys,
                                          bool visibility = false)
    {
      auto intervalsNode = mitk::DataNode::New();
      auto intervals = m2::IntervalVector::New();
      intervals->SetType(type);
      intervals->SetInfo(info);
      intervals->SetProperty("m2aia.helper.spectrum.xaxis.count", mitk::IntProperty::New(xs.size()));

      if (auto image = dynamic_cast<m2::SpectrumImage *>(node->GetData()))
        intervals->SetProperty("m2aia.image.pixel.count", mitk::IntProperty::New(image->GetNumberOfValidPixels()));

      using namespace std;
      auto &i = intervals->GetIntervals();
      transform(begin(xs), end(xs), begin(ys), back_inserter(i), [](auto &a, auto &b) { return m2::Interval(a, b); });
      
      intervalsNode->SetData(intervals);
      intervalsNode->SetName(name);
      intervalsNode->SetVisibility(visibility);
      m2::CopyNodeProperties(node, intervalsNode);       
      intervalsNode->Modified();
      // intervals->
      this->GetDataStorage()->Add(intervalsNode, const_cast<mitk::DataNode *>(node));     
      return intervalsNode;
    };

    const auto &xs = spectrumImage->GetXAxis();

    
 

    if (spectrumImage->GetSpectrumType().Format == m2::SpectrumFormat::ContinuousProfile ||
        spectrumImage->GetSpectrumType().Format == m2::SpectrumFormat::ProcessedProfile)
    {
      AddSpectrum(node->GetName() + ".max_spectrum",
                  m2::SpectrumFormat::Profile,
                  "overview.max",
                  xs,
                  spectrumImage->GetSkylineSpectrum(), false);
      AddSpectrum(node->GetName() + ".mean_spectrum",
                  m2::SpectrumFormat::Profile,
                  "overview.mean",
                  xs,
                  spectrumImage->GetMeanSpectrum(), true);
    }else{
      AddSpectrum(node->GetName() + ".centroids",
                  m2::SpectrumFormat::Centroid,
                  "overview.centroids",
                  xs,
                  spectrumImage->GetMeanSpectrum(), true);
    }

    m_ResetPreventDataStorageOverload.cancel();
    m_ResetPreventDataStorageOverload.setFuture(QtConcurrent::run([](){
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));}));
  }
}

void m2DataView::NodeRemoved(const mitk::DataNode *node)
{
  if (dynamic_cast<m2::SpectrumImage *>(node->GetData()))
  {
    auto derivations = this->GetDataStorage()->GetDerivations(node);
    for (auto &&d : *derivations)
    {
      this->GetDataStorage()->Remove(d);
    }
  }
}
