# Extism Miniaudio

Use [miniaudio](https://github.com/mackron/miniaudio) with a decoder sandboxed in Wasm via Extism.

## Download and Configure (for building with deps in-tree)

```bash
git clone --recurse-submodules -j4 https://github.com/extism/extism-miniaudio
cd extism-miniaudio
cmake -B build
```

## OR Download and Configure (using installed extism, extism-cpp, and jsoncpp)

Install libextism and cpp-sdk as described in: [cpp-sdk building-and-installation](https://github.com/extism/cpp-sdk#building-and-installation)

```bash
git clone https://github.com/extism/extism-miniaudio
git submodule update --init third_party/extism-pdk
git submodule update --init third_party/miniaudio
cd extism-miniaudio
cmake -B build -DEXTISM_CPP_BUILD_IN_TREE=OFF
```

## Build

```bash
cmake --build build
```

## Demo example audio player

```bash
cd build
./extism-miniaudio-example <path_to_song>
```

or

```bash
cd build
./extism-miniaudio-example <url_to_song> <host_to_allow>
```

## Using `miniaudio_decoder_extism` in your projects

In one source file:

```cpp
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "miniaudio_decoder_extism.hpp"
```

In any other source file:

```cpp
#include "miniaudio.h"
#include "miniaudio_decoder_extism.hpp"
```

`miniaudio_decoder_extism_wasm.h` (generated file containing Extism plugin Wasm) must be accessible to
`miniaudio_decoder_extism.hpp`

## Questions and Feedback

Ping @G4Vi in the `#cpp-sdk` channel in the [Extism Discord](https://extism.org/discord).
