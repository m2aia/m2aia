/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "QmitkM2aiaAppWorkbenchAdvisor.h"

#include "internal/QmitkM2aiaApplicationPlugin.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QPoint>
#include <QRect>
#include <QmitkExtWorkbenchWindowAdvisor.h>
#include <QmitkM2aiaAboutDialog.h>
#include <QmitkM2aiaViewAction.h>
#include <berryIBerryPreferences.h>
#include <berryIBerryPreferencesService.h>
#include <berryIPreferences.h>
#include <berryIPreferencesService.h>
#include <berryIWorkbenchWindow.h>
#include <berryPlatform.h>
#include <berryQtPreferences.h>
#include <berryWorkbenchPlugin.h>
#include <berryWorkbenchPreferenceConstants.h>
#include <mitkBaseRenderer.h>
#include <mitkCameraController.h>
#include <mitkImage.h>
#include <mitkM2aiaVersion.h>
#include <mitkVersion.h>
#include <qaction.h>
#include <qevent.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>

const QString QmitkM2aiaAppWorkbenchAdvisor::WELCOME_PERSPECTIVE_ID = "org.mitk.m2.application.perspectives.welcome";

class QmitkM2aiaAppWorkbenchWindowAdvisor : public QmitkExtWorkbenchWindowAdvisor
{
public:
  QmitkM2aiaAppWorkbenchWindowAdvisor(berry::WorkbenchAdvisor *wbAdvisor,
                                      berry::IWorkbenchWindowConfigurer::Pointer configurer)
    : QmitkExtWorkbenchWindowAdvisor(wbAdvisor, configurer)
  {

  }

  void PostWindowOpen() override
  {

    QmitkExtWorkbenchWindowAdvisor::PostWindowOpen();
    berry::IWorkbenchWindowConfigurer::Pointer configurer = GetWindowConfigurer();
    configurer->GetWindow()->GetWorkbench()->GetIntroManager()->ShowIntro(configurer->GetWindow(), false);

    // very bad hack...
    berry::IWorkbenchWindow::Pointer window = this->GetWindowConfigurer()->GetWindow();
    QMainWindow *mainWindow = qobject_cast<QMainWindow *>(window->GetShell()->GetControl());
    mainWindow->setContextMenuPolicy(Qt::PreventContextMenu);

    // ==== RenderwindowViewNames ======================

    berry::IPreferencesService *prefService = berry::Platform::GetPreferencesService();
    Q_ASSERT(prefService);
    auto m_Preferences = prefService->GetSystemPreferences()->Node("org.mitk.editors.stdmultiwidget");
    m_Preferences->Put("stdmulti.widget0 corner annotation", "Top");
    m_Preferences->Put("stdmulti.widget1 corner annotation", "Left");
    m_Preferences->Put("stdmulti.widget2 corner annotation", "Front");
    m_Preferences->Put("stdmulti.widget3 corner annotation", "3D");

    // ==== Application menu ============================
    QMenuBar *menuBar = mainWindow->menuBar();
    menuBar->setContextMenuPolicy(Qt::PreventContextMenu);
    auto actions = menuBar->actions();
    for (auto a : actions)
    {
      MITK_INFO << a->text().toStdString();
      if (a->text() == "&Help")
      {
        auto helpMenu = a->menu();
        helpMenu->addAction("&About M2aia", this, []() { QmitkM2aiaAboutDialog().exec(); });
        for (auto b : helpMenu->actions())
        {
          if (b->text() == "&About")
          {
            b->setText("&About MITK");
          }
        }
      }
    }

    // create GUI widgets from the Qt Designer's .ui file
    auto view = new QMenu("View (3D)", menuBar);
    view->addAction("Top", []() {
      if (vtkRenderWindow *renderWindow = mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget3"))
        if (auto controller = mitk::BaseRenderer::GetInstance(renderWindow)->GetCameraController())
          controller->SetViewToCaudal();
    });
    view->addAction("Bottom", []() {
      if (vtkRenderWindow *renderWindow = mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget3"))
        if (auto controller = mitk::BaseRenderer::GetInstance(renderWindow)->GetCameraController())
          controller->SetViewToCranial();
    });

    view->addAction("Front", []() {
      if (vtkRenderWindow *renderWindow = mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget3"))
        if (auto controller = mitk::BaseRenderer::GetInstance(renderWindow)->GetCameraController())
          controller->SetViewToPosterior();
    });

    view->addAction("Back", []() {
      if (vtkRenderWindow *renderWindow = mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget3"))
        if (auto controller = mitk::BaseRenderer::GetInstance(renderWindow)->GetCameraController())
          controller->SetViewToAnterior();
    });

    view->addAction("Left", []() {
      if (vtkRenderWindow *renderWindow = mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget3"))
        if (auto controller = mitk::BaseRenderer::GetInstance(renderWindow)->GetCameraController())
          controller->SetViewToSinister();
    });

    view->addAction("Right", []() {
      if (vtkRenderWindow *renderWindow = mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget3"))
        if (auto controller = mitk::BaseRenderer::GetInstance(renderWindow)->GetCameraController())
          controller->SetViewToDexter();
    });

    menuBar->addMenu(view);

    // berry::IWorkbenchWindow::Pointer window =
    // this->GetWindowConfigurer()->GetWindow();
    // QMainWindow* mainWindow =
    // qobject_cast<QMainWindow*> (window->GetShell()->GetControl());
    // mainWindow->showMaximized();

    const std::vector<QString> viewCategories = {"MS imaging", "Segmentation" /*, "Registration"*/};

    {
      auto prefService = berry::WorkbenchPlugin::GetDefault()->GetPreferencesService();
      berry::IPreferences::Pointer stylePrefs =
        prefService->GetSystemPreferences()->Node(berry::QtPreferences::QT_STYLES_NODE);
      bool showCategoryNames = stylePrefs->GetBool(berry::QtPreferences::QT_SHOW_TOOLBAR_CATEGORY_NAMES, true);

      // Order view descriptors by category

      QMultiMap<QString, berry::IViewDescriptor::Pointer> categoryViewDescriptorMap;
      std::map<QString, berry::IViewDescriptor::Pointer> VDMap;
      // sort elements (converting vector to map...)
      QList<berry::IViewDescriptor::Pointer>::const_iterator iter;

      berry::IViewRegistry *viewRegistry = berry::PlatformUI::GetWorkbench()->GetViewRegistry();
      const QList<berry::IViewDescriptor::Pointer> viewDescriptors = viewRegistry->GetViews();

      // bool skip = false;
      for (iter = viewDescriptors.begin(); iter != viewDescriptors.end(); ++iter)
      {
        if ((*iter)->GetId() == "org.blueberry.ui.internal.introview")
          continue;
        if ((*iter)->GetId() == "org.mitk.views.imagenavigator")
          continue;
        if ((*iter)->GetId() == "org.mitk.views.viewnavigatorview")
          continue;

        std::pair<QString, berry::IViewDescriptor::Pointer> p((*iter)->GetLabel(), (*iter));
        VDMap.insert(p);
      }
      for (auto labelViewDescriptorPair : VDMap)
      {
        auto viewDescriptor = labelViewDescriptorPair.second;
        auto category =
          !viewDescriptor->GetCategoryPath().isEmpty() ? viewDescriptor->GetCategoryPath().back() : QString();

        categoryViewDescriptorMap.insert(category, viewDescriptor);
      }

      // Create a separate toolbar for each category
      berry::IWorkbenchWindow::Pointer window = this->GetWindowConfigurer()->GetWindow();
      QMainWindow *mainWindow = qobject_cast<QMainWindow *>(window->GetShell()->GetControl());

      for (auto category : categoryViewDescriptorMap.uniqueKeys())
      {
        if (std::find(std::begin(viewCategories), std::end(viewCategories), category) == std::end(viewCategories))
          continue;

        auto viewDescriptorsInCurrentCategory = categoryViewDescriptorMap.values(category);

        if (!viewDescriptorsInCurrentCategory.isEmpty())
        {
          auto toolbar = new QToolBar;
          toolbar->setObjectName(category + " View Toolbar");
          mainWindow->addToolBar(toolbar);

          if (showCategoryNames && !category.isEmpty())
          {
            auto categoryButton = new QToolButton;
            categoryButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
            categoryButton->setText(category);
            categoryButton->setStyleSheet("background: transparent; margin: 0; padding: 0;");
            toolbar->addWidget(categoryButton);

            connect(categoryButton, &QToolButton::clicked, [toolbar]() {
              for (QWidget *widget : toolbar->findChildren<QWidget *>())
              {
                if (QStringLiteral("qt_toolbar_ext_button") == widget->objectName() && widget->isVisible())
                {
                  QMouseEvent pressEvent(
                    QEvent::MouseButtonPress, QPointF(0.0f, 0.0f), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                  QMouseEvent releaseEvent(
                    QEvent::MouseButtonRelease, QPointF(0.0f, 0.0f), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                  QApplication::sendEvent(widget, &pressEvent);
                  QApplication::sendEvent(widget, &releaseEvent);
                }
              }
            });
          }

          for (auto viewDescriptor : viewDescriptorsInCurrentCategory)
          {
            auto viewAction = new m2::QmitkM2aiaViewAction(window, viewDescriptor);
            toolbar->addAction(viewAction);
          }
        }
      }
    }

    {
      auto m_Preferences = prefService->GetSystemPreferences()->Node("org.mitk.editors.stdmultiwidget");
      m_Preferences->Put("DepartmentLogo", ":/org.mitk.gui.qt.m2.application/defaultWatermark.png");
    }
  }
};

void QmitkM2aiaAppWorkbenchAdvisor::Initialize(berry::IWorkbenchConfigurer::Pointer configurer)
{
  berry::QtWorkbenchAdvisor::Initialize(configurer);

  configurer->SetSaveAndRestore(true);
}

berry::WorkbenchWindowAdvisor *QmitkM2aiaAppWorkbenchAdvisor::CreateWorkbenchWindowAdvisor(
  berry::IWorkbenchWindowConfigurer::Pointer configurer)
{
  QList<QString> perspExcludeList;
  perspExcludeList.push_back("org.blueberry.uitest.util.EmptyPerspective");
  perspExcludeList.push_back("org.blueberry.uitest.util.EmptyPerspective2");
  perspExcludeList.push_back("org.blueberry.perspectives.help");

  QList<QString> viewExcludeList;
  viewExcludeList.push_back("org.mitk.views.controlvisualizationpropertiesview");
  viewExcludeList.push_back("org.mitk.views.modules");

  QmitkM2aiaAppWorkbenchWindowAdvisor *advisor = new QmitkM2aiaAppWorkbenchWindowAdvisor(this, configurer);
  advisor->ShowViewMenuItem(true);
  advisor->ShowNewWindowMenuItem(true);
  advisor->ShowClosePerspectiveMenuItem(true);
  advisor->SetPerspectiveExcludeList(perspExcludeList);
  advisor->SetViewExcludeList(viewExcludeList);
  advisor->ShowViewToolbar(false);
  advisor->ShowPerspectiveToolbar(true);
  advisor->ShowVersionInfo(false);
  advisor->ShowMitkVersionInfo(false);
  advisor->ShowMemoryIndicator(true);
  advisor->SetProductName("M2aia");
  advisor->SetWindowIcon(":/org.mitk.gui.qt.m2.application/icon.ico");


  std::cout << "M2aia git commit hash: " << MITKM2AIA_REVISION << std::endl;
  std::cout << "M2aia branch name: " << MITKM2AIA_REVISION_NAME << std::endl;
  std::cout << "MITK git commit hash: " << MITK_REVISION << std::endl;
  std::cout << "MITK branch name: " << MITK_REVISION_NAME << std::endl;

  return advisor;
}

QString QmitkM2aiaAppWorkbenchAdvisor::GetInitialWindowPerspectiveId()
{
  return WELCOME_PERSPECTIVE_ID;
}
