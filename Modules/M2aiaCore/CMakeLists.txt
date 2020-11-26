MITK_CREATE_MODULE(
  DEPENDS MitkCore
  PACKAGE_DEPENDS
    PRIVATE Boost
)

add_subdirectory(autoload/IO)

#if(BUILD_TESTING)
# add_subdirectory(Testing)
#endif()


configure_file(mitkM2aiaVersion.h.in ${MITK_BINARY_DIR}/mitkM2aiaVersion.h)
