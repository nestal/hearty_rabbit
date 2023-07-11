###################################################################################################
# server library
# all server side codes are built as a library for unit test
###################################################################################################

set(SERVER_SRC
	hrb/RequestTarget.hh
	hrb/RequestTarget.cc
	hrb/Server.hh
	hrb/Server.cc
	hrb/SessionHandler.hh
	hrb/SessionHandler.cc
	hrb/SessionHandler.ipp
	hrb/UploadFile.hh
	hrb/UploadFile.cc
	util/Configuration.hh
	util/Configuration.cc
	util/Backtrace.hh
	util/Backtrace.cc
	util/Exception.hh
	util/Exception.cc
	util/Log.hh
	util/Log.cc
	util/StringTemplate.hh
	util/StringTemplate.cc
	net/Listener.hh
	net/Listener.cc
	net/Session.hh
	net/Session.cc
	net/InsecureSession.hh
	net/InsecureSession.cc
	net/MMapResponseBody.hh
	net/MMapResponseBody.cc
	net/Redis.hh
	net/Redis.cc
	net/SplitBuffers.hh
	net/SplitBuffers.cc
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
target_include_directories(hrbsrv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(hrbsrv PUBLIC -DBOOST_BEAST_USE_STD_STRING_VIEW)
