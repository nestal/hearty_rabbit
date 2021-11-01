find_package(Catch2 2.13 REQUIRED)
find_package(nlohmann_json 3.9.0 REQUIRED)

find_package(OpenCV REQUIRED core imgproc imgcodecs objdetect highgui)

add_library(OpenCV::OpenCV INTERFACE IMPORTED)
set_target_properties(OpenCV::OpenCV PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_INCLUDE_DIRS}")
set_target_properties(OpenCV::OpenCV PROPERTIES INTERFACE_LINK_LIBRARIES "${OpenCV_LIBS}")

# Find libb2
find_library(LIBB2_LIBRARY libb2.a b2 PATHS lib)
find_path(LIBB2_INCLUDE blake2.h PATHS include)
message(STATUS "found libb2 at ${LIBB2_LIBRARY} ${LIBB2_INCLUDE}")

add_library(Blake2::Blake2 INTERFACE IMPORTED)
set_target_properties(Blake2::Blake2 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBB2_INCLUDE}")
set_target_properties(Blake2::Blake2 PROPERTIES INTERFACE_LINK_LIBRARIES "${LIBB2_LIBRARY}")
