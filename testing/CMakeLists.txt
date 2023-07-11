###################################################################################################
# Unit test common library: code shared between unit tests
###################################################################################################
add_library(test_common common/TestImages.cc common/TestImages.hh)
target_include_directories(test_common PUBLIC ${PROJECT_SOURCE_DIR}/testing/common)
target_link_libraries(test_common PUBLIC OpenCV::OpenCV)

###################################################################################################
# Unit tests: test cases for the server-side, client-side and common code
# Server will fail to built if any of these test cases failed.
###################################################################################################

#file(GLOB_RECURSE SRV_UT_SRC unittest/*.cc unittest/*.hh)
add_executable(unittest
	unittest/hrb/RequestTarget-UT.cc
	unittest/crypto/Random-UT.cc
	unittest/unittest.cc
)
target_link_libraries(unittest PUBLIC Catch2::Catch2 hrbcommon hrbsrv)

# Automatically run unit tests after the build
# Although we specified the ${CMAKE_BINARY_DIR} as the current directory when running unit tests,
# the test cases should not use absolute path when referring to test data. The test cases
# must not assume the location of ${CMAKE_BINARY_DIR} relative to the source code.
add_custom_command(
	TARGET unittest
	COMMENT "Running server-side unit tests"
	POST_BUILD
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMAND unittest
)

###################################################################################################
# client-side unit tests: test cases for the client-side code
# These test cases will start a server internally.
###################################################################################################

#file(GLOB_RECURSE CLIENT_UT_SRC client/*.cc client/*.hh)
#add_executable(clienttest ${CLIENT_UT_SRC})
#target_link_libraries(clienttest PRIVATE Catch2::Catch2 test_common hrbclient hrbsrv)
#target_compile_definitions(clienttest PRIVATE CONFIG_FILE="${PROJECT_SOURCE_DIR}/etc/hearty_rabbit")

# Automatically run unit tests after the build
# Although we specified the ${CMAKE_BINARY_DIR} as the current directory when running unit tests,
# the test cases should not use absolute path when referring to test data. The test cases
# must not assume the location of ${CMAKE_BINARY_DIR} relative to the source code.
#add_custom_command(
#	TARGET clienttest
#	COMMENT "Running client-side unit tests"
#	POST_BUILD
#	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#	COMMAND clienttest
#)

###################################################################################################
# gui_driver: test driver with GUI
###################################################################################################

#add_executable(gui_driver gui_driver/main.cc)
#target_link_libraries(gui_driver PRIVATE test_common hrbsrv)
