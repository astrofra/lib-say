#include "say.h"

#include "lua.h"
#include "lauxlib.h"

#if LUA_VERSION_NUM == 501
#define luaL_newlib(L, l) (lua_newtable((L)), luaL_register((L), NULL, (l)))
#define luaL_setfuncs(L, l, nup) luaL_register((L), NULL, (l))
#endif

#if defined(_WIN32)
#define SAY_LUA_API __declspec(dllexport)
#else
#define SAY_LUA_API
#endif

#define SAYLUA_BLOB_METATABLE "libsay.blob"

typedef struct saylua_blob_t {
    uint8_t *data;
    size_t size;
} saylua_blob_t;

static int saylua_abs_index(lua_State *L, int index)
{
    if (index > 0 || index <= LUA_REGISTRYINDEX) {
        return index;
    }
    return lua_gettop(L) + index + 1;
}

static int saylua_get_string_field(lua_State *L, int index, const char *key, const char **out_value)
{
    lua_getfield(L, index, key);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }

    *out_value = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    return 1;
}

static int saylua_get_integer_field(lua_State *L, int index, const char *key, lua_Integer *out_value)
{
    lua_getfield(L, index, key);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }

    *out_value = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return 1;
}

static int saylua_get_boolean_field(lua_State *L, int index, const char *key, int *out_value)
{
    int type;

    lua_getfield(L, index, key);
    type = lua_type(L, -1);
    if (type == LUA_TNIL) {
        lua_pop(L, 1);
        return 0;
    }
    if (type != LUA_TBOOLEAN) {
        lua_pop(L, 1);
        luaL_error(L, "option '%s' must be a boolean", key);
    }

    *out_value = lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);
    return 1;
}

static saylua_blob_t *saylua_check_blob(lua_State *L, int index)
{
    return (saylua_blob_t *) luaL_checkudata(L, index, SAYLUA_BLOB_METATABLE);
}

static void saylua_push_blob(lua_State *L, uint8_t *data, size_t size)
{
    saylua_blob_t *blob;

    blob = (saylua_blob_t *) lua_newuserdata(L, sizeof(*blob));
    blob->data = data;
    blob->size = size;

    luaL_getmetatable(L, SAYLUA_BLOB_METATABLE);
    lua_setmetatable(L, -2);
}

static void saylua_parse_options(
    lua_State *L,
    int index,
    say_options_t *out_options,
    say_audio_format_t *out_format
)
{
    const char *text_value;
    lua_Integer int_value;
    int bool_value;

    say_default_options(out_options);
    *out_format = SAY_FORMAT_RAW;

    if (lua_isnoneornil(L, index)) {
        return;
    }

    index = saylua_abs_index(L, index);
    luaL_checktype(L, index, LUA_TTABLE);

    if (saylua_get_string_field(L, index, "lang", &text_value) ||
        saylua_get_string_field(L, index, "language", &text_value)) {
        if (!say_parse_language(text_value, &out_options->language)) {
            luaL_error(L, "unsupported language '%s'", text_value);
        }
    }

    if (saylua_get_integer_field(L, index, "sample_rate", &int_value) ||
        saylua_get_integer_field(L, index, "rate", &int_value)) {
        out_options->sample_rate = (int) int_value;
    }

    if (saylua_get_integer_field(L, index, "frame_ms", &int_value)) {
        out_options->frame_ms = (int) int_value;
    }

    if (saylua_get_boolean_field(L, index, "phonemes", &bool_value)) {
        out_options->phoneme_input = bool_value;
    }

    if (saylua_get_string_field(L, index, "format", &text_value)) {
        if (!say_parse_audio_format(text_value, out_format)) {
            luaL_error(L, "unsupported format '%s'", text_value);
        }
    }
}

static void saylua_push_info(
    lua_State *L,
    const say_options_t *options,
    say_audio_format_t format,
    size_t sample_count,
    size_t byte_count
)
{
    lua_createtable(L, 0, 9);

    lua_pushstring(L, say_language_name(options->language));
    lua_setfield(L, -2, "language");

    lua_pushstring(L, say_audio_format_name(format));
    lua_setfield(L, -2, "format");

    lua_pushinteger(L, options->sample_rate);
    lua_setfield(L, -2, "sample_rate");

    lua_pushinteger(L, options->frame_ms);
    lua_setfield(L, -2, "frame_ms");

    lua_pushboolean(L, options->phoneme_input);
    lua_setfield(L, -2, "phonemes");

    lua_pushinteger(L, 1);
    lua_setfield(L, -2, "channels");

    lua_pushinteger(L, 16);
    lua_setfield(L, -2, "bits_per_sample");

    lua_pushinteger(L, (lua_Integer) sample_count);
    lua_setfield(L, -2, "sample_count");

    lua_pushinteger(L, (lua_Integer) byte_count);
    lua_setfield(L, -2, "byte_count");

    lua_pushnumber(L, options->sample_rate > 0 ? (lua_Number) sample_count / (lua_Number) options->sample_rate : 0.0);
    lua_setfield(L, -2, "duration_seconds");

    if (format == SAY_FORMAT_AIFF) {
        lua_pushstring(L, "s16be");
        lua_setfield(L, -2, "pcm_encoding");
    }
    else {
        lua_pushstring(L, "s16le");
        lua_setfield(L, -2, "pcm_encoding");
    }
}

static int saylua_blob_gc(lua_State *L)
{
    saylua_blob_t *blob;

    blob = saylua_check_blob(L, 1);
    if (blob->data != NULL) {
        say_free(blob->data);
        blob->data = NULL;
    }
    blob->size = 0;
    return 0;
}

static int saylua_blob_get_data(lua_State *L)
{
    const saylua_blob_t *blob;

    blob = saylua_check_blob(L, 1);
    lua_pushinteger(L, (lua_Integer) (intptr_t) blob->data);
    return 1;
}

static int saylua_blob_get_size(lua_State *L)
{
    const saylua_blob_t *blob;

    blob = saylua_check_blob(L, 1);
    lua_pushinteger(L, (lua_Integer) blob->size);
    return 1;
}

static int saylua_synthesize(lua_State *L)
{
    say_options_t options;
    say_audio_format_t format;
    const char *input;
    int16_t *samples;
    size_t sample_count;
    uint8_t *blob;
    size_t blob_size;
    char error[256];

    input = luaL_checkstring(L, 1);
    saylua_parse_options(L, 2, &options, &format);

    samples = NULL;
    sample_count = 0;
    blob = NULL;
    blob_size = 0;
    error[0] = '\0';

    if (!say_synthesize(input, &options, &samples, &sample_count, error, sizeof(error))) {
        return luaL_error(L, "%s", error);
    }

    if (!say_encode_audio(format, options.sample_rate, samples, sample_count, &blob, &blob_size, error, sizeof(error))) {
        say_free(samples);
        return luaL_error(L, "%s", error);
    }

    saylua_push_blob(L, blob, blob_size);
    saylua_push_info(L, &options, format, sample_count, blob_size);

    say_free(samples);
    return 2;
}

static int saylua_debug_report(lua_State *L)
{
    say_options_t options;
    say_audio_format_t ignored_format;
    const char *input;
    char *report;
    char error[256];

    input = luaL_checkstring(L, 1);
    saylua_parse_options(L, 2, &options, &ignored_format);

    report = NULL;
    error[0] = '\0';
    if (!say_build_debug_report(input, &options, &report, error, sizeof(error))) {
        return luaL_error(L, "%s", error);
    }

    lua_pushstring(L, report);
    say_free(report);
    return 1;
}

static int saylua_default_options(lua_State *L)
{
    say_options_t options;

    say_default_options(&options);
    lua_createtable(L, 0, 5);

    lua_pushstring(L, say_language_name(options.language));
    lua_setfield(L, -2, "language");

    lua_pushinteger(L, options.sample_rate);
    lua_setfield(L, -2, "sample_rate");

    lua_pushinteger(L, options.frame_ms);
    lua_setfield(L, -2, "frame_ms");

    lua_pushboolean(L, options.phoneme_input);
    lua_setfield(L, -2, "phonemes");

    lua_pushstring(L, say_audio_format_name(SAY_FORMAT_RAW));
    lua_setfield(L, -2, "format");

    return 1;
}

static const luaL_Reg g_saylua_functions[] = {
    { "debug_report", saylua_debug_report },
    { "default_options", saylua_default_options },
    { "synthesize", saylua_synthesize },
    { NULL, NULL }
};

static const luaL_Reg g_saylua_blob_methods[] = {
    { "GetData", saylua_blob_get_data },
    { "GetSize", saylua_blob_get_size },
    { "__gc", saylua_blob_gc },
    { NULL, NULL }
};

SAY_LUA_API int luaopen_say(lua_State *L)
{
    luaL_newmetatable(L, SAYLUA_BLOB_METATABLE);
    luaL_setfuncs(L, g_saylua_blob_methods, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_newlib(L, g_saylua_functions);

    lua_pushstring(L, "en");
    lua_setfield(L, -2, "LANG_EN");

    lua_pushstring(L, "fr");
    lua_setfield(L, -2, "LANG_FR");

    lua_pushstring(L, "raw");
    lua_setfield(L, -2, "FORMAT_RAW");

    lua_pushstring(L, "aiff");
    lua_setfield(L, -2, "FORMAT_AIFF");

    lua_pushstring(L, "wav");
    lua_setfield(L, -2, "FORMAT_WAV");

    return 1;
}
