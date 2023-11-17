project(Extism-Miniaudio)
cmake_minimum_required(VERSION 3.22)

option(EXTISM_CPP_BUILD_IN_TREE "Set to ON to build with submodule deps" ON)
add_subdirectory(third_party/cpp-sdk)
add_executable(extism-miniaudio-example src/main.cpp)
target_link_libraries(extism-miniaudio-example extism-cpp)

add_subdirectory(wasm-src)
