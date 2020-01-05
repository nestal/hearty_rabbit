###################################################################################################
# hrbsync: directory sync with hearty rabbit server
###################################################################################################

file(GLOB_RECURSE HRB_SYNC_SRC hrbsync/src/*.cc hrbsync/src/*.hh)
add_library(hrbsync_lib ${HRB_SYNC_SRC})
target_link_libraries(hrbsync_lib PUBLIC hrbclient)
target_include_directories(hrbsync_lib PUBLIC hrbsync/src)

file(GLOB_RECURSE HRB_SYNC_UT_SRC hrbsync/unittest/*.cc hrbsync/unittest/*.hh)
add_executable(hrbsync_unittest ${HRB_SYNC_UT_SRC})
target_link_libraries(hrbsync_unittest PRIVATE hrbsync_lib Catch2::Catch2)

add_executable(hrbsync hrbsync/main.cc)
target_link_libraries(hrbsync PRIVATE hrbsync_lib)

# Automatically run unit tests after the build
# Although we specified the ${CMAKE_BINARY_DIR} as the current directory when running unit tests,
# the test cases should not use absolute path when referring to test data. The test cases
# must not assume the location of ${CMAKE_BINARY_DIR} relative to the source code.
add_custom_command(
	TARGET hrbsync_unittest
	COMMENT "Running hrbsync unit tests"
	POST_BUILD
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMAND hrbsync_unittest
)


add_subdirectory(qt)
