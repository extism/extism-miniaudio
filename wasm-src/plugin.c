#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_DEVICE_IO
#define MA_NO_THREADING
#define MA_NO_ENCODING
#define MA_NO_RUNTIME_LINKING
#include "miniaudio.h"
#include "extism-pdk.h"

#define WRITE32LE(P, V)                        \
  ((P)[0] = (0x00000000000000FF & (V)) >> 000, \
   (P)[1] = (0x000000000000FF00 & (V)) >> 010, \
   (P)[2] = (0x0000000000FF0000 & (V)) >> 020, \
   (P)[3] = (0x00000000FF000000 & (V)) >> 030, (P) + 4)

#define WRITE64LE(P, V)                        \
  ((P)[0] = (0x00000000000000FF & (V)) >> 000, \
   (P)[1] = (0x000000000000FF00 & (V)) >> 010, \
   (P)[2] = (0x0000000000FF0000 & (V)) >> 020, \
   (P)[3] = (0x00000000FF000000 & (V)) >> 030, \
   (P)[4] = (0x000000FF00000000 & (V)) >> 040, \
   (P)[5] = (0x0000FF0000000000 & (V)) >> 050, \
   (P)[6] = (0x00FF000000000000 & (V)) >> 060, \
   (P)[7] = (0xFF00000000000000 & (V)) >> 070, (P) + 8)

static bool HasConfig = false;
static ma_decoding_backend_config Config;
static ma_decoder Decoder;
static void *PData;
static ma_format Format;
static uint32_t Channels;
static uint32_t SampleRate;
static void *POutData;
static size_t OldSize;

int32_t EXTISM_EXPORTED_FUNCTION(decoder_extism_init_internal)
{
  if (extism_input_length() != sizeof(ma_decoding_backend_config))
  {
    return -1;
  }
  HasConfig = true;
  extism_load_input((uint8_t *)&Config, extism_input_length());
  return 0;
}

int32_t EXTISM_EXPORTED_FUNCTION(decoder_extism_init_memory)
{
  size_t dataSize = extism_input_length();
  PData = malloc(dataSize);
  if (!PData)
  {
    return -1;
  }
  extism_load_input((uint8_t *)PData, dataSize);
  ma_decoder_config decC = ma_decoder_config_init_default();
  if (HasConfig)
  {
    decC.format = Config.preferredFormat;
    decC.seekPointCount = Config.seekPointCount;
  }
  if ((MA_SUCCESS != ma_decoder_init_memory(PData, dataSize, &decC, &Decoder)) || (MA_SUCCESS != ma_decoder_get_data_format(&Decoder, &Format, &Channels, &SampleRate, NULL, 0)))
  {
    free(PData);
    return -1;
  }
  uint8_t outputConfig[12];
  WRITE32LE(outputConfig, Format);
  WRITE32LE(&outputConfig[4], Channels);
  WRITE32LE(&outputConfig[8], SampleRate);
  ExtismPointer ep = extism_alloc_string((const char *)outputConfig, sizeof(outputConfig));
  extism_output_set(ep, sizeof(outputConfig));
  return 0;
}

int32_t EXTISM_EXPORTED_FUNCTION(decoder_extism_uninit)
{
  HasConfig = false;
  const ma_result result = ma_decoder_uninit(&Decoder);
  free(PData);
  if (POutData)
  {
    free(POutData);
    POutData = NULL;
    OldSize = 0;
  }
  if (MA_SUCCESS != result)
  {
    return -1;
  }
  return 0;
}

int32_t EXTISM_EXPORTED_FUNCTION(decoder_extism_read_pcm_frames)
{
  const uint64_t frameCount = extism_input_load_u64(0);
  const size_t outputSize = frameCount * Channels * ma_get_bytes_per_sample(Format);
  if (outputSize > OldSize)
  {
    void *newOutData = realloc(POutData, outputSize);
    if (!newOutData)
    {
      return -1;
    }
    POutData = newOutData;
    OldSize = outputSize;
  }
  ma_result result;
  uint64_t framesRead;
  result = ma_decoder_read_pcm_frames(&Decoder, POutData, frameCount, &framesRead);
  if (result != MA_SUCCESS)
  {
    if (result == MA_AT_END)
    {
      return 0;
    }
    return -1;
  }
  const uint64_t realOutputSize = framesRead * Channels * ma_get_bytes_per_sample(Format);
  ExtismPointer ep = extism_alloc_string((const char *)POutData, realOutputSize);
  extism_output_set(ep, realOutputSize);
  return 0;
}

int32_t EXTISM_EXPORTED_FUNCTION(decoder_extism_seek_to_pcm_frame)
{
  const uint64_t frameIndex = extism_input_load_u64(0);
  const ma_result result = ma_decoder_seek_to_pcm_frame(&Decoder, frameIndex);

  uint8_t output[4];
  WRITE32LE(output, result);
  ExtismPointer ep = extism_alloc_string((const char *)output, sizeof(output));
  extism_output_set(ep, sizeof(output));
  return 0;
}

int32_t EXTISM_EXPORTED_FUNCTION(decoder_extism_get_cursor_in_pcm_frames)
{
  ma_uint64 cursor;
  if (MA_SUCCESS != ma_decoder_get_cursor_in_pcm_frames(&Decoder, &cursor))
  {
    return -1;
  }
  uint8_t output[8];
  WRITE64LE(output, cursor);
  ExtismPointer ep = extism_alloc_string((const char *)output, sizeof(output));
  extism_output_set(ep, sizeof(output));
  return 0;
}

int32_t EXTISM_EXPORTED_FUNCTION(decoder_extism_get_length_in_pcm_frames)
{
  ma_uint64 length;
  if (MA_SUCCESS != ma_decoder_get_length_in_pcm_frames(&Decoder, &length))
  {
    return -1;
  }
  uint8_t output[8];
  WRITE64LE(output, length);
  ExtismPointer ep = extism_alloc_string((const char *)output, sizeof(output));
  extism_output_set(ep, sizeof(output));
  return 0;
}
