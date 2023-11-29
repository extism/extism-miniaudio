#include <stdio.h>
#include <string.h>

#include "miniaudio.h"
#include "extism-pdk.h"

EXTISM_EXPORT_AS("count_vowels")
int32_t count_vowels(void)
{
  uint64_t count = 0;
  uint8_t ch = 0;
  uint64_t length = extism_input_length();

  for (uint64_t i = 0; i < length; i++)
  {
    ch = extism_input_load_u8(i);
    count += (ch == 'A') + (ch == 'a') + (ch == 'E') + (ch == 'e') +
             (ch == 'I') + (ch == 'i') + (ch == 'O') + (ch == 'o') +
             (ch == 'U') + (ch == 'u');
  }

  char out[128];
  int n = snprintf(out, 128, "{\"count\": %llu}", count);

  uint64_t offs_ = extism_alloc(n);
  extism_store(offs_, (const uint8_t *)out, n);
  extism_output_set(offs_, n);

  return 0;
}

int32_t EXTISM_EXPORTED_FUNCTION(download_audio)(void)
{
  const char *reqStr = "{\
    \"method\": \"GET\",\
    \"url\": \"https://jsonplaceholder.typicode.com/todos/1\"\
  }";

  ExtismPointer req = extism_alloc_string(reqStr, strlen(reqStr));
  ExtismPointer res = extism_http_request(req, 0);

  if (extism_http_status_code() != 200)
  {
    return -1;
  }

  extism_output_set(res, extism_length(res));
  return 0;
}