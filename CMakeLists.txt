cmake_minimum_required(VERSION 3.16)

project(lib_say LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(BUILD_LUA_EXTENSION "Build the optional Lua extension module" ON)
option(SAY_USE_BUNDLED_LUA "Use the vendored Lua sources from Lua/ when available" ON)
set(SAY_BUNDLED_LUA_DIR "${CMAKE_SOURCE_DIR}/Lua" CACHE PATH "Path to bundled Lua sources")

function(say_apply_common_compile_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /O2)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -O2)
    endif()
endfunction()

function(say_set_runtime_output target dir)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${dir}"
        LIBRARY_OUTPUT_DIRECTORY "${dir}"
        ARCHIVE_OUTPUT_DIRECTORY "${dir}"
    )

    foreach(config Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER "${config}" config_upper)
        set_target_properties(${target} PROPERTIES
            "RUNTIME_OUTPUT_DIRECTORY_${config_upper}" "${dir}"
            "LIBRARY_OUTPUT_DIRECTORY_${config_upper}" "${dir}"
            "ARCHIVE_OUTPUT_DIRECTORY_${config_upper}" "${dir}"
        )
    endforeach()
endfunction()

add_library(say_core STATIC
    src/say.c
    src/say.h
)
target_include_directories(say_core PUBLIC src)
say_apply_common_compile_options(say_core)

add_executable(tts src/main.c)
target_link_libraries(tts PRIVATE say_core)
say_apply_common_compile_options(tts)
set_target_properties(tts PROPERTIES OUTPUT_NAME tts)
say_set_runtime_output(tts "${CMAKE_SOURCE_DIR}/bin")

if(BUILD_LUA_EXTENSION)
    set(say_lua_dependency "")

    if(SAY_USE_BUNDLED_LUA AND EXISTS "${SAY_BUNDLED_LUA_DIR}/src/lua.h")
        add_subdirectory("${SAY_BUNDLED_LUA_DIR}" "${CMAKE_BINARY_DIR}/bundled-lua" EXCLUDE_FROM_ALL)
        say_set_runtime_output(say_bundled_lua "${CMAKE_SOURCE_DIR}/bin/lua")
        set(say_lua_dependency say_bundled_lua)
        message(STATUS "Using bundled Lua sources from ${SAY_BUNDLED_LUA_DIR}")
    else()
        find_package(Lua QUIET)

        if(TARGET Lua::Lua)
            set(say_lua_dependency Lua::Lua)
            message(STATUS "Using system Lua target Lua::Lua")
        elseif(LUA_FOUND)
            add_library(say_external_lua INTERFACE)
            target_include_directories(say_external_lua INTERFACE ${LUA_INCLUDE_DIR} ${LUA_INCLUDE_DIRS})
            target_link_libraries(say_external_lua INTERFACE ${LUA_LIBRARIES})
            set(say_lua_dependency say_external_lua)
            message(STATUS "Using system Lua from ${LUA_INCLUDE_DIR}")
        endif()
    endif()

    if(say_lua_dependency)
        add_library(say_lua MODULE src/say_lua.c)
        target_include_directories(say_lua PRIVATE src)
        target_link_libraries(say_lua PRIVATE say_core ${say_lua_dependency})

        say_apply_common_compile_options(say_lua)
        set_target_properties(say_lua PROPERTIES
            PREFIX ""
            OUTPUT_NAME "say"
        )
        say_set_runtime_output(say_lua "${CMAKE_SOURCE_DIR}/bin/lua")
    else()
        message(STATUS "Lua runtime not found; skipping Lua extension target")
    endif()
endif()
