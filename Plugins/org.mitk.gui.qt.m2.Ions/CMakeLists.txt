project(org_mitk_gui_qt_m2_Ions)

mitk_create_plugin(
  EXPORT_DIRECTIVE M2_IONS_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt MitkM2aiaCore
  PACKAGE_DEPENDS 
)

