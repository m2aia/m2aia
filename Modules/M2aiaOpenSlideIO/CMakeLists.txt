set(MITK_MODULE_NAME_PREFIX "")

find_package(OpenSlide REQUIRED)

MITK_CREATE_MODULE(
  DEPENDS MitkCore 
  PACKAGE_DEPENDS PUBLIC ITK|IOImageBase
  ADDITIONAL_LIBS ${OpenSlide_LIBRARY}
  H_FILES ${OpenSlide_INCLUDE_DIR}/openslide.h
  )

  
  # add_subdirectory(autoload/M2aiaCoreIO)
  
  #if(BUILD_TESTING)
  # add_subdirectory(Testing)
  #endif()
  
  
  #configure_file(mitkM2aiaVersion.h.in ${MITK_BINARY_DIR}/mitkM2aiaVersion.h)
  