project(org_mitk_gui_qt_m2_data)

mitk_create_plugin(
  EXPORT_DIRECTIVE IMSBASE_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt MitkMultilabel M2aiaCore M2aiaOpenSlideIO
)
