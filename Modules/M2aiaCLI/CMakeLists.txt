  
#MITK_CREATE_MODULE(
#  DEPENDS MitkCore MitkCommandLine
#  PACKAGE_DEPENDS PUBLIC ITK
#)

#if(MODULE_IS_ENABLED)
  add_subdirectory(Export)
#endif()