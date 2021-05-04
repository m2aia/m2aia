set(MDRA_INCLUDE_DIRS PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../algorithms)
mitkFunctionCreateMatchPointDeployedAlgorithm(NAME M2AIA_FastSymmetricForcesDemons_MultiRes_default CPP_FILES m2FastSymmetricForcesDemonsMultiRes_default.cpp NO_PROFILE_GEN INCLUDE_DIRS ${MDRA_INCLUDE_DIRS})

