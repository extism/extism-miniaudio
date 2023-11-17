#include <iostream>
#include "extism.hpp"

int main(int argc, char *argv[])
{
    const std::string code = "wasm-src/plugin.wasm";
    extism::Manifest manifest = extism::Manifest::wasmPath(code);
    extism::Plugin plugin(manifest, true);
    extism::Buffer buf = plugin.call("count_vowels", "this is a test");
    std::cout << static_cast<std::string>(buf) << std::endl;
}
