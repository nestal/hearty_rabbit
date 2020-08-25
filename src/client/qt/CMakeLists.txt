# cmake 3.8 is required for C++17
cmake_minimum_required(VERSION 3.8)
project(hearty_rabbit_gui)

cmake_policy(SET CMP0100 NEW)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(Boost_USE_STATIC_LIBS  ON)
find_package(Boost REQUIRED COMPONENTS system program_options)
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Network CONFIG REQUIRED)

# Put all source code in a library for unit test
file(GLOB_RECURSE HRB_GUI_SRC *.cc *.hh *.ui)
add_executable(hearty_rabbit_gui ${HRB_GUI_SRC})
target_link_libraries(hearty_rabbit_gui hrbclient Qt5::Widgets Qt5::Network)
target_compile_definitions(hearty_rabbit_gui PUBLIC -DQT_NO_KEYWORDS)
target_include_directories(
	hearty_rabbit_gui PUBLIC
	${PROJECT_SOURCE_DIR}/../../src/client
)
