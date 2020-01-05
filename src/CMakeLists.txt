set(Boost_USE_STATIC_LIBS  ON)
find_package(Boost REQUIRED COMPONENTS system program_options)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(OpenCV REQUIRED core imgproc imgcodecs objdetect highgui)
find_package(Doxygen)

# Find libb2
find_library(LIBB2_LIBRARY libb2.a b2 PATHS lib)
find_path(LIBB2_INCLUDE blake2.h PATHS include)
message(STATUS "found libb2 at ${LIBB2_LIBRARY} ${LIBB2_INCLUDE}")

# Find nlohmann/json
find_path(NLOHMANN_INCLUDE nlohmann/json.hpp PATHS include)
message(STATUS "found nlohmann/json at ${NLOHMANN_INCLUDE}")

pkg_check_modules(HIREDIS REQUIRED IMPORTED_TARGET hiredis)
pkg_check_modules(LIBUNWIND IMPORTED_TARGET libunwind)
pkg_check_modules(LIBEXIF REQUIRED IMPORTED_TARGET libexif)

###################################################################################################
# common library: shared code between client and server
###################################################################################################

# common code for both server and client
file(GLOB_RECURSE COMMON_SRC common/*.cc common/*.hh common/*.ipp)
add_library(hrbcommon ${COMMON_SRC})
target_link_libraries(hrbcommon PUBLIC
	magic
	Boost::system Boost::program_options stdc++fs
	${LIBB2_LIBRARY}
	${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(hrbcommon PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/common
)

###################################################################################################
# server library
# all server side codes are built as a library for unit test
###################################################################################################

file(GLOB_RECURSE SERVER_SRC server/*/*.cc server/*/*.hh server/*/*.ipp)
add_library(hrbsrv ${SERVER_SRC} ${CMAKE_BINARY_DIR}/config.hh)
target_link_libraries(hrbsrv PUBLIC
	hrbcommon
	PkgConfig::HIREDIS
	PkgConfig::LIBEXIF
	OpenSSL::SSL
	OpenSSL::Crypto
	${OpenCV_LIBS}
	${LIBUNWIND_STATIC_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(
	hrbsrv PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/server
)

###################################################################################################
# client library
###################################################################################################

file(GLOB_RECURSE CLIENT_SRC client/http/*.cc client/*.hh client/http/*.ipp)
add_library(hrbclient ${CLIENT_SRC})
target_link_libraries(
	hrbclient
	hrbcommon
	OpenSSL::SSL
	OpenSSL::Crypto
	${OpenCV_LIBS}
	${LIBUNWIND_STATIC_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(
	hrbclient PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/client
)

###################################################################################################
# Main server executable
###################################################################################################

add_executable(hearty_rabbit server/main.cc)
target_link_libraries(hearty_rabbit PRIVATE hrbsrv)
add_dependencies(hearty_rabbit unittest clienttest)

###################################################################################################
# hrbsync: directory sync with hearty rabbit server
###################################################################################################

file(GLOB_RECURSE HRB_SYNC_SRC client/hrbsync/src/*.cc client/hrbsync/src/*.hh)
add_library(hrbsync_lib ${HRB_SYNC_SRC})
target_link_libraries(hrbsync_lib PUBLIC hrbclient)
target_include_directories(hrbsync_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/hrbsync)

#file(GLOB_RECURSE HRB_SYNC_UT_SRC hrbsync/unittest/*.cc hrbsync/unittest/*.hh)
#add_executable(hrbsync_unittest ${HRB_SYNC_UT_SRC})
#target_link_libraries(hrbsync_unittest PRIVATE hrbsync_lib Catch2::Catch2)

add_executable(hrbsync client/hrbsync/main.cc)
target_link_libraries(hrbsync PRIVATE hrbsync_lib)

# Automatically run unit tests after the build
# Although we specified the ${CMAKE_BINARY_DIR} as the current directory when running unit tests,
# the test cases should not use absolute path when referring to test data. The test cases
# must not assume the location of ${CMAKE_BINARY_DIR} relative to the source code.
#add_custom_command(
#	TARGET hrbsync_unittest
#	COMMENT "Running hrbsync unit tests"
#	POST_BUILD
#	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#	COMMAND hrbsync_unittest
#)

if (BUILD_CLIENT)
	add_subdirectory(client/qt)
endif()
