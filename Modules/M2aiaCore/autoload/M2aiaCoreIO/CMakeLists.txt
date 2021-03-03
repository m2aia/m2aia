set(MITK_MODULE_NAME_PREFIX "")
MITK_CREATE_MODULE(
  INCLUDE_DIRS
    PRIVATE src/M2aiaCoreIO
  DEPENDS MitkCore M2aiaCore MitkSceneSerialization M2aiaOpenSlideIO
  AUTOLOAD_WITH MitkCore
)
