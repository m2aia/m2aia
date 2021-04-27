set(MITK_MODULE_NAME_PREFIX "")

MITK_CREATE_MODULE(
  DEPENDS MitkCore M2aiaSignalProcessing MitkMultilabel
  PACKAGE_DEPENDS
    PUBLIC Poco
  )
    
  add_subdirectory(autoload/M2aiaCoreIO)
  
  if(BUILD_TESTING)
   add_subdirectory(Testing)
  endif()
  
  
  configure_file(mitkM2aiaVersion.h.in ${MITK_BINARY_DIR}/mitkM2aiaVersion.h)
  