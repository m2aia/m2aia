project(org_mitk_gui_qt_m2_plot)

mitk_create_plugin(
  EXPORT_DIRECTIVE M2PLOT_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt MitkMultilabel M2aiaCore
  PACKAGE_DEPENDS PUBLIC VTK|GUISupportQt+RenderingOpenGL2+RenderingAnnotation+InteractionStyle
)
