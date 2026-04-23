# lib-say
Say It. Experimental formant-based TTS

## Build

Windows batch helper:
```bat
build.bat
```

Direct CMake:
```bat
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The executable is written to `bin\tts.exe`.

## Lua Extension

If the repository contains `Lua/` with the vendored Lua sources, CMake uses it automatically and builds:

- `bin\lua\lua54.dll`
- `bin\lua\say.dll`

If you prefer an external Lua SDK instead, disable the bundled one:
```bat
cmake -S . -B build -DSAY_USE_BUNDLED_LUA=OFF -DLUA_INCLUDE_DIR=C:\path\to\lua\include -DLUA_LIBRARY=C:\path\to\lua54.lib
```

For an embedded host, those variables should point to the same Lua runtime the host itself uses.

If `Lua/` is absent and auto-detection fails, configure CMake with the exact Lua SDK used by the host application that will load the module:
```bat
cmake -S . -B build -DLUA_INCLUDE_DIR=C:\path\to\lua\include -DLUA_LIBRARY=C:\path\to\lua54.lib
```

Typical usage:
```lua
package.cpath = package.cpath .. ";./bin/lua/?.dll"

local say = require("say")
local blob, info = say.synthesize("Bonjour depuis Lua", {
    lang = "fr",
    format = "raw"
})

print(info.sample_rate, info.byte_count, blob:GetSize())
```

The returned `blob` is a Lua userdata that owns the encoded bytes and exposes `GetData()` and `GetSize()`. It is meant to be passed to another Lua extension through a pointer-and-size style API. More details are in `documentation/lua.md`.
