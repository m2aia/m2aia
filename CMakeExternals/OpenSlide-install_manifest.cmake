message(STATUS "Creating install manifest")

file(GLOB_RECURSE installed_headers "include/OpenSlide/*")
set(installed_binaries 
  iconv.dll
  iconv.dll.debug
  libcairo-2.dll
  libcairo-2.dll.debug
  libffi-6.dll
  libffi-6.dll.debug
  libgdk_pixbuf-2.0-0.dll
  libgdk_pixbuf-2.0-0.dll.debug
  libgio-2.0-0.dll
  libgio-2.0-0.dll.debug
  libglib-2.0-0.dll
  libglib-2.0-0.dll.debug
  libgmodule-2.0-0.dll
  libgmodule-2.0-0.dll.debug
  libgobject-2.0-0.dll
  libgobject-2.0-0.dll.debug
  libgthread-2.0-0.dll
  libgthread-2.0-0.dll.debug
  libintl-8.dll
  libintl-8.dll.debug
  libjpeg-62.dll
  libjpeg-62.dll.debug
  libopenjp2.dll
  libopenjp2.dll.debug
  libopenslide-0.dll
  libopenslide-0.dll.debug
  libpixman-1-0.dll
  libpixman-1-0.dll.debug
  libpng16-16.dll
  libpng16-16.dll.debug
  libsqlite3-0.dll
  libsqlite3-0.dll.debug
  libtiff-5.dll
  libtiff-5.dll.debug
  libxml2-2.dll
  libxml2-2.dll.debug
  openslide-jni.dll
  openslide-jni.dll.debug
  zlib1.dll
  zlib1.dll.debug
  libopenslide.lib
)

set(install_manifest "src/OpenSlide-build/install_manifest.txt")
file(WRITE ${install_manifest} "")

foreach(f ${installed_binaries} ${installed_headers})
  file(APPEND ${install_manifest} "${f}\n")
endforeach()

set(install_targets "src/OpenSlide-build/OpenSlideTargets.cmake")
file(WRITE ${install_targets} "")

set(install_config "src/OpenSlide-build/OpenSlideConfig.cmake")
file(WRITE ${install_config} "")
file(APPEND ${install_config} "
set(OpenSlide_MAJOR_VERSION "2017")
set(OpenSlide_MINOR_VERSION "11")
set(OpenSlide_BUILD_VERSION "22")

# The libraries.
set(OpenSlide_LIBRARIES "")

# The CMake macros dir.
set(OpenSlide_CMAKE_DIR "")

set(OpenSlide_USE_FILE "")

get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

# This is a build tree\n
set(OpenSlide_INCLUDE_DIRS "D:/Dev/MITK-superbuild/ep/include/Openslide")
set(OpenSlide_LIBRARY_DIRS "D:/Dev/MITK-superbuild/ep/lib")


# Backward compatible part:
set(OpenSlide_FOUND       TRUE)")
