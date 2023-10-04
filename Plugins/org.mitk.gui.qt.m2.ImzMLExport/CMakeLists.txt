project(org_mitk_gui_qt_m2_ImzMLExport)

mitk_create_plugin(
  EXPORT_DIRECTIVE M2IMZMLEXPORT_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt M2aiaCore
  )
