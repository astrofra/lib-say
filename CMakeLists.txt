cmake_minimum_required(VERSION 3.16)

project(lib_say LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(tts
    src/main.c
    src/say.c
    src/say.h
)

target_include_directories(tts PRIVATE src)

if(MSVC)
    target_compile_options(tts PRIVATE /W4 /O2)
else()
    target_compile_options(tts PRIVATE -Wall -Wextra -O2)
endif()

set_target_properties(tts PROPERTIES
    OUTPUT_NAME tts
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(tts PROPERTIES
        "RUNTIME_OUTPUT_DIRECTORY_${config_upper}" "${CMAKE_SOURCE_DIR}/bin"
    )
endforeach()
