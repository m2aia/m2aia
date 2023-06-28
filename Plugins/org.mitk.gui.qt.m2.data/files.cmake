set(SRC_CPP_FILES
  Qm2OpenSlideImageIOHelperDialog.cpp
  Qm2PeakListStatisitcsWidget.cpp
)

set(INTERNAL_CPP_FILES
  org_mitk_gui_qt_m2_data_Activator.cpp
  m2Data.cpp
  m2DataTools.cpp
  m2DataStatistics.cpp
)

set(UI_FILES
  src/internal/m2Data.ui
  src/internal/m2DataTools.ui
  src/internal/m2DataStatistics.ui
  src/Qm2OpenSlideImageIOHelperDialog.ui
  src/Qm2PeakListStatisitcsWidget.ui
)

set(MOC_H_FILES
  src/internal/org_mitk_gui_qt_m2_data_Activator.h
  src/internal/m2Data.h
  src/internal/m2DataTools.h
  src/internal/m2DataStatistics.h
  src/Qm2OpenSlideImageIOHelperDialog.h
  src/Qm2PeakListStatisitcsWidget.h
)

# list of resource files which can be used by the plug-in
# system without loading the plug-ins shared library,
# for example the icon used in the menu and tabs for the
# plug-in views in the workbench
set(CACHED_RESOURCE_FILES
  resources/icon_data.svg
  resources/icon_tools.svg
  resources/icon_stats.svg
  plugin.xml
)

# list of Qt .qrc files which contain additional resources
# specific to this plugin
set(QRC_FILES
)

set(CPP_FILES )

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})
