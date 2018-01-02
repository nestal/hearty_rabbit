cmake_minimum_required(VERSION 2.8)
project(hearty_rabbit)

set(Boost_USE_STATIC_LIBS  ON)
find_package(Boost COMPONENTS system filesystem program_options REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(SYSTEMD libsystemd)

set(CMAKE_CXX_STANDARD 17)
set(VERSION "0.1")

include_directories(${Boost_INCLUDE_DIR} ${SYSTEMD_INCLUDE_DIRS})
add_definitions(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

message(STATUS "found systemd = ${SYSTEMD_LIBRARIES}")

file(GLOB MAIN_SRC src/*.cc src/*.hh)

add_executable(hearty_rabbit ${MAIN_SRC})
target_link_libraries(hearty_rabbit ${Boost_LIBRARIES} ${SYSTEMD_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# Post-processing
configure_file(systemd/hearty_rabbit.service.in hearty_rabbit.service)

# Installation
install(TARGETS hearty_rabbit DESTINATION bin)
install(FILES ${CMAKE_BINARY_DIR}/hearty_rabbit.service DESTINATION etc/systemd/system)
install(FILES etc/hearty_rabbit.conf DESTINATION etc)
install(DIRECTORY lib DESTINATION lib/hearty_rabbit)

set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME "hearty-rabbit")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "Nestal Wan")
set(CPACK_PACKAGE_VENDOR "nestal.net")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_AUTOREQ ON)
set(CPACK_RPM_PACKAGE_RELEASE_DIST ON)
include(CPack)
