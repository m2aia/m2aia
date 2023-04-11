set(MITK_MODULE_NAME_PREFIX "")

find_package(OpenSlide REQUIRED)
include_directories(${OpenSlide_INCLUDE_DIR})

MITK_CREATE_MODULE(
  DEPENDS MitkCore 
  PACKAGE_DEPENDS PUBLIC ITK|IOImageBase
  ADDITIONAL_LIBS ${OpenSlide_LIBRARY}
  )


  
  # add_subdirectory(autoload/M2aiaCoreIO)
  
  #if(BUILD_TESTING)
  # add_subdirectory(Testing)
  #endif()
  
  
  #configure_file(mitkM2aiaVersion.h.in ${MITK_BINARY_DIR}/mitkM2aiaVersion.h)
  