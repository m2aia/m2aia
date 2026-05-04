/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/



#include <QFileDialog>
#include <QTextCodec>
#include <itksys/SystemTools.hxx>
#include <mitkCoreServices.h>
#include <mitkExceptionMacro.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>
#include "m2BrowserPreferencesPage.h"
#include <ui_m2PreferencePage.h>

static mitk::IPreferences *GetPreferences()
{
  auto *preferencesService = mitk::CoreServices::GetPreferencesService();
  return preferencesService->GetSystemPreferences();
}

m2BrowserPreferencesPage::m2BrowserPreferencesPage()
  : m_Preferences(GetPreferences()), m_Ui(new Ui_m2PreferencePage), m_Control(nullptr)

{
}

m2BrowserPreferencesPage::~m2BrowserPreferencesPage() {}

void m2BrowserPreferencesPage::CreateQtControl(QWidget *parent)
{
  m_Control = new QWidget(parent);
  /*m_ElastixProcess = new QProcess(m_Control);
  m_TransformixProcess = new QProcess(m_Control);*/

  m_Ui->setupUi(m_Control);

  // init widgets
  auto bins = m_Preferences->GetInt("m2aia.view.spectrum.bins", 15000);
  m_Ui->spnBxBins->setValue(bins);

  m_Ui->useMaxIntensity->setChecked(m_Preferences->GetBool("m2aia.view.spectrum.useMaxIntensity", true));
  m_Ui->useMinIntensity->setChecked(m_Preferences->GetBool("m2aia.view.spectrum.useMinIntensity", true));

  // auto v = ;
  // m_Preferences->PutBool("m2aia.view.spectrum.showSamplingPoints",v);
  m_Ui->showSamplingPoints->setChecked(m_Preferences->GetBool("m2aia.view.spectrum.showSamplingPoints", false));
  m_Ui->minimalImagingArea->setChecked(m_Preferences->GetBool("m2aia.view.image.minimal_area", true));
  m_Ui->verboseOutput->setChecked(m_Preferences->GetBool("m2aia.spectrumimage.verbose_output", false));


  connect(m_Ui->spnBxBins, SIGNAL(valueChanged(int)), this, SLOT(OnBinsSpinBoxValueChanged(int)));
  connect(m_Ui->useMaxIntensity, SIGNAL(toggled(bool)), this, SLOT(OnUseMaxIntensity(bool)));
  connect(m_Ui->useMinIntensity, SIGNAL(toggled(bool)), this, SLOT(OnUseMinIntensity(bool)));
  connect(m_Ui->minimalImagingArea, SIGNAL(toggled(bool)), this, SLOT(OnUseMinimalImagingArea(bool)));
  connect(m_Ui->showSamplingPoints, SIGNAL(toggled(bool)), this, SLOT(OnUseSamplingPoints(bool)));
  connect(m_Ui->verboseOutput, SIGNAL(toggled(bool)), this, SLOT(OnUseVerboseOutput(bool)));
}

void m2BrowserPreferencesPage::OnBinsSpinBoxValueChanged(int value)
{
  m_Preferences->PutInt("m2aia.view.spectrum.bins", value);
}


QWidget *m2BrowserPreferencesPage::GetQtControl() const
{
  return m_Control;
}

void m2BrowserPreferencesPage::Init(berry::IWorkbench::Pointer) {}

void m2BrowserPreferencesPage::PerformCancel() {}

bool m2BrowserPreferencesPage::PerformOk()
{
  return true;
}

void m2BrowserPreferencesPage::OnUseSamplingPoints(bool v)
{
  m_Preferences->PutBool("m2aia.view.spectrum.showSamplingPoints", v);
}

void m2BrowserPreferencesPage::OnUseMaxIntensity(bool v)
{
  m_Preferences->PutBool("m2aia.view.spectrum.useMaxIntensity", v);
}

void m2BrowserPreferencesPage::OnUseMinIntensity(bool v)
{
  m_Preferences->PutBool("m2aia.view.spectrum.useMinIntensity", v);
}

void m2BrowserPreferencesPage::OnUseMinimalImagingArea(bool v)
{
  m_Preferences->PutBool("m2aia.view.image.minimal_area", v);
}

void m2BrowserPreferencesPage::OnUseVerboseOutput(bool v)
{
  m_Preferences->PutBool("m2aia.spectrumimage.verbose_output", v);
}

void m2BrowserPreferencesPage::Update()
{
  // optin
}
