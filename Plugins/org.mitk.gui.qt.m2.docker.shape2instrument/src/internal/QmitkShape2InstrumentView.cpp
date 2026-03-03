/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "QmitkShape2InstrumentView.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent>

#include <berryIWorkbenchWindow.h>
#include <berryPlatformUI.h>

#include <mitkCoreServices.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>

#include <mitkDockerHelper.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateFunction.h>
#include <mitkProgressBar.h>
#include <mitkLabelSetImage.h>
#include <mitkPointSet.h>

const std::string QmitkShape2InstrumentView::VIEW_ID =
  "org.mitk.views.m2.docker.shape2instrument";

static const std::string PREF_OUTPUT_DIRECTORY =
  QmitkShape2InstrumentView::VIEW_ID + ".outputDirectory";

// ---------------------------------------------------------------------------
// CreateQtPartControl
// ---------------------------------------------------------------------------
void QmitkShape2InstrumentView::CreateQtPartControl(QWidget *parent)
{
  m_Controls.setupUi(parent);

  // --- Node predicates ---
  auto maskPredicate = mitk::NodePredicateFunction::New(
    [](const mitk::DataNode *node) -> bool
    {
      return dynamic_cast<mitk::MultiLabelSegmentation *>(node->GetData()) != nullptr;
    });

  auto pointSetPredicate = mitk::NodePredicateFunction::New(
    [](const mitk::DataNode *node) -> bool
    {
      return dynamic_cast<mitk::PointSet *>(node->GetData()) != nullptr;
    });

  // --- Mask selection ---
  m_Controls.maskSelection->SetDataStorage(this->GetDataStorage());
  m_Controls.maskSelection->SetSelectionIsOptional(false);
  m_Controls.maskSelection->SetEmptyInfo(QStringLiteral("Select multi-label mask (.nrrd)"));
  m_Controls.maskSelection->SetNodePredicate(maskPredicate);
  m_Controls.maskSelection->SetAutoSelectNewNodes(true);

  // --- Point set (MPS) selection ---
  m_Controls.mpsSelection->SetDataStorage(this->GetDataStorage());
  m_Controls.mpsSelection->SetSelectionIsOptional(false);
  m_Controls.mpsSelection->SetEmptyInfo(QStringLiteral("Select calibration PointSet (.mps)"));
  m_Controls.mpsSelection->SetNodePredicate(pointSetPredicate);
  m_Controls.mpsSelection->SetAutoSelectNewNodes(true);

  // --- Initial UI state ---
  OnFormatChanged(m_Controls.formatCombo->currentText());

  // --- Connections ---
  connect(m_Controls.btnRun,    &QPushButton::clicked,
          this,                 &QmitkShape2InstrumentView::OnStartDockerProcessing);
  connect(m_Controls.btnCancel, &QPushButton::clicked,
          this,                 &QmitkShape2InstrumentView::OnCancelDockerProcessing);
  connect(m_Controls.btnBrowse, &QPushButton::clicked,
          this,                 &QmitkShape2InstrumentView::OnBrowseOutputDirectory);
  connect(m_Controls.formatCombo, &QComboBox::currentTextChanged,
          this,                   &QmitkShape2InstrumentView::OnFormatChanged);

  connect(m_Controls.btnOpenPointSetInteraction, &QPushButton::clicked, this, []()
  {
    try
    {
      if (auto *platform = berry::PlatformUI::GetWorkbench())
        if (auto workbench = platform->GetActiveWorkbenchWindow())
          if (auto page = workbench->GetActivePage())
            if (page.IsNotNull())
              page->ShowView("org.mitk.views.pointsetinteraction", "", 1);
    }
    catch (berry::PartInitException &e)
    {
      BERRY_ERROR << "Error: " << e.what() << std::endl;
    }
  });

  // Wire QFutureWatcher finished signal → GUI-thread slot
  connect(&m_DockerProcessWatcher, &QFutureWatcher<ProcessingResult>::finished,
          this,                    &QmitkShape2InstrumentView::OnDockerProcessingFinished);

  // Initially hide cancel button
  m_Controls.btnCancel->setVisible(false);

  // Restore persisted output directory
  auto *prefsService = mitk::CoreServices::GetPreferencesService();
  if (prefsService != nullptr)
  {
    auto *prefs = prefsService->GetSystemPreferences();
    if (prefs != nullptr)
    {
      const QString savedDir =
        QString::fromStdString(prefs->Get(PREF_OUTPUT_DIRECTORY, ""));
      if (!savedDir.isEmpty())
        m_Controls.outputEdit->setText(savedDir);
    }
  }
}

// ---------------------------------------------------------------------------
void QmitkShape2InstrumentView::SetFocus()
{
  m_Controls.btnRun->setFocus();
}

// ---------------------------------------------------------------------------
void QmitkShape2InstrumentView::EnableWidgets(bool enable)
{
  m_Controls.maskSelection->setEnabled(enable);
  m_Controls.mpsSelection->setEnabled(enable);
  m_Controls.formatCombo->setEnabled(enable);
  m_Controls.capIdsEdit->setEnabled(enable);
  m_Controls.offsetXSpin->setEnabled(enable);
  m_Controls.offsetYSpin->setEnabled(enable);
  m_Controls.scaleSpin->setEnabled(enable);
  m_Controls.invertXCombo->setEnabled(enable);
  m_Controls.invertYCombo->setEnabled(enable);
  m_Controls.outputEdit->setEnabled(enable);
  m_Controls.btnBrowse->setEnabled(enable);
  m_Controls.btnRun->setEnabled(enable);
  m_Controls.btnCancel->setVisible(!enable);
}

// ---------------------------------------------------------------------------
void QmitkShape2InstrumentView::OnFormatChanged(const QString &format)
{
  const bool isXml = (format == "xml");
  m_Controls.labelCapIds->setVisible(isXml);
  m_Controls.capIdsEdit->setVisible(isXml);
}

// ---------------------------------------------------------------------------
void QmitkShape2InstrumentView::OnBrowseOutputDirectory()
{
  const QString dir = QFileDialog::getExistingDirectory(
    nullptr, "Select Output Directory", m_Controls.outputEdit->text());
  if (!dir.isEmpty())
  {
    m_Controls.outputEdit->setText(dir);
    auto *prefsService = mitk::CoreServices::GetPreferencesService();
    if (prefsService != nullptr)
    {
      auto *prefs = prefsService->GetSystemPreferences();
      if (prefs != nullptr)
        prefs->Put(PREF_OUTPUT_DIRECTORY, dir.toStdString());
    }
  }
}

// ---------------------------------------------------------------------------
void QmitkShape2InstrumentView::OnStartDockerProcessing()
{
  if (!mitk::DockerHelper::CanRunDocker())
  {
    QMessageBox::warning(nullptr, "Docker Error",
      "Docker is not available. Please ensure Docker is installed and running.");
    return;
  }

  auto maskNode    = m_Controls.maskSelection->GetSelectedNode();
  auto mpsNode     = m_Controls.mpsSelection->GetSelectedNode();
  const QString outputDir = m_Controls.outputEdit->text().trimmed();

  if (!maskNode || !mpsNode)
  {
    QMessageBox::warning(nullptr, "Selection Error",
      "Please select both a multi-label mask and a calibration PointSet.");
    return;
  }

  if (outputDir.isEmpty())
  {
    QMessageBox::warning(nullptr, "Output Error",
      "Please specify an output directory.");
    return;
  }

  const QString format   = m_Controls.formatCombo->currentText();
  const QString capIds   = m_Controls.capIdsEdit->text().trimmed();

  if (format == "xml" && capIds.isEmpty())
  {
    QMessageBox::warning(nullptr, "Cap IDs Required",
      "Capture IDs are required for the XML format.\n"
      "Enter a comma-separated list matching the number of label segments.");
    return;
  }

  // Capture parameters for the lambda (avoid capturing UI widgets across threads)
  const double  offsetX  = m_Controls.offsetXSpin->value();
  const double  offsetY  = m_Controls.offsetYSpin->value();
  const double  scale    = m_Controls.scaleSpin->value();
  const double  invertX  = (m_Controls.invertXCombo->currentText() == "-1") ? -1.0 : 1.0;
  const double  invertY  = (m_Controls.invertYCombo->currentText() == "-1") ? -1.0 : 1.0;

  // Extract the pixel image for group 0 from the MultiLabelSegmentation
  auto *seg = dynamic_cast<mitk::MultiLabelSegmentation *>(maskNode->GetData());
  if (!seg)
  {
    QMessageBox::critical(nullptr, "Type Error",
      "Selected mask node does not contain a MultiLabelSegmentation.");
    return;
  }
  mitk::BaseData::Pointer maskData = seg->GetGroupImage(0);
  mitk::BaseData::Pointer mpsData  = mpsNode->GetData();

  EnableWidgets(false);
  mitk::ProgressBar::GetInstance()->AddStepsToDo(1);

  m_DockerProcessWatcher.setFuture(
    QtConcurrent::run([maskData, mpsData, outputDir, format, capIds,
                       offsetX, offsetY, scale, invertX, invertY]() -> ProcessingResult
    {
      ProcessingResult result;
      try
      {
        mitk::DockerHelper helper("ghcr.io/cemos-mannheim/shape2instrument:latest");
        helper.EnableAutoRemoveContainer(true);
        helper.EnableGPUs(false);

        // Provide the mask (group 0 image from MultiLabelSegmentation → .nrrd) and calibration PointSet (→ .mps)
        helper.AddAutoSaveData(maskData, "--mask",   "mask",   ".nrrd");
        helper.AddAutoSaveData(mpsData,  "--mps",    "points", ".mps");

        // The output directory is created inside the container working dir
        helper.AddAutoLoadOutputFolder("--output", "output", {});

        helper.AddApplicationArgument("--format",   format.toStdString());
        helper.AddApplicationArgument("--offset_x", std::to_string(offsetX));
        helper.AddApplicationArgument("--offset_y", std::to_string(offsetY));
        helper.AddApplicationArgument("--scale",    std::to_string(scale));
        helper.AddApplicationArgument("--invert_x", std::to_string(invertX));
        helper.AddApplicationArgument("--invert_y", std::to_string(invertY));

        if (format == "xml" && !capIds.isEmpty())
          helper.AddApplicationArgument("--cap_ids", capIds.toStdString());

        // Run – GetResults() blocks until Docker exits; we are already off-thread
        helper.GetResults();

        // Locate the generated output file inside the container working directory
        const auto workDir   = helper.GetWorkingDirectory();
        const auto outSubdir = workDir / "output";
        boost::filesystem::path found;
        if (boost::filesystem::exists(outSubdir))
        {
          for (auto &entry : boost::filesystem::directory_iterator(outSubdir))
          {
            if (boost::filesystem::is_regular_file(entry))
            {
              found = entry.path();
              break;
            }
          }
        }

        // Copy the result file to the user-specified output directory if it is
        // a valid, writable directory (and distinct from where Docker wrote it).
        boost::filesystem::path finalPath = found;
        if (!found.empty() && !outputDir.isEmpty())
        {
          const boost::filesystem::path destDir(outputDir.toStdString());
          boost::system::error_code ec;
          if (boost::filesystem::is_directory(destDir, ec) && !ec)
          {
            const auto destFile = destDir / found.filename();
            boost::system::error_code ec2;
            // Remove existing file first so copy_file works across all Boost versions
            if (boost::filesystem::exists(destFile))
              boost::filesystem::remove(destFile, ec2);
            boost::filesystem::copy_file(found, destFile, ec);
            if (!ec)
              finalPath = destFile;
            else
              MITK_WARN << "Could not copy output to user directory: " << ec.message();
          }
        }

        result.success    = true;
        result.outputFile = finalPath.empty() ? outputDir : QString::fromStdString(finalPath.string());
      }
      catch (const std::exception &e)
      {
        result.success      = false;
        result.errorMessage = QString::fromStdString(e.what());
        MITK_ERROR << "Shape2Instrument Docker run failed: " << e.what();
      }

      mitk::ProgressBar::GetInstance()->Progress();
      return result;
    }));
}

// ---------------------------------------------------------------------------
void QmitkShape2InstrumentView::OnCancelDockerProcessing()
{
  // Qt futures launched with QtConcurrent::run() cannot be interrupted
  // mid-flight, but we flag it so OnDockerProcessingFinished() skips result
  // handling and the user gets immediate visual feedback.
  m_DockerProcessWatcher.cancel();
  m_Controls.statusLabel->setText("Cancelling…");
}

// ---------------------------------------------------------------------------
void QmitkShape2InstrumentView::OnDockerProcessingFinished()
{
  EnableWidgets(true);
  mitk::ProgressBar::GetInstance()->Reset();

  if (m_DockerProcessWatcher.isCanceled())
  {
    m_Controls.statusLabel->setText("Cancelled.");
    return;
  }

  const ProcessingResult result = m_DockerProcessWatcher.result();

  if (result.success)
  {
    m_Controls.statusLabel->setText("Done: " + result.outputFile);
    QMessageBox::information(nullptr, "Shape2Instrument",
      "Processing finished.\nOutput: " + result.outputFile);
  }
  else
  {
    m_Controls.statusLabel->setText("Error – see log.");
    QMessageBox::critical(nullptr, "Shape2Instrument Error",
      "Docker processing failed:\n" + result.errorMessage);
  }
}
