#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define EXTISM_IMPLEMENTATION
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
