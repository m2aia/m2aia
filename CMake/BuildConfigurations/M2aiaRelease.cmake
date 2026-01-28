set(MITK_CONFIG_PACKAGES
  MatchPoint
  Qt6
  BLUEBERRY
)

set(MITK_CONFIG_PLUGINS
  org.mitk.gui.qt.datamanager
  org.mitk.gui.qt.stdmultiwidgeteditor
  org.mitk.planarfigure  
  org.mitk.gui.qt.imagecropper
  org.mitk.gui.qt.measurementtoolbox
  org.mitk.gui.qt.segmentation
  org.mitk.gui.qt.volumevisualization
  org.mitk.gui.qt.pointsetinteraction
  org.mitk.gui.qt.overlaymanager
  # org.mitk.gui.qt.imagenavigator
  org.mitk.gui.qt.moviemaker
  org.mitk.gui.qt.properties
  org.mitk.gui.qt.viewnavigator
  org.mitk.gui.qt.renderwindowmanager
  org.mitk.gui.qt.multilabelsegmentation
  # org.mitk.gui.qt.geometrytools
  org.mitk.gui.qt.m2.application
  org.mitk.gui.qt.m2.common
  org.mitk.gui.qt.m2.data
#  org.mitk.gui.qt.m2.CombineImages
  org.mitk.gui.qt.m2.spectrum
  org.mitk.gui.qt.m2.peakpicking
  org.mitk.gui.qt.m2.ImzMLExport
  org.mitk.gui.qt.m2.reconstruction3D
  org.mitk.gui.qt.m2.Position
  org.mitk.gui.qt.m2.registration
  org.mitk.gui.qt.matchpoint.evaluator
  # org.mitk.gui.qt.dicombrowser
  # org.mitk.gui.qt.dicominspector
)

set(MITK_VTK_DEBUG_LEAKS OFF CACHE BOOL "Enable VTK Debug Leaks" FORCE)
set(MITK_BUILD_MatchPoint ON CACHE BOOL "Enable MatchPoint Leaks" FORCE)

find_package(Doxygen REQUIRED)

# Ensure that the in-application help can be build
set(BLUEBERRY_QT_HELP_REQUIRED ON CACHE BOOL "Required Qt help documentation in plug-ins" FORCE)


list(REMOVE_ITEM MITK_CONFIG_PLUGINS org.mitk.gui.qt.pythonsettings)
if(DEFINED MITK_USE_Python3 AND MITK_USE_Python3)
    message(STATUS "MITK_USE_Python3 is set to TRUE, remove it as we have selected a NoPython configuration")
    unset(MITK_USE_Python3 CACHE)
endif()
if(DEFINED MITK_USE_SWIG AND MITK_USE_SWIG)
    message(STATUS "MITK_USE_SWIG is set to TRUE, remove it as we have selected a NoPython configuration")
    unset(MITK_USE_SWIG CACHE)
endif()
