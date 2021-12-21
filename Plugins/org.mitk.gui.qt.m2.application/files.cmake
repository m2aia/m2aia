set(SRC_CPP_FILES
  QmitkM2aiaAppApplication.cpp
  QmitkM2aiaAppWorkbenchAdvisor.cpp
  QmitkM2aiaViewAction.cpp
  QmitkM2aiaAboutDialog.cpp
)

set(INTERNAL_CPP_FILES
  QmitkM2aiaApplicationPlugin.cpp
  Perspectives/QmitkSpectrumImagingPerspective.cpp
)

set(UI_FILES
  src/QmitkM2aiaAboutDialog.ui
)

set(MOC_H_FILES
 # src/internal/QmitkM2aiaAppIntroPart.h
  src/internal/QmitkM2aiaApplicationPlugin.h
  src/QmitkM2aiaAppApplication.h

  src/QmitkM2aiaViewAction.h
  src/QmitkM2aiaAboutDialog.h

  src/internal/Perspectives/QmitkSpectrumImagingPerspective.h
)

set(CACHED_RESOURCE_FILES
# list of resource files which can be used by the plug-in
# system without loading the plug-ins shared library,
# for example the icon used in the menu and tabs for the
# plug-in views in the workbench
  plugin.xml
  resources/perspectives/icon.png
)

set(QRC_FILES
  resources/M2aiaApplication.qrc
)

# set(CPP_FILES)

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})

