# Module/plug-in whitelist for M2aia.
#
# MITK offers no per-module switch: every entry of MITK_MODULES is added
# unconditionally. The only supported filter is this whitelist, which is
# included by mitkFunctionWhitelistModules() and mitkFunctionWhitelistPlugins()
# and must define enabled_modules and enabled_plugins.
#
# Instead of enumerating all core modules by hand - which would silently drop
# every module a future MITK update adds - the lists are derived from MITK's own
# ModuleList.cmake/PluginList.cmake and the entries below are removed from them.
#
# Note that a module which loses a dependency is not an error: it prints
# "won't be built, missing dependency" and disappears, together with any autoload
# IO module it carries. Check the configure output after changing these lists.

set(_m2aia_excluded_modules
  Pharmacokinetics                   # DCE/PK modelling, used by the pharmacokinetics.* plug-ins
  PharmacokineticsUI
  ModelFit                           # only used by the fit.* and pharmacokinetics.* plug-ins
  ModelFitUI
  CEST
  PET
  RT
  RTUI
  DICOMPM
  DICOMTesting
  DICOMUI
  Classification
  XNAT
  Remeshing
  Gizmo
  Persistence
  RESTAPI
  QtOverlays
  CoreCmdApps
)

# Kept on purpose although nothing in M2aia links them directly:
#   DICOM, DICOMQI  MitkMultilabel depends on MitkDICOMQI (which depends on
#                   MitkDICOM), and Modules/DICOM carries autoload/DICOMImageIO.
#   CommandLine     implied by mitkFunctionCreateCommandLineApp(), used by
#                   M2aiaCLI when BUILD_M2aiaCLI_Export is ON.
#   MapperExt/IOExt MitkIOExt is AUTOLOAD_WITH MitkCore and provides additional
#                   VTK-based readers and writers.

# All of these are OFF by default and none is required by an enabled plug-in.
# They are listed so that MITK_BUILD_ALL_PLUGINS=ON cannot try to build them
# against the modules excluded above.
set(_m2aia_excluded_plugins
  org.mitk.gui.qt.pharmacokinetics.concentration.mri
  org.mitk.gui.qt.pharmacokinetics.curvedescriptor
  org.mitk.gui.qt.pharmacokinetics.mri
  org.mitk.gui.qt.pharmacokinetics.pet
  org.mitk.gui.qt.pharmacokinetics.simulation
  org.mitk.gui.qt.fit.demo
  org.mitk.gui.qt.fit.inspector
  org.mitk.gui.qt.fit.genericfitting
  org.mitk.gui.qt.cest
  org.mitk.gui.qt.pet.suvcalculation
  org.mitk.gui.qt.dosevisualization
  org.mitk.gui.qt.dicombrowser
  org.mitk.gui.qt.remeshing
  org.mitk.gui.qt.restapi
  org.mitk.gui.qt.xnat
)

# Re-including the canonical lists is safe: both whitelist functions include this
# file and only afterwards iterate MITK_MODULES/MITK_PLUGINS, so they end up with
# exactly the values they had. It also repairs MITK_MODULES, which the extension
# loop in MITK's CMakeLists.txt leaves holding the last extension's module list.
include("${MITK_SOURCE_DIR}/Modules/ModuleList.cmake")
set(enabled_modules ${MITK_MODULES})
list(REMOVE_ITEM enabled_modules ${_m2aia_excluded_modules})

include("${MITK_SOURCE_DIR}/Plugins/PluginList.cmake")
set(enabled_plugins "")
foreach(_m2aia_plugin ${MITK_PLUGINS})
  string(REGEX REPLACE ":(ON|OFF)$" "" _m2aia_plugin "${_m2aia_plugin}")
  list(APPEND enabled_plugins ${_m2aia_plugin})
endforeach()
list(REMOVE_ITEM enabled_plugins ${_m2aia_excluded_plugins})
