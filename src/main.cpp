
#include <fstream>
#include <iostream>
#include <optional>
#include "extism.hpp"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "miniaudio_decoder_extism.h"
static const std::optional<extism::Buffer> download_audio(extism::Plugin &plugin, std::string src)
{
    try
    {
        extism::Buffer buf = plugin.call("download_audio", std::move(src));
        return buf;
    }
    catch (const extism::Error &)
    {
        return {};
    }
}

static std::vector<uint8_t> readFile(const std::string &filename)
{
    struct stat st;
    if (stat(filename.c_str(), &st) == -1)
    {
        throw std::runtime_error("error stat-ing file");
    }
    auto inSize = st.st_size;
    std::vector<uint8_t> buf(inSize);
    std::ifstream f(filename, std::ios::binary);
    f.read(reinterpret_cast<char *>(buf.data()), inSize);
    if (f.gcount() != inSize)
    {
        throw std::runtime_error("failed to read");
    }
    return buf;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "A URL to an audio file and an allowed host must be provided!" << std::endl;
        return 1;
    }
    const std::string code("wasm-src/plugin.wasm");
    extism::Manifest manifest = extism::Manifest::wasmPath(code);
    manifest.allowHost(argv[2]);
    extism::Plugin plugin(manifest, true);
    auto tempBuf = readFile(argv[1]);
    const extism::Buffer buf(tempBuf.data(), tempBuf.size());
    // const auto maybeBuf = download_audio(plugin, argv[1]);
    // if (!maybeBuf)
    //{
    //     std::cerr << "Failed to download audio!" << std::endl;
    //     return 1;
    // }
    // const extism::Buffer &buf = *maybeBuf;

    ma_decoder_extism decoder_extism;
    if (MA_SUCCESS != ma_decoder_extism_init_memory(buf.data, buf.length, NULL, NULL, &decoder_extism))
    {
        std::cerr << "error initializing decoder extism" << std::endl;
        return 1;
    }
    // ma_decoder decoder;
    // if (MA_SUCCESS != ma_decoder_init_memory(buf.data, buf.length, NULL, &decoder))
    //{
    //     std::cerr << "error initializing decoder" << std::endl;
    //     return 1;
    // }
    ma_engine engine;
    ma_engine_config config = ma_engine_config_init();
    config.periodSizeInFrames = 512;
    if (MA_SUCCESS != ma_engine_init(&config, &engine))
    {
        std::cerr << "error initializing engine" << std::endl;
        return 1;
    }
    if (MA_SUCCESS != ma_engine_start(&engine))
    {
        std::cerr << "error starting engine" << std::endl;
        return 1;
    }
    ma_sound sound;
    if (MA_SUCCESS != ma_sound_init_from_data_source(&engine, &decoder_extism, 0, NULL, &sound))
    {
        std::cerr << "error initializing sound" << std::endl;
        return 1;
    }
    if (MA_SUCCESS != ma_sound_start(&sound))
    {
        std::cerr << "error starting sound" << std::endl;
        return 1;
    }
    while (!ma_sound_at_end(&sound))
    {
        sleep(1);
    }
    ma_sound_uninit(&sound);
    ma_engine_uninit(&engine);
    // ma_decoder_uninit(&decoder);
    ma_decoder_extism_uninit(&decoder_extism, NULL);
}
