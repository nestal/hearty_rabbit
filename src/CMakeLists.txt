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
add_library(hrbcommon STATIC
	crypto/Password.cc
	crypto/Password.hh
	crypto/Blake2.cc
	crypto/Blake2.hh
	crypto/Random.cc
	crypto/Random.hh
	util/Cookie.cc
	util/Cookie.hh
	util/Error.cc
	util/Error.hh
	util/Escape.cc
	util/Escape.hh
	util/FS.cc
	util/FS.hh
	util/JSON.hh
	util/Magic.cc
	util/Magic.hh
	util/MMap.cc
	util/MMap.hh
	util/RepeatingTuple.hh
	util/StringFields.hh
	util/Timestamp.cc
	util/Timestamp.hh
	util/Overload.hh
	util/Exception.cc
	util/Exception.hh
	image/ImageContent.cc
	image/ImageContent.hh
	util/Backtrace.cc
	util/Backtrace.hh
	URLIntent.cc
	URLIntent.hh
	UserID.cc
	UserID.hh
	HeartyRabbit.hh
	SessionID.hh
)
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
target_include_directories(hrbcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

###################################################################################################
# server library
# all server side codes are built as a library for unit test
###################################################################################################

add_library(hrbsrv STATIC ${CMAKE_BINARY_DIR}/config.hh
	server/Authentication.cc
	server/Authentication.ipp
	server/Authentication.hh
	server/HeartyRabbitServer.cc
	server/HeartyRabbitServer.hh
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
	server/util/StringTemplate.cc
	server/util/StringTemplate.hh
)
target_link_libraries(hrbsrv PUBLIC
	hrbcommon
	PkgConfig::HIREDIS
	OpenSSL::SSL
	OpenSSL::Crypto
	nlohmann_json::nlohmann_json
	OpenCV::OpenCV
	${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(hrbsrv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/server)
target_compile_definitions(hrbsrv PUBLIC -DBOOST_BEAST_USE_STD_STRING_VIEW)

###################################################################################################
# client library
###################################################################################################

add_library(hrbclient STATIC
	client/HeartyRabbitClient.cc
	client/HeartyRabbitClient.hh
	client/http/GenericHTTPRequest.cc
	client/http/GenericHTTPRequest.hh
	client/http/RequestScheduler.cc
	client/http/RequestScheduler.hh
)
target_link_libraries(hrbclient PUBLIC
	hrbcommon
	OpenSSL::SSL
	OpenSSL::Crypto
	${CMAKE_THREAD_LIBS_INIT}
)
target_include_directories(hrbclient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/client)
target_compile_definitions(hrbclient PUBLIC -DBOOST_BEAST_USE_STD_STRING_VIEW)

###################################################################################################
# Main server executable
###################################################################################################

add_executable(hearty_rabbit server/main.cc)
target_link_libraries(hearty_rabbit PRIVATE hrbsrv)
add_dependencies(hearty_rabbit unittest)

if (BUILD_CLIENT)
	add_subdirectory(client/qt)
endif()
