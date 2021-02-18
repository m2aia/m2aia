##################################################################
#
# m2aia_create_module
# Wrapps the mitk_create_module function
# by forcing
# set(MITK_MODULE_NAME_PREFIX "M2aia")
# and resetting it after creation
#
##################################################################

function(m2_create_module)

  set(_macro_params
      VERSION                # module version number, e.g. "1.2.0"
      EXPORT_DEFINE          # export macro name for public symbols of this module (DEPRECATED)
      AUTOLOAD_WITH          # a module target name identifying the module which will trigger the
                             # automatic loading of this module
      FILES_CMAKE            # file name of a CMake file setting source list variables
                             # (defaults to files.cmake)
      DEPRECATED_SINCE       # marks this modules as deprecated
      DESCRIPTION            # a description for this module
     )

  set(_macro_multiparams
      SUBPROJECTS            # list of CDash labels (deprecated)
      INCLUDE_DIRS           # include directories: [PUBLIC|PRIVATE|INTERFACE] <list>
      INTERNAL_INCLUDE_DIRS  # include dirs internal to this module (DEPRECATED)
      DEPENDS                # list of modules this module depends on: [PUBLIC|PRIVATE|INTERFACE] <list>
      DEPENDS_INTERNAL       # list of modules this module internally depends on (DEPRECATED)
      PACKAGE_DEPENDS        # list of "packages this module depends on (e.g. Qt, VTK, etc.): [PUBLIC|PRIVATE|INTERFACE] <package-list>
      TARGET_DEPENDS         # list of CMake targets this module should depend on: [PUBLIC|PRIVATE|INTERFACE] <list>
      ADDITIONAL_LIBS        # list of addidtional private libraries linked to this module.
      CPP_FILES              # list of cpp files
      H_FILES                # list of header files: [PUBLIC|PRIVATE] <list>
     )

  set(_macro_options
      FORCE_STATIC           # force building this module as a static library
      HEADERS_ONLY           # this module is a headers-only library
      GCC_DEFAULT_VISIBILITY # do not use gcc visibility flags - all symbols will be exported
      NO_DEFAULT_INCLUDE_DIRS # do not add default include directories like "include" or "."
      NO_INIT                # do not create CppMicroServices initialization code
      NO_FEATURE_INFO        # do not create a feature info by calling add_feature_info()
      WARNINGS_NO_ERRORS     # do not treat compiler warnings as errors
      EXECUTABLE             # create an executable; do not use directly, use mitk_create_executable() instead
      C_MODULE               # compile all source files as C sources
      CXX_MODULE             # compile all source files as C++ sources
     )

    
    #make_parse_arguments(MODULE "${_macro_options}" "${_macro_params}" "${_macro_multiparams}" ${ARGN})
    MITK_CREATE_MODULE()
    


endfunction()
