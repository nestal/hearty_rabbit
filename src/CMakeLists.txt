set(Boost_USE_STATIC_LIBS  ON)
find_package(Boost REQUIRED COMPONENTS system program_options)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Doxygen)

pkg_check_modules(HIREDIS REQUIRED IMPORTED_TARGET hiredis)
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
	nlohmann_json::nlohmann_json
	OpenCV::OpenCV
	Blake2::Blake2
	${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(hrbcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

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
	nlohmann_json::nlohmann_json
	OpenCV::OpenCV
	Blake2::Blake2
	${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(hrbsrv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/server)

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
	${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(hrbclient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/client)

###################################################################################################
# Main server executable
###################################################################################################

add_executable(hearty_rabbit server/main.cc)
target_link_libraries(hearty_rabbit PRIVATE hrbsrv)
add_dependencies(hearty_rabbit unittest clienttest)

###################################################################################################
# hrbsync: directory sync with hearty rabbit server
###################################################################################################

file(GLOB_RECURSE HRB_SYNC_SRC client/hrbsync/*.cc client/hrbsync/*.hh)
add_library(hrbsync_lib ${HRB_SYNC_SRC})
target_link_libraries(hrbsync_lib PUBLIC hrbclient)
target_include_directories(hrbsync_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/client)

add_executable(hrbsync client/hrbsync/main.cc)
target_link_libraries(hrbsync PRIVATE hrbsync_lib)

if (BUILD_CLIENT)
	add_subdirectory(client/qt)
endif()
