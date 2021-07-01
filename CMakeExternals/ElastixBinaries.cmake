#-----------------------------------------------------------------------------
# ElastixBinaries
#-----------------------------------------------------------------------------

if(MITK_USE_ElastixBinaries)

  #[[ Sanity checks ]]
  if(DEFINED ElastixBinaries_DIR AND NOT EXISTS ${ElastixBinaries_DIR})
    message(FATAL_ERROR "ElastixBinaries_DIR variable is defined but corresponds to non-existing directory")
  endif()

  set(proj ElastixBinaries)

  if(NOT DEFINED ElastixBinaries_DIR)

  #[[ Reset variables. ]]


    set(dummy_cmd cd .)
    set(patch_cmd ${dummy_cmd})
    set(configure_cmd ${dummy_cmd})
    set(install_cmd "")

    
    set(ElastixBinaries_VERSION 5.0.0)
    set(ElastixBinaries_URL "")
    set(ElastixBinaries_MD5 "")

    if(WIN32)
      set(install_cmd
        ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR> <INSTALL_DIR>/lib/
        && ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR> <INSTALL_DIR>/bin/
      )

      set(build_cmd
        ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR> .
      )
    else()
      set(install_cmd
        ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/lib/ <INSTALL_DIR>/lib/
        && ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/bin/ <INSTALL_DIR>/bin/
      )

      set(build_cmd
        ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR> .
      )

    endif()

    if(WIN32)
      set(ElastixBinaries_URL "https://github.com/SuperElastix/elastix/releases/download/${ElastixBinaries_VERSION}/elastix-${ElastixBinaries_VERSION}-win64.zip")
      set(ElastixBinaries_MD5 b2e93f98c4f9ab8f5250f75340464d88)
    elseif(UNIX)
      set(ElastixBinaries_URL "https://github.com/SuperElastix/elastix/releases/download/${ElastixBinaries_VERSION}/elastix-${ElastixBinaries_VERSION}-linux.tar.bz2")
      set(ElastixBinaries_MD5 c1138d7904836c901482dbd2f9c6e378)
    elseif(APPLE)
      set(ElastixBinaries_URL "https://github.com/SuperElastix/elastix/releases/download/${ElastixBinaries_VERSION}/elastix-${ElastixBinaries_VERSION}-mac.tar.gz")
      set(ElastixBinaries_MD5 9902cf6c3d2b223b58a96bb80ea47046)
    endif()

      
      ExternalProject_Add(${proj}
        URL ${ElastixBinaries_URL}
        URL_MD5 ${ElastixBinaries_MD5}
        PATCH_COMMAND ${patch_cmd}
        CONFIGURE_COMMAND ${configure_cmd}
        BUILD_COMMAND ${build_cmd}
        INSTALL_COMMAND ${install_cmd}
      )

      # ExternalProject_Add_Step(${proj} post_install
      #   COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/ElastixBinaries-post_install-WIN32.cmake
      #   DEPENDEES install
      #   WORKING_DIRECTORY <INSTALL_DIR>/bin
      # )
      
      ExternalProject_Add_Step(${proj} install_manifest
         COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/ElastixBinaries-install_manifest.cmake
         DEPENDEES install
         WORKING_DIRECTORY ${ep_prefix}
       )
       
      set(ElastixBinaries_DIR ${ep_prefix}/bin)

      # Create an configuration file for accessing the correct binary path of elastix
      configure_file(${MITK_CMAKE_EXTERNALS_EXTENSION_DIR}/m2ElxConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/MITK-build/m2ElxConfig.h @ONLY)

    else()
      mitkMacroEmptyExternalProject(${proj})
    endif()

  else()
    mitkMacroEmptyExternalProject(${proj})
  endif()

  
