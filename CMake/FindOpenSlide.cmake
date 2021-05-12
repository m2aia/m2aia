set(OpenSlide_DIR ${MITK_EXTERNAL_PROJECT_PREFIX}/src/OpenSlide-build/)

# Look for the header.
find_path(OpenSlide_INCLUDE_DIR NAMES openslide.h
  PATH ${MITK_EXTERNAL_PROJECT_PREFIX}/include
  PATH_SUFFIXES openslide
  )
mark_as_advanced(OpenSlide_INCLUDE_DIR)

# Look for the library.
find_library(OpenSlide_LIBRARY 
  NAMES openslide libopenslide
  PATH ${MITK_EXTERNAL_PROJECT_PREFIX}/bin
)
mark_as_advanced(OpenSlide_LIBRARY)

# Handle the QUIETLY and REQUIRED arguments and set OpenSlide_FOUND true if all
# the listed variables are TRUE.

find_package(PackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenSlide DEFAULT_MSG OpenSlide_LIBRARY OpenSlide_INCLUDE_DIR)

if(OpenSlide_FOUND)
  set(OpenSlide_LIBRARIES ${OpenSlide_LIBRARY})
  set(OpenSlide_INCLUDE_DIRS ${OpenSlide_INCLUDE_DIR})
endif()

