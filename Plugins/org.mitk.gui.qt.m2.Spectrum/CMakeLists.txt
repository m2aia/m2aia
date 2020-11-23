project(org_mitk_gui_qt_m2_Spectrum)

mitk_create_plugin(
  EXPORT_DIRECTIVE M2_SPECTRUM_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt MitkM2aiaCore
  PACKAGE_DEPENDS Qt5|Charts Boost
)
