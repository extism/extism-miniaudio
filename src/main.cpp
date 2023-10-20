#include <iostream>
#include "extism.hpp"

int main(int argc, char *argv[])
{
    const std::string code = "third_party/cpp-sdk/wasm/code.wasm";
    extism::Manifest manifest = extism::Manifest::wasmPath(code);
    manifest.setConfig("a", "1");
    extism::Plugin plugin(manifest);
    extism::Buffer buf = plugin.call("count_vowels", "this is a test");
    std::cout << static_cast<std::string>(buf) << std::endl;
}
