project(org_mitk_gui_qt_m2_connectedcomponents)

mitk_create_plugin(
  EXPORT_DIRECTIVE CONNECTEDCOMPONENTS_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS PRIVATE MitkQtWidgets MitkQtWidgetsExt MitkMultilabel M2aiaCore
  PACKAGE_DEPENDS PRIVATE ITK|Review
)
