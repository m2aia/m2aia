set(SRC_CPP_FILES
  #m2IonsInfoSelection.cpp
  #m2IonsInfoObject.cpp
  #m2IonsInfoSelectionProvider.cpp
  m2CommunicationService.cpp
)

set(INTERNAL_CPP_FILES
  org_mitk_m2_core_helper_Activator.cpp
  m2BrowserPreferencesPage.cpp
  #m2BrowserConstants.cpp
  QmitkDirectoryListWidget.cpp
  QmitkFileListWidget.cpp
)

set(UI_FILES
  src/internal/QmitkPathListWidget.ui
  src/internal/m2PreferencePage.ui
)

set(H_FILES
  #src/m2BrowserConstants.h
  #src/m2IonsInfoSelection.h
  #src/m2IonsInfoObject.h
)

set(MOC_H_FILES
  src/internal/org_mitk_m2_core_helper_Activator.h
  src/internal/m2BrowserPreferencesPage.h
  src/internal/QmitkDirectoryListWidget.h
  src/internal/QmitkFileListWidget.h
  src/m2CommunicationService.h
)

# list of resource files which can be used by the plug-in
# system without loading the plug-ins shared library,
# for example the icon used in the menu and tabs for the
# plug-in views in the workbench
set(CACHED_RESOURCE_FILES
  plugin.xml
  resources/MAPRegData.svg
)

# list of Qt .qrc files which contain additional resources
# specific to this plugin
set(QRC_FILES
  resources/QmitkM2aiaCore.qrc
)

set(CPP_FILES )

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})

