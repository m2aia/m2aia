
option(BUILD_M2aiaCLI_Export "Build commandline tools for M2aia" OFF)
if(BUILD_M2aiaCLI_Export OR MITK_BUILD_ALL_APPS)

  set(MITK_MODULE_NAME_PREFIX "M2aia")
  # needed include directories
  include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    )

    # list of diffusion cmdapps
    # if an app requires additional dependencies
    # they are added after a ^^ and separated by _
  set( m2cli_export_apps
    NLinkedGlycan^^
    ImzMLConverter^^
    PeakPicking^^
    MsImageRegistration^^MitkMatchPointRegistration
    )

  foreach(m2cli_export_app ${m2cli_export_apps})
      # extract cmd app name and dependencies
      string(REPLACE "^^" "\\;" cmdapp_info ${m2cli_export_app})
      set(cmdapp_info_list ${cmdapp_info})
      list(GET cmdapp_info_list 0 appname)
      list(GET cmdapp_info_list 1 raw_dependencies)
      string(REPLACE "_" "\\;" dependencies "${raw_dependencies}")
      set(dependencies_list ${dependencies})

      mitkFunctionCreateCommandLineApp(
        NAME ${appname}
        DEPENDS M2aiaCore ${dependencies_list}
        PACKAGE_DEPENDS ITK VTK Eigen
	    CPP_FILES ${appname}.cpp
      )
  endforeach()
  
  set(MITK_MODULE_NAME_PREFIX "Mitk")
  
  if(EXECUTABLE_IS_ENABLED)
    MITK_INSTALL_TARGETS(EXECUTABLES ${EXECUTABLE_TARGET})
  endif()

  endif()
