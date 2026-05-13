# Playdate Lua Extension Feasibility Study

## Status

Feasibility study, based on the current `lib-say` codebase and Playdate SDK 3.0.6.

The SDK archive used for header-level inspection was downloaded from Panic's official Linux SDK distribution into `build/playdate-sdk-study/PlaydateSDK-3.0.6.tar.gz`. No SDK installer was run.

## Executive Summary

Porting `lib-say` to Playdate as a Lua-facing native extension is feasible, including playback through Playdate's native audio mixer in both the Simulator and device builds.

The main caveat is packaging. Playdate does not support a desktop-style dynamic Lua module such as `require("say")` loading an arbitrary native shared library at runtime. Native code is packaged into each game bundle as the single Playdate extension binary (`pdex.bin` on device, an OS-specific simulator library in the `.pdx`). Therefore the realistic distribution model is:

- a reusable Playdate C extension source package or static library that developers integrate into their game build;
- a pure-Lua wrapper module, for example `Source/libsay.lua`, that calls the registered C functions;
- prebuilt simulator/device artifacts only for template projects, not as a universal runtime plugin.

For pure Lua Playdate games this is a practical drop-in dependency. For games that already use native C code, it must be merged into their existing `pdex` build because a `.pdx` bundle can only contain one native extension binary.

## Current `lib-say` Baseline

`lib-say` is already close to the audio payload Playdate needs:

- core API: `say_synthesize()` returns heap-owned mono `int16_t` samples;
- output format: fixed 44,100 Hz is enforced by `say_prepare_pipeline()`;
- existing Lua binding: `say.synthesize(input, options?) -> blob, info`;
- current desktop extension binary size on Windows: `say.dll` is about 110 KB, excluding `lua54.dll`;
- source footprint in `src/`: about 297 KB across 13 files.

The existing `src/say_lua.c` is not directly portable to Playdate. It includes `lua.h` and `lauxlib.h`, creates Lua userdata with the standard Lua C API, and exports `luaopen_say`. Playdate extensions instead register functions and classes through `PlaydateAPI->lua` during the `kEventInitLua` event. The SDK header `pd_api_lua.h` typedefs `lua_State` as an opaque `void*` for extension callbacks and exposes wrapper methods such as `addFunction`, `registerClass`, `getArgString`, `pushObject`, and `pushBytes`.

## Playdate SDK Findings

### Native Extension Model

Playdate's C API supports two native use cases:

- extending the Lua runtime with C functions/classes;
- replacing Lua entirely with a C update loop.

For this project, only the first model is appropriate. The extension should avoid `playdate->system->setUpdateCallback()` so the host game remains a normal Lua game.

Initialization must be handled through:

```c
int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
    if (event == kEventInitLua) {
        /* register lib-say functions/classes here */
    }
    return 0;
}
```

The extension must be built as:

- Simulator: platform-native shared library copied into the `.pdx`;
- Device: ARM `pdex.bin` built with the SDK's `C_API/buildsupport/arm.cmake`;
- Windows Simulator: 64-bit native binary, per the SDK requirements.

### Lua API Constraints

Playdate's Lua extension API is not the full Lua C API:

- arguments are read through `playdate->lua->getArg*()`;
- return values are pushed through `playdate->lua->push*()`;
- custom objects are registered with `registerClass()` and returned with `pushObject()`;
- there is no direct `luaL_newlib()` or `luaopen_say` flow.

This means `src/say_lua.c` should be treated as the desktop binding only. A new binding, for example `src/playdate/say_playdate.c`, should be written around `PlaydateAPI`.

### Audio API Fit

The SDK exposes two viable native playback paths.

Preferred v1 path: synthesize complete speech into memory, wrap it as an `AudioSample`, and play it with a `SamplePlayer`.

```c
AudioSample* sample = pd->sound->sample->newSampleFromData(
    (uint8_t*)samples,
    kSound16bitMono,
    44100,
    (int)(sample_count * sizeof(int16_t)),
    1
);

SamplePlayer* player = pd->sound->sampleplayer->newPlayer();
pd->sound->sampleplayer->setSample(player, sample);
pd->sound->sampleplayer->play(player, 1, 1.0f);
```

This is the simplest route and it uses Playdate's normal audio mixer, volume, pause, and finish-callback behavior.

Alternate path: use `playdate->sound->addSource()` or `channel->addCallbackSource()` with an `AudioSourceFunction`. This is better for future low-latency or streaming synthesis, but it requires a ring buffer or incremental renderer. The current `lib-say` engine renders whole buffers, so this should not be the first implementation target.

## Proposed Playdate API

The Lua-facing API should be idiomatic for Playdate rather than a byte-blob interop API.

```lua
import "libsay"

local voice = libsay.speak("Hello from Playdate.", {
    language = "en",
    gain = 1.5,
})

voice:setVolume(0.8)
voice:play()
```

Recommended v1 surface:

- `libsay.synthesize(text, options) -> speech`
- `libsay.speak(text, options) -> speech`, same as synthesize and immediately starts playback
- `speech:play(repeatCount?, rate?)`
- `speech:stop()`
- `speech:isPlaying() -> boolean`
- `speech:setVolume(left, right?)`
- `speech:getDuration() -> seconds`
- `speech:getInfo() -> table`
- `speech:free()` or rely on object finalization if class deallocation support is sufficient
- `libsay.debugReport(text, options) -> string`, optional debug build feature

Options should preserve current names where they make sense:

- `language` / `lang`: `"en"` or `"fr"`
- `phonemes`: boolean
- `gain`: number > 0
- `phone`: boolean
- `frame_ms`: 5..10
- `centralization_pct`, `articulation_pct`, `voice_formants_pct`, `voice_pitch_pct`

`format`, `GetData()`, and `GetSize()` should not be part of the primary Playdate API because Playdate can consume the sample directly. A low-level `synthesizeBytes()` can be added later if there is a real interop need.

## Technical Risks

### 1. Single Native Binary Per PDX

This is the largest product constraint. A game cannot load several independent native Lua extensions at runtime the way desktop Lua can. `lib-say` can be reusable, but it has to be linked into the game's native extension binary.

Impact:

- pure Lua games can adopt a template project easily;
- native-heavy games need build integration;
- distribution should emphasize source/static-library integration, not binary-only installation.

### 2. Memory Use

Playdate has 16 MB RAM. A complete 44.1 kHz mono 16-bit sample uses about 88.2 KB per second. The current synthesizer also allocates an internal `double` mix buffer before producing the final `int16_t` samples, so peak synthesis memory is roughly:

```text
sample_count * (sizeof(double) + sizeof(int16_t))
```

That is about 441 KB per second of generated speech before Playdate's `AudioSample` ownership/copy behavior is considered.

Observed desktop examples:

- `"Hello from Playdate Lua."`: 93,712 samples, about 2.13 seconds, about 183 KB final PCM;
- `"Bonjour depuis la Playdate."`: 99,225 samples, about 2.25 seconds, about 194 KB final PCM.

Short prompts and UI barks are safe. Long narration should be capped, chunked, or moved to an offline asset-generation workflow.

### 3. CPU Cost And Floating Point

The synthesis code uses many `double` operations and math functions (`sin`, `cos`, `pow`, `tanh`). The Playdate hardware CPU is much slower than a desktop simulator, and the SDK documentation explicitly calls out floating point math as a performance area to watch.

Initial strategy:

- synthesize only on demand, never from the audio callback;
- prefer short phrases;
- cache generated `AudioSample` objects where reuse is likely;
- profile on device before committing to runtime generation in gameplay-critical moments.

Potential optimization after prototype:

- convert hot DSP paths from `double` to `float`;
- precompute filter coefficients and phoneme tables;
- remove AIFF/WAV encoders from the Playdate build;
- add a hard max duration or max sample count;
- consider streaming only if complete-buffer synthesis proves too memory-heavy.

### 4. C Library And Allocator Use

The core uses `malloc`, `free`, `memcpy`, `memset`, `vsnprintf`, and math library calls. That is acceptable for a C extension, but the Playdate build should centralize allocation behind a small adapter if possible:

- desktop: `malloc` / `free`;
- Playdate: `pd->system->realloc` where ownership crosses SDK APIs.

File output code in `say_audio_io.c` should be excluded from the Playdate runtime target unless explicitly needed for debug builds. The device extension should only need synthesis, gain/filter post-processing, and conversion into `AudioSample`.

### 5. Existing Lua Binding Incompatibility

The current desktop binding's blob userdata model is useful for generic Lua hosts, but it is not the right abstraction on Playdate. A separate binding should be maintained.

Recommended split:

- keep `src/say_lua.c` for standard Lua 5.4 desktop builds;
- add `src/playdate/say_playdate.c` for Playdate;
- keep `say_core` shared between both;
- add `SAY_BUILD_PLAYDATE` or a separate Playdate `CMakeLists.txt`.

## Build Plan

### Repository Layout

Recommended additions:

```text
playdate/
  CMakeLists.txt
  Source/
    main.lua
    pdxinfo
    libsay.lua
  src/
    say_playdate.c
```

The Playdate target should compile only:

- `src/say.c`
- `src/say_phonemes.c`
- `src/say_text.c`
- `src/say_text_en_nrl.c`
- `src/say_phonol.c`
- `src/say_prosody.c`
- `src/say_synth.c`
- `src/say_audio_io.c` only if gain/filter functions are not split out
- `playdate/src/say_playdate.c`

Longer-term, split post-processing out of `say_audio_io.c` so Playdate can omit file/container encoding completely.

### Toolchain

Use the SDK's standard CMake support:

```powershell
cmake -S playdate -B build/playdate-sim
cmake --build build/playdate-sim --config Release

cmake -S playdate -B build/playdate-device `
  --toolchain="$env:PLAYDATE_SDK_PATH/C_API/buildsupport/arm.cmake" `
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/playdate-device
```

The resulting `.pdx` should be tested in:

- Playdate Simulator on Windows;
- Playdate Simulator on at least one other desktop OS if prebuilt simulator artifacts are distributed;
- physical Playdate hardware.

## Implementation Roadmap

1. Create a minimal Playdate extension that registers `libsay.version()` during `kEventInitLua`.
2. Link `say_core` into the Playdate extension and expose `libsay.synthesizeInfo(text)` returning metadata only.
3. Allocate samples with SDK-compatible ownership and play them through `newSampleFromData()` + `SamplePlayer`.
4. Register a `libsay.speech` class wrapping `AudioSample*`, `SamplePlayer*`, and metadata.
5. Add cleanup paths for stop/free/finish callback and Lua object finalization.
6. Add a template `.pdx` sample that demonstrates `import "libsay"` from Lua.
7. Profile on hardware with short, medium, and intentionally too-long phrases.
8. Optimize memory and math only after real device measurements.

## Feasibility Verdict

The port is technically feasible with a moderate amount of work.

The strongest path is not to emulate the desktop Lua module loader. Instead, build a Playdate-native C extension that registers Lua-callable functions and returns Playdate audio objects. The current core synthesizer is already self-contained and emits the right PCM shape, so the main engineering work is binding, packaging, memory ownership, and hardware profiling.

Expected difficulty: medium.

Expected first playable prototype: 2 to 4 focused engineering days if the Playdate SDK and ARM toolchain are already installed.

Expected production-ready library: 1 to 2 weeks, mostly for device profiling, memory limits, packaging docs, and robust cleanup behavior.

Main blocker: Playdate's one-native-extension-per-game model. This does not prevent distribution to Lua developers, but it changes the product from a universal binary Lua module into a reusable Playdate build dependency.

## Sources

- Panic, Playdate developer page, SDK 3.0.6 download links: https://play.date/dev/
- Panic, Inside Playdate with C 3.0.6: https://sdk.play.date/3.0.6/Inside%20Playdate%20with%20C.html
- Panic, Playdate SDK 3.0.6 headers inspected locally from `PlaydateSDK-3.0.6.tar.gz`: `pd_api_lua.h`, `pd_api_sound.h`, `pd_api_sys.h`, `C_API/buildsupport/arm.cmake`
- Panic, Playdate hardware specs: https://play.date/
- Local code inspected: `src/say.h`, `src/say.c`, `src/say_lua.c`, `src/say_audio_io.c`, `src/say_synth.c`, `CMakeLists.txt`, `documentation/lua.md`
