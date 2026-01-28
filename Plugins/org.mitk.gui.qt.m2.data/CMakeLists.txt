project(org_mitk_gui_qt_m2_data)

# find_package(Qt6 COMPONENTS WebEngineWidgets REQUIRED)

mitk_create_plugin(
  EXPORT_DIRECTIVE IMSBASE_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt MitkMultilabel M2aiaCore M2aiaOpenSlideIO
  PACKAGE_DEPENDS PUBLIC Qt6|WebEngineWidgets
  # ADDITIONAL_LIBS Qt6::WebEngineWidgets
)
