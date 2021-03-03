#-----------------------------------------------------------------------------
# Boost
#-----------------------------------------------------------------------------

if(MITK_USE_OpenSlide)

#[[ Sanity checks ]]
if(DEFINED OpenSlide_DIR AND NOT EXISTS ${OpenSlide_DIR})
  message(FATAL_ERROR "OpenSlide_DIR variable is defined but corresponds to non-existing directory")
endif()

set(proj OpenSlide)
set(proj_DEPENDENCIES )
set(OpenSlide_DEPENDS ${proj})

  if(NOT DEFINED OpenSlide_DIR)

  #[[ Reset variables. ]]
  

    set(dummy_cmd cd .)
    set(patch_cmd ${dummy_cmd})
    set(configure_cmd ${dummy_cmd})
    set(install_cmd "")

    set(install_cmd
        ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include/ <INSTALL_DIR>/include/ 
        # && ${CMAKE_COMMAND} -E make_directory <INSTALL_DIR>/lib/OpenSlide 
        # && ${CMAKE_COMMAND} -E make_directory <INSTALL_DIR>/bin/OpenSlide 
        && ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/lib/ <INSTALL_DIR>/lib/
        && ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/bin/ <INSTALL_DIR>/bin/
      )

    if(WIN32)
      set(url "https://github.com/openslide/openslide-winbuild/releases/download/v20171122/openslide-win64-20171122.zip")
      set(md5 68c026f4a06cc5b70fbffabd02ae63a4)
      ExternalProject_Add(${proj}
        URL ${url}
        URL_MD5 ${md5}
        PATCH_COMMAND ${patch_cmd}
        CONFIGURE_COMMAND ${configure_cmd}
        BUILD_COMMAND ""
        INSTALL_COMMAND ${install_cmd}
      )

      ExternalProject_Add_Step(${proj} post_install
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/OpenSlide-post_install-WIN32.cmake
        DEPENDEES install
        WORKING_DIRECTORY <INSTALL_DIR>/bin
      )
      
      ExternalProject_Add_Step(${proj} install_manifest
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/OpenSlide-install_manifest.cmake
        DEPENDEES post_install
        WORKING_DIRECTORY ${ep_prefix}
      )
      set(OpenSlide_DIR ${ep_prefix}/src/OpenSlide)
    else()
      mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
    endif()

  else()
    mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
  endif()
endif()
