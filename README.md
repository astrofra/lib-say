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
