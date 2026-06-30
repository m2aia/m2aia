project(org_mitk_gui_qt_m2_featurelists)

mitk_create_plugin(
  EXPORT_DIRECTIVE M2FEATURELISTS_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt M2aiaCore
)
