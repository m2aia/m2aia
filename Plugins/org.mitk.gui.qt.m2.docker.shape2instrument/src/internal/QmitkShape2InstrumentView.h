/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#pragma once

#include <berryISelectionListener.h>
#include <QmitkAbstractView.h>
#include <QmitkSingleNodeSelectionWidget.h>
#include <QFutureWatcher>

// The generated header from Qt UI Compiler.
#include <ui_Shape2InstrumentViewControls.h>

/**
 * @brief Shape2Instrument Docker plugin view.
 *
 * Converts a segmented multi-label NRRD mask and an MPS calibration PointSet
 * into a Leica XML or MMI CSV instrument file by running the
 * ghcr.io/cemos-mannheim/shape2instrument Docker container.
 *
 * Processing runs in a background thread via QtConcurrent::run() so the GUI
 * remains responsive.  The QFutureWatcher bridges the background thread back
 * to the main (GUI) thread through Qt's signal-slot mechanism.
 */
class QmitkShape2InstrumentView : public QmitkAbstractView
{
  Q_OBJECT

public:
  static const std::string VIEW_ID;

  void CreateQtPartControl(QWidget* parent) override;

private slots:
  /// Called when the user presses "Run".  Validates inputs and launches the
  /// background Docker task.
  void OnStartDockerProcessing();

  /// Called when the user presses "Cancel".  Marks the future as cancelled;
  /// OnDockerProcessingFinished() skips result handling accordingly.
  void OnCancelDockerProcessing();

  /// Called by m_DockerProcessWatcher when the background task finishes
  /// (successfully, with an error, or cancelled).  Always executes on the
  /// GUI thread.
  void OnDockerProcessingFinished();

  /// Show / hide the Cap IDs row depending on the selected output format.
  void OnFormatChanged(const QString& format);

  /// Open a directory-chooser dialog and put the path into the lineEdit.
  void OnBrowseOutputDirectory();

private:
  void SetFocus() override;

  /// Enable or disable all interactive widgets (used while processing).
  void EnableWidgets(bool enable);

  // -------------------------------------------------------------------------
  // Result structure returned from the background thread
  // -------------------------------------------------------------------------
  struct ProcessingResult
  {
    bool    success      = false;
    QString outputFile;       ///< Absolute path of the written output file
    QString errorMessage;     ///< Non-empty on failure
  };

  // -------------------------------------------------------------------------
  // Data members
  // -------------------------------------------------------------------------
  Ui::Shape2InstrumentViewControls m_Controls;

  /// Watcher that bridges the QtConcurrent future back to the GUI thread.
  QFutureWatcher<ProcessingResult> m_DockerProcessWatcher;
};
