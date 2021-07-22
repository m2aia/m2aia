if(CPACK_GENERATOR MATCHES "NSIS")

  # set the package header icon for MUI
  SET(CPACK_PACKAGE_ICON "D:\\Dev\\m2aia\\Applications\\M2aia\\icons\\icon.ico")
  # set the install/unistall icon used for the installer itself
  # There is a bug in NSIS that does not handle full unix paths properly.
  SET(CPACK_NSIS_MUI_ICON "D:\\Dev\\m2aia\\Applications\\M2aia\\icons\\icon.ico")
  SET(CPACK_NSIS_MUI_UNIICON "D:\\Dev\\m2aia\\Applications\\M2aia\\icons\\icon.ico")

  set(CPACK_NSIS_DISPLAY_NAME "M2aia")

  # tell cpack to create links to the doc files
  SET(CPACK_NSIS_MENU_LINKS
    "http://www.github.com/jtfcordes/" "M2aia Github"
    )

  # tell cpack the executables you want in the start menu as links
  SET(CPACK_PACKAGE_EXECUTABLES "M2aia;M2aia" CACHE INTERNAL "Collecting windows shortcuts to executables")

  # tell cpack to create a desktop link to M2aia
  # SET(CPACK_CREATE_DESKTOP_LINKS "M2aia")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\icon.ico")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\www.github.com/jtfcordes/")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\www.github.com/jtfcordes/")
  SET(CPACK_NSIS_CONTACT j.cordes@hs-mannheim.de)
  SET(CPACK_NSIS_MODIFY_PATH ON)

endif(CPACK_GENERATOR MATCHES "NSIS")

