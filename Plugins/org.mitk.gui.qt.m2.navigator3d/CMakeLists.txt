project(org_mitk_gui_qt_m2_navigator3d)

mitk_create_plugin(
  EXPORT_DIRECTIVE IMAGENAVIGATOR3D_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt
)
