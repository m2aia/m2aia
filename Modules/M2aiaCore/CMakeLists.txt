set(MITK_MODULE_NAME_PREFIX "")

MITK_CREATE_MODULE(
  DEPENDS MitkCore M2aiaSignalProcessing MitkMultilabel MitkMatchPointRegistration
  PACKAGE_DEPENDS
    PUBLIC Poco MatchPoint
  INCLUDE_DIRS
    PUBLIC algorithms
  )

  if(TARGET ${MODULE_TARGET})
  set(ALG_PROFILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/algorithms)

  include(${MatchPoint_SOURCE_DIR}/CMake/mapFunctionCreateAlgorithmProfile.cmake)
  file(GLOB ALG_PROFILE_FILES LIST_DIRECTORIES false RELATIVE ${ALG_PROFILE_DIR} "${ALG_PROFILE_DIR}/*.profile")

  foreach(profile_file ${ALG_PROFILE_FILES})
    get_filename_component(profile_name ${profile_file} NAME_WE)
    message(STATUS "Generate MDRA profile ${profile_name} (from ${profile_file})")
    CREATE_ALGORITHM_PROFILE(${profile_name} ${ALG_PROFILE_DIR}/${profile_file})
  endforeach()
  
  add_subdirectory(deployment)
endif()
    
  add_subdirectory(autoload/M2aiaCoreIO)
  
  if(BUILD_TESTING)
   add_subdirectory(Testing)
  endif()
  
  
  configure_file(mitkM2aiaVersion.h.in ${MITK_BINARY_DIR}/mitkM2aiaVersion.h)
  