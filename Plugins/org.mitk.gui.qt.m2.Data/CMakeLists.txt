project(org_mitk_gui_qt_m2_Data)

mitk_create_plugin(
  EXPORT_DIRECTIVE IMSBASE_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt MitkImageStatistics MitkM2aiaCore
  PACKAGE_DEPENDS Qt5|Charts ITK|ITKIONRRD+ITKImageIntensity Boost
)
