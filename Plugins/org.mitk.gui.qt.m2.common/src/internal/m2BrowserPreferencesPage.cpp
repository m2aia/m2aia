/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "m2BrowserPreferencesPage.h"

#include <berryIPreferencesService.h>
#include <berryPlatform.h>

#include <mitkExceptionMacro.h>
#include <ui_m2PreferencePage.h>

#include <QFileDialog>
#include <QTextCodec>

static berry::IPreferences::Pointer GetPreferences()
{
	berry::IPreferencesService* preferencesService = berry::Platform::GetPreferencesService();

	if (preferencesService != nullptr)
	{
		berry::IPreferences::Pointer systemPreferences = preferencesService->GetSystemPreferences();

		if (systemPreferences.IsNotNull())
			return systemPreferences->Node("/org.mitk.gui.qt.m2aia.preferences");
	}

	mitkThrow();
}

m2BrowserPreferencesPage::m2BrowserPreferencesPage()
    :
	m_ElastixProcess(nullptr),
    m_TransformixProcess(nullptr),
    m_Preferences(GetPreferences()),
    m_Ui(new Ui_m2PreferencePage),
    m_Control(nullptr)

{
}

m2BrowserPreferencesPage::~m2BrowserPreferencesPage()
{
}

void m2BrowserPreferencesPage::CreateQtControl(QWidget* parent)
{
	m_Control = new QWidget(parent);
	m_ElastixProcess = new QProcess(m_Control);
	m_TransformixProcess = new QProcess(m_Control);

	m_Ui->setupUi(m_Control);

	connect(m_ElastixProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(OnElastixProcessError(QProcess::ProcessError)));
	connect(m_ElastixProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(OnElastixProcessFinished(int, QProcess::ExitStatus)));
	connect(m_Ui->elastixButton, SIGNAL(clicked()), this, SLOT(OnElastixButtonClicked()));

	connect(
		m_TransformixProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(OnTransformixProcessError(QProcess::ProcessError)));
	connect(m_TransformixProcess,
		SIGNAL(finished(int, QProcess::ExitStatus)),
		this,
		SLOT(OnTransformixProcessFinished(int, QProcess::ExitStatus)));
	connect(m_Ui->transformixButton, SIGNAL(clicked()), this, SLOT(OnTransformixButtonClicked()));

	connect(m_Ui->spnBxBins, SIGNAL(valueChanged(const QString &)), this, SLOT(OnBinsSpinBoxValueChanged(const QString &)));
	connect(m_Ui->useMaxIntensity, SIGNAL(toggled(bool)), this, SLOT(OnUseMaxIntensity(bool)));
	connect(m_Ui->useMinIntensity, SIGNAL(toggled(bool)), this, SLOT(OnUseMinIntensity(bool)));

    // image artifacts
    connect(m_Ui->chkBxIndexImage, SIGNAL(toggled(bool)), this, SLOT(OnShowIndexImage(bool)));
    connect(m_Ui->chkBxMaskImage, SIGNAL(toggled(bool)), this, SLOT(OnShowMaskImage(bool)));
    connect(m_Ui->chkBxNormalizationImage, SIGNAL(toggled(bool)), this, SLOT(OnShowNormalizationImage(bool)));

	this->Update();
}


void m2BrowserPreferencesPage::OnBinsSpinBoxValueChanged(const QString & text){
	m_Preferences->Put("bins", text);
}


void m2BrowserPreferencesPage::OnElastixButtonClicked()
{
	QString filter = "elastix executable ";

#if defined(WIN32)
	filter += "(elastix.exe)";
#else
	filter += "(elastix)";
#endif

	QString path = QFileDialog::getOpenFileName(m_Control, "Elastix", "", filter);

	if (!path.isEmpty())
	{
		m_ElastixPath = path;
		m_ElastixProcess->start(path, QStringList() << "--version", QProcess::ReadOnly);
	}
}

void m2BrowserPreferencesPage::OnElastixProcessError(QProcess::ProcessError)
{
	m_ElastixPath.clear();
	m_Ui->elastixLineEdit->clear();
}

void m2BrowserPreferencesPage::OnElastixProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	if (exitStatus == QProcess::NormalExit && exitCode == 0)
	{
		QString output = QTextCodec::codecForName("UTF-8")->toUnicode(m_ElastixProcess->readAllStandardOutput());

		if (output.startsWith("elastix"))
		{
			m_Ui->elastixLineEdit->setText(m_ElastixPath);
			return;
		}
	}

	m_ElastixPath.clear();
	m_Ui->elastixLineEdit->clear();
}

void m2BrowserPreferencesPage::OnTransformixButtonClicked()
{
	QString filter = "transformix executable ";

#if defined(WIN32)
	filter += "(transformix.exe)";
#else
	filter += "(transformix)";
#endif

	QString path = QFileDialog::getOpenFileName(m_Control, "Transformix", "", filter);

	if (!path.isEmpty())
	{
		m_TransformixPath = path;
		m_TransformixProcess->start(path, QStringList() << "--version", QProcess::ReadOnly);
	}
}

void m2BrowserPreferencesPage::OnTransformixProcessError(QProcess::ProcessError)
{
	m_TransformixPath.clear();
	m_Ui->transformixLineEdit->clear();
}

void m2BrowserPreferencesPage::OnTransformixProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	if (exitStatus == QProcess::NormalExit && exitCode == 0)
	{
		QString output = QTextCodec::codecForName("UTF-8")->toUnicode(m_TransformixProcess->readAllStandardOutput());

		if (output.startsWith("transformix"))
		{
			m_Ui->transformixLineEdit->setText(m_TransformixPath);
			return;
		}
	}

	m_TransformixPath.clear();
	m_Ui->transformixLineEdit->clear();
}

QWidget* m2BrowserPreferencesPage::GetQtControl() const
{
	return m_Control;
}

void m2BrowserPreferencesPage::Init(berry::IWorkbench::Pointer)
{
	
}

void m2BrowserPreferencesPage::PerformCancel()
{
}

bool m2BrowserPreferencesPage::PerformOk()
{
	m_Preferences->Put("elastix", m_ElastixPath);
	m_Preferences->Put("transformix", m_TransformixPath);
	return true;
}

void m2BrowserPreferencesPage::OnUseMaxIntensity(bool v){
	m_Preferences->PutBool("useMaxIntensity", v);
}

void m2BrowserPreferencesPage::OnUseMinIntensity(bool v){
	m_Preferences->PutBool("useMinIntensity", v);
}

void m2BrowserPreferencesPage::OnShowIndexImage(bool v){
    m_Preferences->PutBool("showIndexImage", v);
}

void m2BrowserPreferencesPage::OnShowMaskImage(bool v){
    m_Preferences->PutBool("showMaskImage", v);
}

void m2BrowserPreferencesPage::OnShowNormalizationImage(bool v){
    m_Preferences->PutBool("showNormalizationImage", v);
}

void m2BrowserPreferencesPage::Update()
{
    //optin
    m_Ui->useMaxIntensity->setChecked(m_Preferences->GetBool("useMaxIntensity", true));
    m_Ui->useMinIntensity->setChecked(m_Preferences->GetBool("useMinIntensity", true));

    m_Ui->chkBxIndexImage->setChecked(m_Preferences->GetBool("showIndexImage", false));
    m_Ui->chkBxMaskImage->setChecked(m_Preferences->GetBool("showMaskImage", false));
    m_Ui->chkBxNormalizationImage->setChecked(m_Preferences->GetBool("showNormalizationImage", false));
	

	auto bins = m_Preferences->Get("bins", "");
	if(bins.isEmpty())
		m_Preferences->Put("bins", m_Ui->spnBxBins->text());
	else
		m_Ui->spnBxBins->setValue(bins.toInt());
	
	

	m_ElastixPath = m_Preferences->Get("elastix", "");

	if (!m_ElastixPath.isEmpty())
		m_ElastixProcess->start(m_ElastixPath, QStringList() << "--version", QProcess::ReadOnly);


	m_TransformixPath = m_Preferences->Get("transformix", "");

	if (!m_TransformixPath.isEmpty())
		m_TransformixProcess->start(m_TransformixPath, QStringList() << "--version", QProcess::ReadOnly);
}
