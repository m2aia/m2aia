set(MITK_MODULE_NAME_PREFIX "")

MITK_CREATE_MODULE(
  DEPENDS MitkCore
  PACKAGE_DEPENDS
)


if(BUILD_TESTING)
 add_subdirectory(Testing)
endif()
