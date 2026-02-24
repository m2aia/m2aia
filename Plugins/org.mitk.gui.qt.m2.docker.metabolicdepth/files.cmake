set(SRC_CPP_FILES
)

set(INTERNAL_CPP_FILES
  PluginActivator.cpp
  QmitkMetabolicDepthView.cpp
)

set(UI_FILES
  src/internal/MetabolicDepthViewControls.ui
)

set(MOC_H_FILES
  src/internal/PluginActivator.h
  src/internal/QmitkMetabolicDepthView.h
)

set(CACHED_RESOURCE_FILES
  plugin.xml
  resources/MetabolicDepthIcon.svg
)

set(QRC_FILES
)

set(CPP_FILES
)

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})
