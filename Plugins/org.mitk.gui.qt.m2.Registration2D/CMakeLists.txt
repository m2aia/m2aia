project(org_mitk_gui_qt_m2_Registration2D)

mitk_create_plugin(
  EXPORT_DIRECTIVE M2REGISTRATION2D_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt M2aiaCore MitkSceneSerialization
)
