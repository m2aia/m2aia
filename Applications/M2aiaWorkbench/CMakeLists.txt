project(M2aiaWorkbench)

# Create a cache entry for the provisioning file which is used to export
# the file name in the MITKConfig.cmake file. This will keep external projects
# which rely on this file happy.
set(M2AIAAPP_PROVISIONING_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/M2aiaWorkbench.provisioning" CACHE INTERNAL "M2aiaWorkbench provisioning file" FORCE)

# Plug-ins listed below will not be
#  - added as a build-time dependency to the executable
#  - listed in the provisioning file for the executable
#  - installed if they are external plug-ins

set(_src_files
  M2aiaWorkbench.cpp
)


set(_exclude_plugins
  org.mitk.example.gui.customviewer
  org.mitk.example.gui.customviewer.views
  org.mitk.example.gui.selectionservicemitk
  org.mitk.example.gui.selectionservicemitk.views
  org.mitk.example.gui.selectionserviceqt
  org.mitk.example.gui.extensionpointcontribution
  org.mitk.example.gui.extensionpointdefinition
  org.mitk.example.gui.minimalapplication
  org.mitk.example.gui.multipleperspectives
)

mitkFunctionCreateBlueBerryApplication(
  NAME M2aiaWorkbench
  DESCRIPTION "M2aiaWorkbench"
  EXCLUDE_PLUGINS ${_exclude_plugins}
  SOURCES ${_src_files}
)

#Setting application icon for macOS systems
set_target_properties(M2aiaWorkbench PROPERTIES MACOSX_BUNDLE_ICON_FILE "icon.icns")

if(APPLE)
  install(FILES "icons/icon.icns" DESTINATION "M2aiaWorkbench.app/Contents/Resources")
endif(APPLE)

# Add meta dependencies (e.g. on auto-load modules from depending modules)
if(TARGET ${CMAKE_PROJECT_NAME}-autoload)
  add_dependencies(M2aiaWorkbench ${CMAKE_PROJECT_NAME}-autoload)
endif()
# Add a build time dependency to legacy BlueBerry bundles.
if(MITK_MODULES_ENABLED_PLUGINS)
  add_dependencies(M2aiaWorkbench ${MITK_MODULES_ENABLED_PLUGINS})
endif()
