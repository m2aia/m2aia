project(org_mitk_gui_qt_m2_CombineImages)

mitk_create_plugin(
  EXPORT_DIRECTIVE M2COMBINEIMAGES_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt M2aiaCore
)
