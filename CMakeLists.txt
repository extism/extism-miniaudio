project(Extism-Miniaudio)
cmake_minimum_required(VERSION 3.16.3)

option(EXTISM_CPP_BUILD_IN_TREE "Set to ON to build with submodule deps" ON)
if(EXTISM_CPP_BUILD_IN_TREE)
  message("EXTISM_CPP_BUILD_IN_TREE (Extism-Miniaudio): using deps from parent directory")
  add_subdirectory(third_party/cpp-sdk)
else()
  find_package(extism-cpp)
  if(extism-cpp_FOUND)
    message("Found extism-cpp")
  else()
    message("Did not find extism-cpp, using FetchContent")
    include(FetchContent)
    FetchContent_Declare(
      extism-cpp
      GIT_REPOSITORY https://github.com/extism/cpp-sdk.git
      GIT_TAG main
    )
    FetchContent_MakeAvailable(extism-cpp)
  endif()
endif()
add_executable(extism-miniaudio-example src/main.cpp)
target_link_libraries(extism-miniaudio-example extism-cpp)

add_subdirectory(wasm-src)
