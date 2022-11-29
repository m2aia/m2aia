option(BUILD_M2aiaDockerHelper_Export "Build M2aiaDockerHelper Module" ON)
if(BUILD_M2aiaDockerHelper_Export OR MITK_BUILD_ALL_APPS)

	# unset module name prefix temporarily
	set(MITK_MODULE_NAME_PREFIX "")

	MITK_CREATE_MODULE(
		INCLUDE_DIRS PUBLIC ${H_FILES} PRIVATE ${CPP_FILES}
		DEPENDS PUBLIC MitkCore M2aiaCore
		PACKAGE_DEPENDS PUBLIC Poco
	)

	if(BUILD_TESTING)
		add_subdirectory(test)
	endif()

	# resets module name prefix to mitk
	set(MITK_MODULE_NAME_PREFIX "Mitk")

endif()
