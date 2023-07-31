set(CPP_FILES
  src/internal/PluginActivator.cpp
  src/internal/QmitkSimCLRView.cpp
)

set(UI_FILES
  src/internal/ViewControls.ui
)

set(MOC_H_FILES
  src/internal/PluginActivator.h
  src/internal/QmitkSimCLRView.h
)

# List of resource files that can be used by the plugin system without loading
# the actual plugin. For molecular, the icon that is typically displayed in the
# plugin view menu at the top of the application window.
set(CACHED_RESOURCE_FILES
  resources/logo.svg
  plugin.xml
)

set(QRC_FILES
)
