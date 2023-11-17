#include <iostream>
#include "extism.hpp"

int main(int argc, char *argv[])
{
    const std::string code = "wasm-src/plugin.wasm";
    extism::Manifest manifest = extism::Manifest::wasmPath(code);
    manifest.allowHost("jsonplaceholder.typicode.com");
    extism::Plugin plugin(manifest, true);
    extism::Buffer buf = plugin.call("count_vowels", "this is a test");
    std::cout << static_cast<std::string>(buf) << std::endl;
    extism::Buffer buf2 = plugin.call("download_audio", "");
    std::cout << static_cast<std::string>(buf2) << std::endl;
}
