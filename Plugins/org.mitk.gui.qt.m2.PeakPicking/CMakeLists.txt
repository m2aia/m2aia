project(org_mitk_gui_qt_m2_PeakPicking)

mitk_create_plugin(
  EXPORT_DIRECTIVE M2PEAKPICKING_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt MitkM2aiaCore
)

