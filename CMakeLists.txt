# cmake 3.8 is required for C++17
cmake_minimum_required(VERSION 3.8)
project(hearty_rabbit)

set(VERSION "0.5-pre")

# Get build number from environment variable. It should be set by travis.
if (NOT BUILD_NUMBER)
	string(TIMESTAMP BUILD_NUMBER "%Y%m%d%H%M%S")
endif()
message(STATUS "Building HeartyRabbit ${VERSION}.${BUILD_NUMBER}")

set(Boost_USE_STATIC_LIBS  ON)
find_package(Boost REQUIRED COMPONENTS system filesystem program_options)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(OpenCV REQUIRED core imgproc imgcodecs)
find_package(Doxygen)
find_package(CURL)

# Find libb2
find_library(LIBB2_LIBRARY libb2.a b2 PATHS lib)
find_path(LIBB2_INCLUDE blake2.h PATHS include)
message(STATUS "found libb2 at ${LIBB2_LIBRARY} ${LIBB2_INCLUDE}")

# Find nlohmann/json
find_path(NLOHMANN_INCLUDE json.hpp PATHS include)
message(STATUS "found nlohmann/json at ${NLOHMANN_INCLUDE}")

pkg_check_modules(HIREDIS REQUIRED hiredis)
pkg_check_modules(LIBUNWIND libunwind)
pkg_check_modules(EXIV2 exiv2)

set(CMAKE_CXX_STANDARD 17)
set(CONFIG_FILENAME "hearty_rabbit.json")

# use new C++ ABI
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)

include_directories(
	${Boost_INCLUDE_DIR}
	${HIREDIS_INCLUDE_DIRS}
	${LIBUNWIND_INCLUDE_DIRS}
	${LIBB2_INCLUDE}
	${NLOHMANN_INCLUDE}
	${CMAKE_BINARY_DIR}
	${PROJECT_SOURCE_DIR}/src
)

# Post-processing
configure_file(config.hh.in config.hh)

###################################################################################################
# Generating the server resource list
###################################################################################################

# Static files served by HTTP
file(GLOB STATIC_RESOURCES  lib/static/*)
file(GLOB DYNAMIC_RESOURCES lib/dynamic/*)

# Generate a list of static files
set(WEB_RESOURCE_LIST "${CMAKE_CURRENT_BINARY_DIR}/ResourcesList.hh")
file(REMOVE ${WEB_RESOURCE_LIST})
file(APPEND ${WEB_RESOURCE_LIST} "#include <string>\n#include <unordered_set>\nnamespace hrb {\n")

file(APPEND ${WEB_RESOURCE_LIST} "const std::unordered_set<std::string> static_resources = {\n")
foreach(fullpath ${STATIC_RESOURCES})
	get_filename_component(FILENAME_WO_DIR ${fullpath} NAME)
    file(APPEND ${WEB_RESOURCE_LIST} "\t\"${FILENAME_WO_DIR}\",\n")
endforeach(fullpath)
file(APPEND ${WEB_RESOURCE_LIST} "};\nconst std::unordered_set<std::string> dynamic_resources = {\n")
foreach(fullpath ${DYNAMIC_RESOURCES})
	get_filename_component(FILENAME_WO_DIR ${fullpath} NAME)
    file(APPEND ${WEB_RESOURCE_LIST} "\t\"${FILENAME_WO_DIR}\",\n")
endforeach(fullpath)
file(APPEND ${WEB_RESOURCE_LIST} "};\n}")

###################################################################################################
# common library
###################################################################################################

# common code for both server and client
file(GLOB_RECURSE COMMON_SRC src/common/*.cc src/common/*.hh src/common/*.ipp)
add_library(hrbcommon ${COMMON_SRC})
target_link_libraries(
	hrbcommon
	${Boost_LIBRARIES}
	${LIBB2_LIBRARY}
	${CMAKE_THREAD_LIBS_INIT}
)

###################################################################################################
# server library
# all server side codes are built as a library for unit test
###################################################################################################

file(GLOB_RECURSE SERVER_SRC src/server/*.cc src/server/*.hh src/server/*.ipp)
add_library(hrbsrv ${SERVER_SRC} ${CMAKE_CURRENT_BINARY_DIR}/config.hh)
target_link_libraries(
	hrbsrv
	hrbcommon
	magic
	${Boost_LIBRARIES}
	${HIREDIS_LIBRARIES}
	OpenSSL::SSL
	OpenSSL::Crypto
	${OpenCV_LIBS}
	${LIBUNWIND_STATIC_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(
	hrbsrv PUBLIC
	${PROJECT_SOURCE_DIR}/src/server
)

###################################################################################################
# client library
###################################################################################################

if (CURL_FOUND)
	message(STATUS "building client library")
	file(GLOB_RECURSE CLIENT_SRC src/client/*.cc src/client/*.hh src/client/*.ipp)
	add_library(hrbclient ${CLIENT_SRC})
	target_link_libraries(
		hrbclient
		hrbcommon
		${Boost_LIBRARIES}
		OpenSSL::SSL
		OpenSSL::Crypto
		${OpenCV_LIBS}
		${LIBUNWIND_STATIC_LIBRARIES}
		${CURL_LIBRARIES}
		${CMAKE_THREAD_LIBS_INIT}
	)
	target_include_directories(
		hrbsrv PUBLIC
		${PROJECT_SOURCE_DIR}/src/client
		${CURL_INCLUDE_DIRS}
	)
endif()

###################################################################################################
# unit tests
###################################################################################################

file(GLOB_RECURSE UNITTEST_SRC unittest/tests/unittest.cc
	unittest/tests/*.cc unittest/tests/*/*.hh
)
add_executable(unittest ${UNITTEST_SRC})
target_include_directories(
	unittest PRIVATE
	unittest/Catch2/single_include
	${PROJECT_SOURCE_DIR}/src/server
	${EXIV2_INCLUDE_DIRS}
)
target_link_libraries(
	unittest
	hrbsrv
	${EXIV2_LIBRARIES}		# exiv2 is used to verify the EXIF decoder
)

# Automatically run unit tests after the build
# Although we specified the ${CMAKE_BINARY_DIR} as the current directory when running unit tests,
# the test cases should not use absolute path when referring to test data. The test cases
# must not assume the location of ${CMAKE_BINARY_DIR} relative to the source code.
add_custom_command(
	TARGET unittest
	COMMENT "Run unit tests"
	POST_BUILD
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMAND unittest
)

###################################################################################################
# Main server executable
###################################################################################################

add_executable(hearty_rabbit src/server/main.cc)
target_link_libraries(
	hearty_rabbit
	hrbsrv
)
add_dependencies(hearty_rabbit unittest)

if (BUILD_CLIENT)
	add_subdirectory(client/qt)
endif()

###################################################################################################
# Doxygen
###################################################################################################

if (DOXYGEN_FOUND)
	configure_file(doc/Doxyfile.in Doxyfile @ONLY)
	add_custom_command(OUTPUT html
		COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
		COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/html
		DEPENDS ${LIB_SRC} doc/Doxyfile.in doc/doxygenextra.css
	)
	add_custom_target(doc ALL DEPENDS html)
	set_target_properties(doc PROPERTIES EXCLUDE_FROM_ALL TRUE)
else()
	message(STATUS "No Doxygen found. Documentation won't be built")
endif()

###################################################################################################
# Installation
###################################################################################################

install(TARGETS hearty_rabbit DESTINATION bin)
install(DIRECTORY lib/ DESTINATION lib)

set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME "hearty-rabbit")
set(CPACK_PACKAGE_CONTACT "Nestal Wan")
set(CPACK_PACKAGE_VENDOR "nestal.net")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

# Get build number from environment variable. It should be set by travis.
set(CPACK_RPM_PACKAGE_RELEASE ${BUILD_NUMBER})
set(CPACK_RPM_PACKAGE_AUTOREQ ON)
set(CPACK_RPM_PACKAGE_RELEASE_DIST ON)
include(CPack)
