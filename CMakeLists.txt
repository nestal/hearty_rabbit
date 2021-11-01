# cmake 3.14 is required for FetchContent
cmake_minimum_required(VERSION 3.14)
project(hearty_rabbit)

set(VERSION "0.5-pre")

# Get build number from environment variable. It should be set by travis.
if (NOT BUILD_NUMBER)
	string(TIMESTAMP BUILD_NUMBER "%Y%m%d%H%M%S")
endif()
message(STATUS "Building HeartyRabbit ${VERSION}.${BUILD_NUMBER}")

set(CMAKE_CXX_STANDARD 20)
set(CONFIG_FILENAME "hearty_rabbit.json")

# OpenCV haarcascade model for unit test
message(STATUS "using haarcascade at ${HAARCASCADE_PATH}")

include(FetchContent)
include(thirdparty/thirdparty.cmake)

include_directories(${CMAKE_BINARY_DIR})

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

add_subdirectory(src)
add_subdirectory(testing)

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
