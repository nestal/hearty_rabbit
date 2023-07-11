add_library(Catch2::Catch2 INTERFACE IMPORTED)
set_target_properties(Catch2::Catch2 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/thirdparty")

add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
set_target_properties(nlohmann_json::nlohmann_json PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/thirdparty")

find_package(OpenCV REQUIRED core imgproc imgcodecs objdetect highgui)

add_library(OpenCV::OpenCV INTERFACE IMPORTED)
set_target_properties(OpenCV::OpenCV PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_INCLUDE_DIRS}")
set_target_properties(OpenCV::OpenCV PROPERTIES INTERFACE_LINK_LIBRARIES "${OpenCV_LIBS}")
