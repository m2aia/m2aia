project(org_mitk_modules_docker_external)

mitk_create_plugin(
  EXPORT_DIRECTIVE EXTERNAL_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt M2aiaCore
)
