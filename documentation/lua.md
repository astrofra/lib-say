# Lua Binding

If the repository contains the vendored `Lua/` source tree, the build uses it automatically and produces a standard Lua 5.4 runtime plus the `say` module.

If you need to target another Lua runtime, disable the bundled one with `-DSAY_USE_BUNDLED_LUA=OFF` and pass `LUA_INCLUDE_DIR` / `LUA_LIBRARY` explicitly. For an embedded host, those paths should target the same Lua runtime as the host.

## Loading The Module

Windows example:
```lua
package.cpath = package.cpath .. ";./bin/lua/?.dll"
local say = require("say")
```

On Windows, keep `say.dll` and `lua54.dll` together in the same directory, or otherwise ensure `lua54.dll` is discoverable by the loader.

## API

### `say.synthesize(input, options?) -> blob, info`

Generates audio in memory and returns:

- `blob`: a userdata owning the encoded bytes
- `info`: a table describing the encoded payload

Supported options:

- `lang` or `language`: `"en"` or `"fr"`
- `sample_rate` or `rate`: `44100` (default)
- `frame_ms`: integer between `5` and `10`
- `phonemes`: boolean, interpret `input` as phoneme symbols
- `format`: `"raw"`, `"aiff"`, or `"wav"` (`"raw"` by default)
- `gain`: number > 0, post-synthesis linear gain with a soft-knee limiter. `1.0` (default) is unchanged; `2.0` ≈ +6 dB; values up to ~3 are useful for cutting through music with no audible clipping. The knee is fixed at -3 dBFS — below that the response is purely linear, above it a tanh curve bends peaks toward the ceiling instead of square-clipping. Practical range is roughly 1.0–5.0; past that the saturator is doing real compression and the timbre starts to get squashed.
- `phone`: boolean, post-synthesis "remote phone call" filter. Tight 500–2800 Hz bandpass with 24 dB/oct skirts, a +8 dB resonance peak at ~1700 Hz to mimic an earpiece capsule, and a hard tanh saturator that stands in for low-bitrate codec compression. When combined with `gain`, the phone filter runs first so the gain stage shapes the already-bandlimited signal.

Example:
```lua
local say = require("say")

local blob, info = say.synthesize("Hello from Lua", {
    lang = "en",
    format = "aiff"
})

print(info.format, info.sample_count, info.duration_seconds)
print(blob:GetSize())
```

Cutting through music with the soft-knee gain pass:
```lua
local blob, info = say.synthesize("Stand by for incoming transmission.", {
    lang  = "en",
    format = "wav",
    gain  = 2.0,                -- ~+6 dB, lands just under full-scale, no clipping
})
```

Telephone-call effect:
```lua
local blob, info = say.synthesize("Allo, ne quittez pas.", {
    lang  = "fr",
    format = "wav",
    phone = true,               -- 500-2800 Hz bandpass + earpiece resonance + codec-style saturation
})
```

Blob methods:

- `blob:GetData()`: returns the raw pointer as an integer (`intptr_t` style)
- `blob:GetSize()`: returns the byte size

For `format = "raw"`, the blob contains mono 16-bit PCM little-endian samples (`info.pcm_encoding == "s16le"`).

For `format = "aiff"`, the blob contains a complete AIFF file payload.

For `format = "wav"`, the blob contains a complete WAV/RIFF mono 16-bit PCM payload.

### `say.debug_report(input, options?) -> report`

Builds the same debug report as the CLI and returns it as a Lua string.

### `say.default_options() -> table`

Returns the module defaults as a Lua table.

## Pointer-Based Interop

The `say` module is intentionally generic: it does not depend on any specific consumer library. The intended integration model is:

- call `say.synthesize(...)`
- retrieve the pointer with `blob:GetData()`
- retrieve the byte size with `blob:GetSize()`
- pass both values to another Lua extension that knows how to consume raw memory

Illustrative example:

```lua
local say = require("say")

local blob = select(1, say.synthesize("Hello", {format = "raw"}))
local ptr = blob:GetData()
local size = blob:GetSize()

-- Example pattern for an external extension:
-- consumer.accept_buffer(ptr, size)
```

If the target API only borrows the pointer, you must keep `blob` alive for at least as long as that external object uses the memory. If the target API copies the memory immediately, prefer that copy path.
