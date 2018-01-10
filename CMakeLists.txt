cmake_minimum_required(VERSION 3.6)
project(hearty_rabbit)

set(Boost_USE_STATIC_LIBS  ON)
find_package(Boost REQUIRED COMPONENTS system filesystem program_options)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)

pkg_check_modules(SYSTEMD libsystemd)
pkg_check_modules(RapidJSON REQUIRED RapidJSON)
pkg_check_modules(HIREDIS hiredis)

set(CMAKE_CXX_STANDARD 17)
set(VERSION "0.1")
set(CONFIG_FILENAME "hearty_rabbit.json")

include_directories(${Boost_INCLUDE_DIR} ${SYSTEMD_INCLUDE_DIRS} ${CMAKE_BINARY_DIR} ${PROJECT_SOURCE_DIR}/src)

# Post-processing
configure_file(systemd/hearty_rabbit.service.in hearty_rabbit.service)
configure_file(config.hh.in config.hh)

file(GLOB LIB_SRC src/*/*.cc src/*/*.hh)
add_library(hrblib ${LIB_SRC} ${CMAKE_CURRENT_BINARY_DIR}/config.hh)
target_link_libraries(
	hrblib
	${Boost_LIBRARIES}
	OpenSSL::SSL
	OpenSSL::Crypto
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
add_custom_command(
	TARGET unittest
	COMMENT "Run unit tests"
	POST_BUILD
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMAND unittest
)

if (HIREDIS_FOUND)
	add_executable(test_redis unittest/test_redis.cc)
	target_link_libraries(
		test_redis
		${Boost_LIBRARIES}
		${HIREDIS_LIBRARIES}
		${CMAKE_THREAD_LIBS_INIT}
	)
endif()

# Installation
install(TARGETS hearty_rabbit DESTINATION bin)
install(FILES ${CMAKE_BINARY_DIR}/hearty_rabbit.service DESTINATION etc/systemd/system)
install(FILES etc/${CONFIG_FILENAME} DESTINATION etc)
install(DIRECTORY lib DESTINATION lib/hearty_rabbit)

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
