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
			return systemPreferences->Node("/org.mitk.gui.qt.ext.externalprograms");
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

	this->Update();
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

void m2BrowserPreferencesPage::Update()
{
	m_ElastixPath = m_Preferences->Get("elastix", "");

	if (!m_ElastixPath.isEmpty())
		m_ElastixProcess->start(m_ElastixPath, QStringList() << "--version", QProcess::ReadOnly);


	m_TransformixPath = m_Preferences->Get("transformix", "");

	if (!m_TransformixPath.isEmpty())
		m_TransformixProcess->start(m_TransformixPath, QStringList() << "--version", QProcess::ReadOnly);
}
