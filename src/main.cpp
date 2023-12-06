#include <iostream>
#include "extism.hpp"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "A URL to an audio file must be provided!" << std::endl;
        return 1;
    }
    const std::string code = "wasm-src/plugin.wasm";
    extism::Manifest manifest = extism::Manifest::wasmPath(code);
    manifest.allowHost("SET_ME");
    extism::Plugin plugin(manifest, true);
    extism::Buffer buf = plugin.call("count_vowels", "this is a test");
    std::cout << static_cast<std::string>(buf) << std::endl;
    try
    {
        extism::Buffer buf2 = plugin.call("download_audio", argv[1]);
        //std::cout << static_cast<std::string>(buf2) << std::endl;
        ma_decoder decoder;
        if (MA_SUCCESS != ma_decoder_init_memory(buf2.data, buf2.length, NULL, &decoder))
        {
            std::cerr << "error initializing decoder" << std::endl;
            return 1;
        }
        ma_engine engine;
        if (MA_SUCCESS != ma_engine_init(NULL, &engine))
        {
            std::cerr << "error initializing engine" << std::endl;
            return 1;
        }
        ma_engine_start(&engine);
        ma_sound sound;
        ma_sound_init_from_data_source(&engine, &decoder, 0, NULL, &sound);
        ma_sound_start(&sound);
        while (1)
            ;
    }
    catch (...)
    {
        std::cerr << "plugin call failed" << std::endl;
    }
}
