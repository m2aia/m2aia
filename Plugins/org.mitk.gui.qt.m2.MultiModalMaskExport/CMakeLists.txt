project(org_mitk_gui_qt_m2_MultiModalMaskExport)

mitk_create_plugin(
  EXPORT_DIRECTIVE MULTIMODALMASKEXPORT_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt MitkM2aiaCore MitkSegmentation
)
