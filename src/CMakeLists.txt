include_directories(commands)
include_directories(maj)

file(GLOB_RECURSE FILENAMES *.cpp)

find_package( Eigen3 3.3 REQUIRED NO_MODULE )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(also also.cpp ${FILENAMES})
target_link_libraries(also alice mockturtle Eigen3::Eigen)
