project(org_mitk_biomarker_plot_volcano)

mitk_create_plugin(
  EXPORT_DIRECTIVE VOLCANO_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt M2aiaCore M2aiaDockerHelper
)
