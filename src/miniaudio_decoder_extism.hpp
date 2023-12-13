/*
This implements a data source that decodes streams via ma_decoder in Extism

This object can be plugged into any `ma_data_source_*()` API and can also be used as a custom
decoding backend. See the custom_decoder example.

You need to include this file after miniaudio.h.
*/
#ifndef miniaudio_decoder_extism_h
#define miniaudio_decoder_extism_h

#include "extism.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        ma_data_source_base ds; /* The extism decoder can be used independently as a data source. */
        ma_read_proc onRead;
        ma_seek_proc onSeek;
        ma_tell_proc onTell;
        void *pReadSeekTellUserData;
        ma_format format;
        uint32_t channels;
        uint32_t sampleRate;
        extism::Plugin *plugin;
    } ma_decoder_extism;

    MA_API ma_result ma_decoder_extism_init(ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData, const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks, ma_decoder_extism *pExtism);
    MA_API ma_result ma_decoder_extism_init_file(const char *pFilePath, const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks, ma_decoder_extism *pExtism);
    MA_API ma_result ma_decoder_extism_init_memory(void *pData, size_t dataSize, const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks, ma_decoder_extism *pExtism);
    MA_API void ma_decoder_extism_uninit(ma_decoder_extism *pExtism, const ma_allocation_callbacks *pAllocationCallbacks);
    MA_API ma_result ma_decoder_extism_read_pcm_frames(ma_decoder_extism *pExtism, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead);
    MA_API ma_result ma_decoder_extism_seek_to_pcm_frame(ma_decoder_extism *pExtism, ma_uint64 frameIndex);
    MA_API ma_result ma_decoder_extism_get_data_format(ma_decoder_extism *pExtism, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate, ma_channel *pChannelMap, size_t channelMapCap);
    MA_API ma_result ma_decoder_extism_get_cursor_in_pcm_frames(ma_decoder_extism *pExtism, ma_uint64 *pCursor);
    MA_API ma_result ma_decoder_extism_get_length_in_pcm_frames(ma_decoder_extism *pExtism, ma_uint64 *pLength);

#ifdef __cplusplus
}
#endif
#endif

#if defined(MINIAUDIO_IMPLEMENTATION) || defined(MA_IMPLEMENTATION)
#include <chrono>
#include <iostream>
#include "miniaudio_decoder_extism_wasm.h"

#define READ32LE(S)                                                      \
    ((uint32_t)(255 & (S)[3]) << 030 | (uint32_t)(255 & (S)[2]) << 020 | \
     (uint32_t)(255 & (S)[1]) << 010 | (uint32_t)(255 & (S)[0]) << 000)

#define READ64LE(S)                                                      \
    ((uint64_t)(255 & (S)[7]) << 070 | (uint64_t)(255 & (S)[6]) << 060 | \
     (uint64_t)(255 & (S)[5]) << 050 | (uint64_t)(255 & (S)[4]) << 040 | \
     (uint64_t)(255 & (S)[3]) << 030 | (uint64_t)(255 & (S)[2]) << 020 | \
     (uint64_t)(255 & (S)[1]) << 010 | (uint64_t)(255 & (S)[0]) << 000)

#define WRITE64LE(P, V)                          \
    ((P)[0] = (0x00000000000000FF & (V)) >> 000, \
     (P)[1] = (0x000000000000FF00 & (V)) >> 010, \
     (P)[2] = (0x0000000000FF0000 & (V)) >> 020, \
     (P)[3] = (0x00000000FF000000 & (V)) >> 030, \
     (P)[4] = (0x000000FF00000000 & (V)) >> 040, \
     (P)[5] = (0x0000FF0000000000 & (V)) >> 050, \
     (P)[6] = (0x00FF000000000000 & (V)) >> 060, \
     (P)[7] = (0xFF00000000000000 & (V)) >> 070, (P) + 8)

static ma_result ma_decoder_extism_ds_read(ma_data_source *pDataSource, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead)
{
    return ma_decoder_extism_read_pcm_frames((ma_decoder_extism *)pDataSource, pFramesOut, frameCount, pFramesRead);
}

static ma_result ma_decoder_extism_ds_seek(ma_data_source *pDataSource, ma_uint64 frameIndex)
{
    return ma_decoder_extism_seek_to_pcm_frame((ma_decoder_extism *)pDataSource, frameIndex);
}

static ma_result ma_decoder_extism_ds_get_data_format(ma_data_source *pDataSource, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate, ma_channel *pChannelMap, size_t channelMapCap)
{
    return ma_decoder_extism_get_data_format((ma_decoder_extism *)pDataSource, pFormat, pChannels, pSampleRate, pChannelMap, channelMapCap);
}

static ma_result ma_decoder_extism_ds_get_cursor(ma_data_source *pDataSource, ma_uint64 *pCursor)
{
    return ma_decoder_extism_get_cursor_in_pcm_frames((ma_decoder_extism *)pDataSource, pCursor);
}

static ma_result ma_decoder_extism_ds_get_length(ma_data_source *pDataSource, ma_uint64 *pLength)
{
    return ma_decoder_extism_get_length_in_pcm_frames((ma_decoder_extism *)pDataSource, pLength);
}

static ma_data_source_vtable g_ma_decoder_extism_ds_vtable =
    {
        ma_decoder_extism_ds_read,
        ma_decoder_extism_ds_seek,
        ma_decoder_extism_ds_get_data_format,
        ma_decoder_extism_ds_get_cursor,
        ma_decoder_extism_ds_get_length};

static ma_result ma_decoder_extism_init_internal(const ma_decoding_backend_config *pConfig, ma_decoder_extism *pExtism)
{
    ma_result result;
    ma_data_source_config dataSourceConfig;

    if (pExtism == NULL)
    {
        return MA_INVALID_ARGS;
    }

    MA_ZERO_OBJECT(pExtism);

    dataSourceConfig = ma_data_source_config_init();
    dataSourceConfig.vtable = &g_ma_decoder_extism_ds_vtable;

    result = ma_data_source_init(&dataSourceConfig, &pExtism->ds);
    if (result != MA_SUCCESS)
    {
        return result; /* Failed to initialize the base data source. */
    }

    pExtism->plugin = new extism::Plugin(plugin_wasm, sizeof(plugin_wasm), true);
    return MA_SUCCESS;
}

MA_API ma_result ma_decoder_extism_init(ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData, const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks, ma_decoder_extism *pExtism)
{
    ma_result result;

    (void)pAllocationCallbacks;

    result = ma_decoder_extism_init_internal(pConfig, pExtism);
    if (result != MA_SUCCESS)
    {
        return result;
    }

    if (onRead == NULL || onSeek == NULL)
    {
        return MA_INVALID_ARGS; /* onRead and onSeek are mandatory. */
    }

    pExtism->onRead = onRead;
    pExtism->onSeek = onSeek;
    pExtism->onTell = onTell;
    pExtism->pReadSeekTellUserData = pReadSeekTellUserData;

    return MA_NOT_IMPLEMENTED;
}

MA_API ma_result ma_decoder_extism_init_file(const char *pFilePath, const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks, ma_decoder_extism *pExtism)
{
    ma_result result;

    (void)pAllocationCallbacks;

    result = ma_decoder_extism_init_internal(pConfig, pExtism);
    if (result != MA_SUCCESS)
    {
        return result;
    }

    (void)pFilePath;
    return MA_NOT_IMPLEMENTED;
}

MA_API ma_result ma_decoder_extism_init_memory(const void *pData, size_t dataSize, const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks, ma_decoder_extism *pExtism)
{
    ma_result result;

    (void)pAllocationCallbacks;

    result = ma_decoder_extism_init_internal(pConfig, pExtism);
    if (result != MA_SUCCESS)
    {
        return result;
    }

    extism::Buffer buf = pExtism->plugin->call("decoder_extism_init_memory", static_cast<const uint8_t *>(pData), dataSize);
    pExtism->format = static_cast<ma_format>(READ32LE(buf.data));
    pExtism->channels = READ32LE(buf.data + 4);
    pExtism->sampleRate = READ32LE(buf.data + 8);
    extism_plugin_reset(pExtism->plugin->plugin);
    return MA_SUCCESS;
}

MA_API void ma_decoder_extism_uninit(ma_decoder_extism *pExtism, const ma_allocation_callbacks *pAllocationCallbacks)
{
    if (pExtism == NULL)
    {
        return;
    }

    (void)pAllocationCallbacks;

    if (pExtism->plugin != NULL)
    {
        pExtism->plugin->call("decoder_extism_uninit");
        delete pExtism->plugin;
    }

    ma_data_source_uninit(&pExtism->ds);
}

MA_API ma_result ma_decoder_extism_read_pcm_frames(ma_decoder_extism *pExtism, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead)
{
    const auto start{std::chrono::steady_clock::now()};
    if (pFramesRead != NULL)
    {
        *pFramesRead = 0;
    }

    if (frameCount == 0)
    {
        return MA_INVALID_ARGS;
    }

    if (pExtism == NULL)
    {
        return MA_INVALID_ARGS;
    }
    uint8_t frameCountBuf[8];
    WRITE64LE(frameCountBuf, frameCount);
    extism::Buffer out = pExtism->plugin->call("decoder_extism_read_pcm_frames", frameCountBuf, sizeof(frameCountBuf));
    memcpy(pFramesOut, out.data, out.length);
    *pFramesRead = out.length / (pExtism->channels * ma_get_bytes_per_sample(pExtism->format));

    const auto end{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{end - start};
    const double acceptable_time = (double)frameCount / pExtism->sampleRate;
    const double delta = elapsed_seconds.count() - acceptable_time;
    if (delta > 0)
    {
        std::cerr << "too long by " << delta << " acceptable " << acceptable_time << " frameCount " << frameCount << std::endl;
    }

    if (*pFramesRead == 0)
    {
        return MA_AT_END;
    }

    return MA_SUCCESS;
}

MA_API ma_result ma_decoder_extism_seek_to_pcm_frame(ma_decoder_extism *pExtism, ma_uint64 frameIndex)
{
    if (pExtism == NULL)
    {
        return MA_INVALID_ARGS;
    }

    uint8_t frameIndexBuf[8];
    WRITE64LE(frameIndexBuf, frameIndex);
    extism::Buffer out = pExtism->plugin->call("decoder_extism_seek_to_pcm_frame", frameIndexBuf, sizeof(frameIndexBuf));
    return static_cast<ma_result>(READ32LE(out.data));
}

MA_API ma_result ma_decoder_extism_get_data_format(ma_decoder_extism *pExtism, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate, ma_channel *pChannelMap, size_t channelMapCap)
{
    /* Defaults for safety. */
    if (pFormat != NULL)
    {
        *pFormat = ma_format_unknown;
    }
    if (pChannels != NULL)
    {
        *pChannels = 0;
    }
    if (pSampleRate != NULL)
    {
        *pSampleRate = 0;
    }
    if (pChannelMap != NULL)
    {
        MA_ZERO_MEMORY(pChannelMap, sizeof(*pChannelMap) * channelMapCap);
    }

    if (pExtism == NULL)
    {
        return MA_INVALID_OPERATION;
    }

    if (pFormat != NULL)
    {
        *pFormat = pExtism->format;
    }

    if (pChannels != NULL)
    {
        *pChannels = pExtism->channels;
    }

    if (pSampleRate != NULL)
    {
        *pSampleRate = pExtism->sampleRate;
    }

    return MA_SUCCESS;
}

MA_API ma_result ma_decoder_extism_get_cursor_in_pcm_frames(ma_decoder_extism *pExtism, ma_uint64 *pCursor)
{
    if (pCursor == NULL)
    {
        return MA_INVALID_ARGS;
    }

    *pCursor = 0; /* Safety. */

    if (pExtism == NULL)
    {
        return MA_INVALID_ARGS;
    }

    extism::Buffer out = pExtism->plugin->call("decoder_extism_get_cursor_in_pcm_frames");
    *pCursor = READ64LE(out.data);
    return MA_SUCCESS;
}

MA_API ma_result ma_decoder_extism_get_length_in_pcm_frames(ma_decoder_extism *pExtism, ma_uint64 *pLength)
{
    if (pLength == NULL)
    {
        return MA_INVALID_ARGS;
    }

    *pLength = 0; /* Safety. */

    if (pExtism == NULL)
    {
        return MA_INVALID_ARGS;
    }

    extism::Buffer out = pExtism->plugin->call("decoder_extism_get_length_in_pcm_frames");
    *pLength = READ64LE(out.data);
    return MA_SUCCESS;
}

#endif
