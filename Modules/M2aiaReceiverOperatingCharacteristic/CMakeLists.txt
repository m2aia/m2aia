option(BUILD_M2aiaReceiverOperatingCharacteristic_Export "Build ROC Module for M2aia" ON)
if(BUILD_M2aiaReceiverOperatingCharacteristic_Export OR MITK_BUILD_ALL_APPS)

	# unset module name prefix temporarily
	set(MITK_MODULE_NAME_PREFIX "")

	MITK_CREATE_MODULE(
		INCLUDE_DIRS PUBLIC ${H_FILES} PRIVATE ${CPP_FILES}
		DEPENDS PUBLIC MitkCore M2aiaCore
		PACKAGE_DEPENDS
	)

	if(BUILD_TESTING)
		add_subdirectory(test)
	endif()

	# resets module name prefix to mitk
	set(MITK_MODULE_NAME_PREFIX "Mitk")

endif()
