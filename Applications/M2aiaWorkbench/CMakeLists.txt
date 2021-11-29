project(M2aiaWorkbench)

set(M2AIAAPP_NAME M2aiaWorkbench)

# Create a cache entry for the provisioning file which is used to export
# the file name in the MITKConfig.cmake file. This will keep external projects
# which rely on this file happy.
set(M2AIAAPP_PROVISIONING_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${M2AIAAPP_NAME}.provisioning" CACHE INTERNAL "${M2AIAAPP_NAME} provisioning file" FORCE)

set(_plugin "")
set(_exclude_plugins "" )

set(_src_files
  M2aiaWorkbench.cpp
)

mitkFunctionCreateBlueBerryApplication(
  NAME ${M2AIAAPP_NAME}
  DESCRIPTION "M2aiaWorkbench"
  PLUGINS ${_plugins}
  EXCLUDE_PLUGINS ${_exclude_plugins}
  SOURCES ${_src_files}
)

# Add meta dependencies (e.g. on auto-load modules from depending modules)
if(TARGET ${CMAKE_PROJECT_NAME}-autoload)
  add_dependencies(${M2AIAAPP_NAME} ${CMAKE_PROJECT_NAME}-autoload)
endif()

# Add a build time dependency to legacy BlueBerry bundles.
if(MITK_MODULES_ENABLED_PLUGINS)
  add_dependencies(${M2AIAAPP_NAME} ${MITK_MODULES_ENABLED_PLUGINS})
endif()