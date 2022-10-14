option(BUILD_M2aiaReceiverOperatorCharacteristic_Export "Build ROC Module for M2aia" OFF)
if(BUILD_M2aiaReceiverOperatorCharacteristic_Export OR MITK_BUILD_ALL_APPS)

	# set module name prefix to m2 temporarily
	set(MITK_MODULE_NAME_PREFIX "")

	MITK_CREATE_MODULE(
		DEPENDS MitkCore M2aiaCore
		PACKAGE_DEPENDS
	)

	if(BUILD_TESTING)
		add_subdirectory(test)
	endif()

	# resest module name prefix to mitk
	set(MITK_MODULE_NAME_PREFIX "Mitk")

endif()
