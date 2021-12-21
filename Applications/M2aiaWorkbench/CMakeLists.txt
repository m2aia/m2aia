project(M2aiaWorkbench)
set(M2AIA_APP_NAME M2aiaWorkbench)

# Create a cache entry for the provisioning file which is used to export
# the file name in the MITKConfig.cmake file. This will keep external projects
# which rely on this file happy.
set(M2AIAAPP_PROVISIONING_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${M2AIA_APP_NAME}.provisioning" CACHE INTERNAL "${M2AIA_APP_NAME} provisioning file" FORCE)

set(_src_files
  M2aiaWorkbench.cpp
)

set(_exclude_plugins
  org.blueberry.test
  org.blueberry.uitest
  org.mitk.gui.qt.coreapplication
  org.mitk.gui.qt.extapplication
)

mitkFunctionCreateBlueBerryApplication(
  NAME ${M2AIA_APP_NAME}
  DESCRIPTION "M2aiaWorkbench"
  EXCLUDE_PLUGINS ${_exclude_plugins}
  SOURCES ${_src_files}
)

#Setting application icon for macOS systems
set_target_properties(${M2AIA_APP_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE "icon.icns")

if(APPLE)
  install(FILES "icons/icon.icns" DESTINATION "${M2AIA_APP_NAME}.app/Contents/Resources")
endif(APPLE)

# Add meta dependencies (e.g. on auto-load modules from depending modules)
if(TARGET ${CMAKE_PROJECT_NAME}-autoload)
  add_dependencies(${M2AIA_APP_NAME} ${CMAKE_PROJECT_NAME}-autoload)
endif()

# Add a build time dependency to legacy BlueBerry bundles.
if(MITK_MODULES_ENABLED_PLUGINS)
  add_dependencies(${M2AIA_APP_NAME} ${MITK_MODULES_ENABLED_PLUGINS})
endif()