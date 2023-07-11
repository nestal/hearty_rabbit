set(Boost_USE_STATIC_LIBS  ON)
find_package(Boost REQUIRED COMPONENTS system program_options)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Doxygen)

pkg_check_modules(HIREDIS REQUIRED IMPORTED_TARGET hiredis)
pkg_check_modules(LIBEXIF REQUIRED IMPORTED_TARGET libexif)

# V2 code:
add_library(
	hrb2_common
	common/util/Escape.hh
	common/util/Escape.cc
)
target_include_directories(hrb2_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)
add_library(
	hrb2
	server/hrb/RequestTarget.hh
	server/hrb/RequestTarget.cc
)
target_include_directories(hrb2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/server)
target_link_libraries(hrb2 PUBLIC hrb2_common OpenSSL::Crypto)

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
	PkgConfig::LIBEXIF
	${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(hrbcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

###################################################################################################
# server library
# all server side codes are built as a library for unit test
###################################################################################################

set(SERVER_SRC
	server/hrb/RequestTarget.hh
	server/hrb/RequestTarget.cc
	server/hrb/Server.hh
	server/hrb/Server.cc
	server/hrb/SessionHandler.hh
	server/hrb/SessionHandler.cc
	server/hrb/SessionHandler.ipp
	server/hrb/UploadFile.hh
	server/hrb/UploadFile.cc
	server/util/Configuration.hh
	server/util/Configuration.cc
	server/util/Backtrace.hh
	server/util/Backtrace.cc
	server/util/Exception.hh
	server/util/Exception.cc
	server/util/Log.hh
	server/util/Log.cc
	server/util/StringTemplate.hh
	server/util/StringTemplate.cc
	server/net/Listener.hh
	server/net/Listener.cc
	server/net/Session.hh
	server/net/Session.cc
	server/net/InsecureSession.hh
	server/net/InsecureSession.cc
	server/net/MMapResponseBody.hh
	server/net/MMapResponseBody.cc
	server/net/Redis.hh
	server/net/Redis.cc
	server/net/SplitBuffers.hh
	server/net/SplitBuffers.cc
)
add_library(hrbsrv ${SERVER_SRC} ${CMAKE_BINARY_DIR}/config.hh)
target_link_libraries(hrbsrv PUBLIC
	hrbcommon
	PkgConfig::HIREDIS
	OpenSSL::SSL
	OpenSSL::Crypto
	nlohmann_json::nlohmann_json
	OpenCV::OpenCV
	${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(hrbsrv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/server)
target_compile_definitions(hrbsrv PUBLIC -DBOOST_BEAST_USE_STD_STRING_VIEW)

###################################################################################################
# client library
###################################################################################################

#file(GLOB_RECURSE CLIENT_SRC client/http/*.cc client/*.hh client/http/*.ipp)
#add_library(hrbclient ${CLIENT_SRC})
#target_link_libraries(
#	hrbclient
#	hrbcommon
#	OpenSSL::SSL
#	OpenSSL::Crypto
#	${CMAKE_THREAD_LIBS_INIT}
#)
#target_include_directories(hrbclient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/client)
#target_compile_definitions(hrbclient PUBLIC -DBOOST_BEAST_USE_STD_STRING_VIEW)

###################################################################################################
# Main server executable
###################################################################################################

add_executable(hearty_rabbit server/main.cc)
target_link_libraries(hearty_rabbit PRIVATE hrbsrv)
add_dependencies(hearty_rabbit unittest)

###################################################################################################
# hrbsync: directory sync with hearty rabbit server
###################################################################################################

#file(GLOB_RECURSE HRB_SYNC_SRC client/hrbsync/*.cc client/hrbsync/*.hh)
#add_library(hrbsync_lib ${HRB_SYNC_SRC})
#target_link_libraries(hrbsync_lib PUBLIC hrbclient)
#target_include_directories(hrbsync_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/client)
#
#add_executable(hrbsync client/hrbsync/main.cc)
#target_link_libraries(hrbsync PRIVATE hrbsync_lib)
#add_dependencies(hrbsync unittest clienttest)
#
#if (BUILD_CLIENT)
#	add_subdirectory(client/qt)
#endif()
