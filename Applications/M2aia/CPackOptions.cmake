# Set M2aia specific CPack options

set(CPACK_PACKAGE_EXECUTABLES "MitkM2aia;M2aia")
set(CPACK_PACKAGE_NAME "M2aia")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "M2aia - Mass spectrometry imaging applications for interactive analysis in MITK.")
# Major version is the year of release
set(CPACK_PACKAGE_VERSION_MAJOR "2020")
# Minor version is the month of release
set(CPACK_PACKAGE_VERSION_MINOR "11")
# Patch versioning
set(CPACK_PACKAGE_VERSION_PATCH "01")

# this should result in names like 2011.09, 2012.06, ...
# version has to be set explicitly to avoid such things as CMake creating the install directory "MITK M2aia 2020.."
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
#set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")#.${CPACK_PACKAGE_VERSION_PATCH}")

set(CPACK_PACKAGE_FILE_NAME "M2aia-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_ARCH}")

