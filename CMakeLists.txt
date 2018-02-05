cmake_minimum_required(VERSION 3.6)
project(hearty_rabbit)

set(Boost_USE_STATIC_LIBS  ON)
find_package(Boost REQUIRED COMPONENTS system filesystem program_options)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)

# Find libb2
find_library(LIBB2_LIBRARY libb2.a b2 PATHS lib)
find_path(LIBB2_INCLUDE blake2.h PATHS include)
message(STATUS "found libb2 at ${LIBB2_LIBRARY} ${LIBB2_INCLUDE}")

# Need to remove libsystemd as it pulls a lot of dependencies
#pkg_check_modules(SYSTEMD libsystemd)
pkg_check_modules(RapidJSON REQUIRED RapidJSON)
pkg_check_modules(HIREDIS REQUIRED hiredis)
pkg_check_modules(LIBUNWIND libunwind)

set(CMAKE_CXX_STANDARD 17)
set(VERSION "0.1")
set(CONFIG_FILENAME "hearty_rabbit.json")

add_definitions(-DRAPIDJSON_HAS_STDSTRING=1)

include_directories(
	${Boost_INCLUDE_DIR}
	${SYSTEMD_INCLUDE_DIRS}
	${HIREDIS_INCLUDE_DIRS}
	${LIBUNWIND_INCLUDE_DIRS}
	${LIBB2_INCLUDE}
	${CMAKE_BINARY_DIR}
	${PROJECT_SOURCE_DIR}/src
)

# Post-processing
configure_file(systemd/hearty_rabbit.service.in hearty_rabbit.service)
configure_file(config.hh.in config.hh)

# Static files served by HTTP
file(GLOB STATIC_RESOURCES lib/*)
message(STATUS ${STATIC_RESOURCES})

# Generate a list of static files
set(WEB_RESOURCE_LIST "${CMAKE_CURRENT_BINARY_DIR}/WebResources.hh")
file(REMOVE ${WEB_RESOURCE_LIST})
file(APPEND ${WEB_RESOURCE_LIST} "#include <string>\n#include <unordered_set>\nnamespace hrb {\nconst std::unordered_set<std::string> web_resources = {\n")
foreach(fullpath ${STATIC_RESOURCES})
	get_filename_component(FILENAME_WO_DIR ${fullpath} NAME)
    file(APPEND ${WEB_RESOURCE_LIST} "\t\"${FILENAME_WO_DIR}\",\n")
endforeach(fullpath)
file(APPEND ${WEB_RESOURCE_LIST} "};\n}")

# Put all source code in a library for unit test
file(GLOB LIB_SRC src/*/*.cc src/*/*.hh)
add_library(hrblib ${LIB_SRC} ${CMAKE_CURRENT_BINARY_DIR}/config.hh)
target_link_libraries(
	hrblib
	magic
	${Boost_LIBRARIES}
	${HIREDIS_LIBRARIES}
	OpenSSL::SSL
	OpenSSL::Crypto
	${LIBUNWIND_LIBRARIES}
	${LIBB2_LIBRARY}
	${SYSTEMD_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)

# Main executable
add_executable(hearty_rabbit src/main.cc)
target_link_libraries(
	hearty_rabbit
	hrblib
)

# Unit tests
file(GLOB UNITTEST_SRC unittest/tests/unittest.cc unittest/tests/*/*.cc unittest/tests/*/*.hh)
add_executable(unittest ${UNITTEST_SRC})
target_include_directories(unittest PRIVATE unittest/Catch2/single_include)
target_link_libraries(
	unittest
	hrblib
)

# Automatically run unit tests after the build
# Although we specific the ${CMAKE_BINARY_DIR} as the current directory when running unit tests,
# the test cases should not use absolute path when referring to test data. The test cases
# must not assume the location of ${CMAKE_BINARY_DIR} relative to the source code.
add_custom_command(
	TARGET unittest
	COMMENT "Run unit tests"
	POST_BUILD
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMAND unittest
)

# Installation
install(TARGETS hearty_rabbit DESTINATION bin)
#install(FILES ${CMAKE_BINARY_DIR}/hearty_rabbit.service DESTINATION etc/systemd/system)
#install(FILES etc/${CONFIG_FILENAME} DESTINATION etc)
install(DIRECTORY lib/ DESTINATION lib)

set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME "hearty-rabbit")
set(CPACK_PACKAGE_CONTACT "Nestal Wan")
set(CPACK_PACKAGE_VENDOR "nestal.net")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

# Get build number from environment variable. It should be set by travis.
if (DEFINED ENV{BUILD_NUMBER})
	set(CPACK_RPM_PACKAGE_RELEASE $ENV{BUILD_NUMBER})
else()
	string(TIMESTAMP CPACK_RPM_PACKAGE_RELEASE "%Y%m%d%H%M%S")
endif()

set(CPACK_RPM_PACKAGE_AUTOREQ ON)
set(CPACK_RPM_PACKAGE_RELEASE_DIST ON)
include(CPack)
