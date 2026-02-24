project(org_mitk_gui_qt_m2_docker_volcano)

mitk_create_plugin(
  EXPORT_DIRECTIVE VOLCANO_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS PRIVATE MitkQtWidgetsExt M2aiaCore MitkDocker
)
