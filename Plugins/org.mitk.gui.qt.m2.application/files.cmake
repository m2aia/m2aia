set(SRC_CPP_FILES
  QmitkM2aiaAppApplication.cpp
  QmitkM2aiaAppWorkbenchAdvisor.cpp
  QmitkM2aiaViewAction.cpp
  QmitkM2aiaAboutDialog.cpp
)

set(INTERNAL_CPP_FILES
  QmitkM2aiaApplicationPlugin.cpp
  Perspectives/QmitkWelcomePerspective.cpp
)

set(UI_FILES
  src/QmitkM2aiaAboutDialog.ui
)

set(MOC_H_FILES
 # src/internal/QmitkM2aiaAppIntroPart.h
  src/internal/QmitkM2aiaApplicationPlugin.h
  src/QmitkM2aiaAppApplication.h
  src/internal/Perspectives/QmitkWelcomePerspective.h

  src/QmitkM2aiaViewAction.h
  src/QmitkM2aiaAboutDialog.h
)

set(CACHED_RESOURCE_FILES
# list of resource files which can be used by the plug-in
# system without loading the plug-ins shared library,
# for example the icon used in the menu and tabs for the
# plug-in views in the workbench
  plugin.xml
  resources/icon_home.png
)

set(QRC_FILES
  resources/org_mitk_gui_qt_m2_application.qrc
)

# set(CPP_FILES)

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})

