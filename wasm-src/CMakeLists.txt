add_executable(plugin plugin.c)
target_include_directories(plugin PRIVATE ../third_party/extism-pdk ../third_party/miniaudio)
set(CMAKE_C_COMPILER "$ENV{WASI_SDK_PATH}/bin/clang")
set(CMAKE_EXECUTABLE_SUFFIX ".wasm")
target_compile_options(plugin PRIVATE -O2 -g)
target_link_options(plugin PRIVATE -mexec-model=reactor)
