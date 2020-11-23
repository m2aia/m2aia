#-----------------------------------------------------------------------------
# M2aia Data
#-----------------------------------------------------------------------------
include(ExternalProject)

set(proj M2aia-Data)
set(proj_DEPENDENCIES)
set(M2AIA-Data_DEPENDS ${proj})

if(BUILD_TESTING)

# set(revision_tag da5dd4ff) # first 8 characters of hash-tag
#                  ^^^^^^^^  these are just to check correct length of hash part

  ExternalProject_Add(${proj}
    SOURCE_DIR ${proj}
    GIT_REPOSITORY https://github.com/jtfc-Ooro/m2aia-data.git 
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    DEPENDS ${proj_DEPENDENCIES}
  )

  # Place of test data
  set(M2AIA_DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/${proj})

  # Create an configuration file for accessing the correct path @M2AIA_DATA_DIR@ within the code
  # MITK-m2aia commit b3c6923538f44
  configure_file(${MITK_CMAKE_EXTERNALS_EXTENSION_DIR}/m2TestingConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/MITK-build/m2TestingConfig.h @ONLY)

else()

  mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")

endif(BUILD_TESTING)
