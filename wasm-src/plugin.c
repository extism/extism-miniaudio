#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_DEVICE_IO
#define MA_NO_THREADING
#define MA_NO_ENCODING
#define MA_NO_RUNTIME_LINKING
#define DR_FLAC_BUFFER_SIZE 65536
#include "miniaudio.h"
#include "extism-pdk.h"

static char *extism_input_load_string(void)
{
  uint64_t buf_size = extism_input_length() + 1;
  char *sz = malloc(buf_size);
  if (!sz)
  {
    return NULL;
  }
  extism_load_input((uint8_t *)sz, buf_size - 1);
  sz[buf_size - 1] = '\0';
  return sz;
}

int32_t EXTISM_EXPORTED_FUNCTION(download_audio)
{
  char *url = extism_input_load_string();
  if (!url)
  {
    return -1;
  }
  const size_t url_len = strlen(url);
  static char get_req_prefix[] = "{\"method\": \"GET\",\"url\": \"";
  static char get_req_suffix[] = "\"}";
  char *reqStr = malloc(sizeof(get_req_prefix) + url_len + sizeof(get_req_suffix) - 1);
  if (!reqStr)
  {
    free(url);
    return -1;
  }
  mempcpy(mempcpy(mempcpy(reqStr, get_req_prefix, sizeof(get_req_prefix) - 1), url, url_len), get_req_suffix, sizeof(get_req_suffix));
  free(url);
  ExtismPointer req = extism_alloc_string(reqStr, strlen(reqStr));
  free(reqStr);
  ExtismPointer res = extism_http_request(req, 0);
  if (extism_http_status_code() != 200)
  {
    return -1;
  }
  extism_output_set(res, extism_length(res));
  return 0;
}

static ma_decoder Decoder;
static void *PData;
static ma_format Format;
static uint32_t Channels;
static uint32_t SampleRate;
static void *POutData;
static size_t OldSize;

#define WRITE32LE(P, V)                        \
  ((P)[0] = (0x00000000000000FF & (V)) >> 000, \
   (P)[1] = (0x000000000000FF00 & (V)) >> 010, \
   (P)[2] = (0x0000000000FF0000 & (V)) >> 020, \
   (P)[3] = (0x00000000FF000000 & (V)) >> 030, (P) + 4)

int32_t EXTISM_EXPORTED_FUNCTION(decoder_extism_init_memory)
{
  size_t dataSize = extism_input_length();
  PData = malloc(dataSize);
  if (!PData)
  {
    return -1;
  }
  extism_load_input((uint8_t *)PData, dataSize);
  ma_decoder_config decC = ma_decoder_config_init(ma_format_s16, 0, 0);
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
  uint64_t framesRead;
  if (MA_SUCCESS != ma_decoder_read_pcm_frames(&Decoder, POutData, frameCount, &framesRead))
  {
    return -1;
  }
  const uint64_t realOutputSize = framesRead * Channels * ma_get_bytes_per_sample(Format);
  ExtismPointer ep = extism_alloc_string((const char *)POutData, realOutputSize);
  extism_output_set(ep, realOutputSize);
  return 0;
}

int32_t EXTISM_EXPORTED_FUNCTION(decoder_extism_nop)
{
  return 0;
}
