set(SRC_CPP_FILES

)

set(INTERNAL_CPP_FILES
  org_mitk_gui_qt_m2_featurelists_Activator.cpp
  m2FeatureListsView.cpp
)

set(UI_FILES

)

set(MOC_H_FILES
  src/internal/org_mitk_gui_qt_m2_featurelists_Activator.h
  src/internal/m2FeatureListsView.h
)

set(CACHED_RESOURCE_FILES
  resources/icon_featurelists.svg
  plugin.xml
)

set(QRC_FILES

)

set(CPP_FILES )

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})
