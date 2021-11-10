set(Boost_USE_STATIC_LIBS  ON)
find_package(Boost REQUIRED COMPONENTS system program_options)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL 1.1 REQUIRED)
find_package(Doxygen)
find_package(PostgreSQL REQUIRED)

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
	PostgreSQL::PostgreSQL
	PkgConfig::LIBEXIF
	${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(hrbcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

###################################################################################################
# server library
# all server side codes are built as a library for unit test
###################################################################################################

add_library(hrbsrv ${CMAKE_BINARY_DIR}/config.hh
	server/HeartyRabbitServer.cc
	server/HeartyRabbitServer.hh
	server/crypto/Password.cc
	server/crypto/Password.hh
	server/crypto/Authentication.cc
	server/crypto/Authentication.cc
	server/hrb/SessionHandler.hh
	server/hrb/SessionHandler.ipp
	server/hrb/SessionHandler.cc
	server/hrb/UploadFile.cc
	server/hrb/UploadFile.hh
	server/hrb/WebResources.cc
	server/hrb/WebResources.hh
	server/hrb/Server.cc
	server/hrb/Server.hh
	server/net/Redis.cc
	server/net/Redis.hh
	server/net/Listener.cc
	server/net/Listener.hh
	server/net/InsecureSession.cc
	server/net/InsecureSession.hh
	server/net/Session.cc
	server/net/Session.hh
	server/util/Log.cc
	server/util/Log.hh
	server/util/Configuration.cc
	server/util/Configuration.hh
	server/util/Backtrace.cc
	server/util/Backtrace.hh
	server/util/StringTemplate.cc
	server/util/StringTemplate.hh
	server/util/Exception.cc
	server/util/Exception.hh
	server/image/ImageContent.cc
	server/image/ImageContent.hh
)
target_link_libraries(hrbsrv PUBLIC
	hrbcommon
	hrb2
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
target_compile_definitions(hrbclient PUBLIC -DBOOST_BEAST_USE_STD_STRING_VIEW)

###################################################################################################
# Main server executable
###################################################################################################

add_executable(hearty_rabbit server/main.cc client/HeartyRabbitClient.cc client/HeartyRabbitClient.hh)
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
add_dependencies(hrbsync unittest clienttest)

###################################################################################################
# HRB V2
###################################################################################################
add_library(hrb2 HeartyRabbit.hh common/util/Overload.hh)
target_link_libraries(hrb2 PUBLIC hrbsrv)
target_include_directories(hrb2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(hrb2 PUBLIC -fcoroutines)

if (BUILD_CLIENT)
	add_subdirectory(client/qt)
endif()
