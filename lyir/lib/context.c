/*
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2023 Local Atticus
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "lyir.h"

// TODO(local): remove this, very soonly
#include "laye.h"

void layec_type_destroy(lyir_type* type);
void layec_value_destroy(lyir_value* value);

lyir_target_info* lyir_default_target;
lyir_target_info* lyir_x86_64_linux;
lyir_target_info* lyir_x86_64_windows;

void lyir_init_targets(lca_allocator allocator) {
    lyir_x86_64_linux = lca_allocate(allocator, sizeof(lyir_target_info));
    lyir_x86_64_windows = lca_allocate(allocator, sizeof(lyir_target_info));

    assert(lyir_x86_64_linux != NULL);
    *lyir_x86_64_linux = (lyir_target_info) {
        .ffi = {
            .size_of_bool = 8,
            .size_of_char = 8,
            .size_of_short = 16,
            .size_of_int = 32,
            .size_of_long = 64,
            .size_of_long_long = 64,
            .size_of_float = 32,
            .size_of_double = 64,

            .align_of_bool = 8,
            .align_of_char = 8,
            .align_of_short = 16,
            .align_of_int = 32,
            .align_of_long = 64,
            .align_of_long_long = 64,
            .align_of_float = 32,
            .align_of_double = 64,

            .char_is_signed = true,
        },

        .size_of_pointer = 64,
        .align_of_pointer = 64,
    };

    assert(lyir_x86_64_windows != NULL);
    memcpy(lyir_x86_64_windows, lyir_x86_64_linux, sizeof(lyir_target_info));
    lyir_x86_64_windows->ffi.size_of_long = 32;
    lyir_x86_64_windows->ffi.align_of_long = 32;

    lyir_default_target = lyir_x86_64_linux;

    assert(lyir_default_target != NULL);
}

lyir_context* lyir_context_create(lca_allocator allocator) {
    lyir_context* context = lca_allocate(allocator, sizeof *context);
    assert(context != NULL);

    context->allocator = allocator;

    context->target = lyir_default_target;
    assert(context->target != NULL);

    context->max_interned_string_size = 1024 * 1024;

    context->string_arena = lca_arena_create(allocator, context->max_interned_string_size);
    assert(context->string_arena != NULL);

    context->laye_types.type = laye_node_create_in_context(context, LAYE_NODE_TYPE_TYPE, (laye_type){0});
    assert(context->laye_types.type != NULL);
    context->laye_types.type->type = LTY(context->laye_types.type);
    context->laye_types.type->sema_state = LYIR_SEMA_OK;

    context->laye_types.poison = laye_node_create_in_context(context, LAYE_NODE_TYPE_POISON, LTY(context->laye_types.type));
    assert(context->laye_types.poison != NULL);
    context->laye_types.poison->sema_state = LYIR_SEMA_OK;

    context->laye_types.unknown = laye_node_create_in_context(context, LAYE_NODE_TYPE_UNKNOWN, LTY(context->laye_types.type));
    assert(context->laye_types.unknown != NULL);
    context->laye_types.unknown->sema_state = LYIR_SEMA_OK;

    context->laye_types.var = laye_node_create_in_context(context, LAYE_NODE_TYPE_VAR, LTY(context->laye_types.type));
    assert(context->laye_types.var != NULL);
    context->laye_types.var->sema_state = LYIR_SEMA_OK;

    context->laye_types._void = laye_node_create_in_context(context, LAYE_NODE_TYPE_VOID, LTY(context->laye_types.type));
    assert(context->laye_types._void != NULL);
    context->laye_types._void->sema_state = LYIR_SEMA_OK;

    context->laye_types.noreturn = laye_node_create_in_context(context, LAYE_NODE_TYPE_NORETURN, LTY(context->laye_types.type));
    assert(context->laye_types.noreturn != NULL);
    context->laye_types.noreturn->sema_state = LYIR_SEMA_OK;

    context->laye_types._bool = laye_node_create_in_context(context, LAYE_NODE_TYPE_BOOL, LTY(context->laye_types.type));
    assert(context->laye_types._bool != NULL);
    context->laye_types._bool->type_primitive.bit_width = 8;
    context->laye_types._bool->sema_state = LYIR_SEMA_OK;

    context->laye_types.i8 = laye_node_create_in_context(context, LAYE_NODE_TYPE_INT, LTY(context->laye_types.type));
    assert(context->laye_types.i8 != NULL);
    context->laye_types.i8->sema_state = LYIR_SEMA_OK;
    context->laye_types.i8->type_primitive.bit_width = 8;
    context->laye_types.i8->type_primitive.is_signed = true;

    context->laye_types._int = laye_node_create_in_context(context, LAYE_NODE_TYPE_INT, LTY(context->laye_types.type));
    assert(context->laye_types._int != NULL);
    context->laye_types._int->sema_state = LYIR_SEMA_OK;
    context->laye_types._int->type_primitive.is_platform_specified = true;
    context->laye_types._int->type_primitive.bit_width = context->target->size_of_pointer;
    context->laye_types._int->type_primitive.is_signed = true;

    context->laye_types._uint = laye_node_create_in_context(context, LAYE_NODE_TYPE_INT, LTY(context->laye_types.type));
    assert(context->laye_types._uint != NULL);
    context->laye_types._uint->sema_state = LYIR_SEMA_OK;
    context->laye_types._uint->type_primitive.is_platform_specified = true;
    context->laye_types._uint->type_primitive.bit_width = context->target->size_of_pointer;
    context->laye_types._uint->type_primitive.is_signed = false;

    // TODO(local): remove the generic `float` type from Laye
    context->laye_types._float = laye_node_create_in_context(context, LAYE_NODE_TYPE_FLOAT, LTY(context->laye_types.type));
    assert(context->laye_types._float != NULL);
    context->laye_types._float->type_primitive.is_platform_specified = true;
    context->laye_types._float->type_primitive.bit_width = 64;
    context->laye_types._float->sema_state = LYIR_SEMA_OK;
    // TODO(local): remove the generic `float` type from Laye

    context->laye_types.i8_buffer = laye_node_create_in_context(context, LAYE_NODE_TYPE_BUFFER, LTY(context->laye_types.type));
    assert(context->laye_types.i8_buffer != NULL);
    context->laye_types.i8_buffer->type_container.element_type = LTY(context->laye_types.i8);
    context->laye_types.i8_buffer->sema_state = LYIR_SEMA_OK;

    context->laye_dependencies = lyir_dependency_graph_create_in_context(context);
    assert(context->laye_dependencies != NULL);

    context->type_arena = lca_arena_create(allocator, 1024 * 1024);
    assert(context->type_arena != NULL);

    return context;
}

void lyir_context_destroy(lyir_context* context) {
    if (context == NULL) return;

    lca_allocator allocator = context->allocator;

    for (int64_t i = 0, count = arr_count(context->sources); i < count; i++) {
        lyir_source* source = &context->sources[i];
        string_destroy(&source->name);
        string_destroy(&source->text);
    }

    arr_free(context->sources);
    arr_free(context->include_directories);
    arr_free(context->library_directories);
    arr_free(context->link_libraries);

    lca_arena_destroy(context->string_arena);
    arr_free(context->_interned_strings);

    for (int64_t i = 0, count = arr_count(context->allocated_strings); i < count; i++) {
        string* string = &context->allocated_strings[i];
        string_destroy(string);
    }

    for (int64_t i = 0; i < arr_count(context->ir_modules); i++) {
        layec_module_destroy(context->ir_modules[i]);
        context->ir_modules[i] = NULL;
    }

    for (int64_t i = 0; i < arr_count(context->laye_modules); i++) {
        laye_module_destroy(context->laye_modules[i]);
        context->laye_modules[i] = NULL;
    }

    arr_free(context->allocated_strings);
    arr_free(context->laye_modules);
    arr_free(context->ir_modules);

    lca_deallocate(allocator, context->laye_types.poison);
    lca_deallocate(allocator, context->laye_types.unknown);
    lca_deallocate(allocator, context->laye_types.var);
    lca_deallocate(allocator, context->laye_types.type);
    lca_deallocate(allocator, context->laye_types._void);
    lca_deallocate(allocator, context->laye_types.noreturn);
    lca_deallocate(allocator, context->laye_types._bool);
    lca_deallocate(allocator, context->laye_types.i8);
    lca_deallocate(allocator, context->laye_types._int);
    lca_deallocate(allocator, context->laye_types._uint);
    lca_deallocate(allocator, context->laye_types._float);
    lca_deallocate(allocator, context->laye_types.i8_buffer);

    for (int64_t i = 0, count = arr_count(context->_all_depgraphs); i < count; i++) {
        lyir_dependency_graph_destroy(context->_all_depgraphs[i]);
    }

    arr_free(context->_all_depgraphs);

    for (int64_t i = 0, count = arr_count(context->_all_types); i < count; i++) {
        layec_type_destroy(context->_all_types[i]);
    }
    
    arr_free(context->types.int_types);
    arr_free(context->_all_types);
    arr_free(context->_all_struct_types);
    lca_arena_destroy(context->type_arena);

    for (int64_t i = 0, count = arr_count(context->_all_values); i < count; i++) {
        layec_value_destroy(context->_all_values[i]);
        lca_deallocate(allocator, context->_all_values[i]);
    }

    arr_free(context->_all_values);

    *context = (lyir_context){0};
    lca_deallocate(allocator, context);
}

static int read_file_to_string(lca_allocator allocator, string file_path, string* out_contents) {
    assert(out_contents != NULL);
    const char* file_path_cstr = string_as_cstring(file_path);
    assert(file_path_cstr != NULL);
    FILE* stream = fopen(file_path_cstr, "r");
    if (stream == NULL) {
        return errno;
    }
    fseek(stream, 0, SEEK_END);
    int64_t count = ftell(stream);
    fseek(stream, 0, SEEK_SET);
    char* data = lca_allocate(allocator, count + 1);
    fread(data, (size_t)count, 1, stream);
    data[count] = 0;
    fclose(stream);
    *out_contents = string_from_data(allocator, data, count, count + 1);
    return 0;
}

lyir_sourceid lyir_context_get_or_add_source_from_file(lyir_context* context, string_view file_path) {
    assert(context != NULL);

    lyir_sourceid sourceid = 0;
    for (; sourceid < arr_count(context->sources); sourceid++) {
        if (string_view_equals(string_as_view(context->sources[sourceid].name), file_path))
            return sourceid;
    }

    string file_path_owned = string_view_to_string(context->allocator, file_path);
    string text = {0};
    
    int error_code = read_file_to_string(context->allocator, file_path_owned, &text);
    if (error_code != 0) {
        //const char* error_string = strerror(error_code);
        //fprintf(stderr, "Error when opening source file \"%.*s\": %s\n", STR_EXPAND(file_path), error_string);

        string_destroy(&file_path_owned);
        return -1;
    }

    return lyir_context_get_or_add_source_from_string(context, file_path_owned, text);
}

lyir_sourceid lyir_context_get_or_add_source_from_string(lyir_context* context, string name, string source_text) {
    assert(context != NULL);

    lyir_sourceid sourceid = arr_count(context->sources);
    lyir_source source = {
        .name = name,
        .text = source_text,
    };

    arr_push(context->sources, source);
    return sourceid;
}

lyir_source lyir_context_get_source(lyir_context* context, lyir_sourceid sourceid) {
    assert(context != NULL);
    assert(sourceid >= 0 && sourceid < arr_count(context->sources));
    return context->sources[sourceid];
}

bool lyir_context_get_location_info(lyir_context* context, lyir_location location, string_view* out_name, int64_t* out_line, int64_t* out_column) {
    assert(context != NULL);

    if (location.offset < 0) return false;

    lyir_source source = lyir_context_get_source(context, location.sourceid);
    if (out_name != NULL) *out_name = string_as_view(source.name);

    if (location.offset >= source.text.count) return false;
    if (location.offset + location.length > source.text.count) return false;

    int64_t last_line_start_offset = 0;
    int64_t line_number = 1;

    char lastc = 0;
    for (int64_t i = 0; i <= location.offset; i++) {
        if (lastc == '\n') {
            last_line_start_offset = i;
            line_number++;
        }

        lastc = source.text.data[i];
    }

    if (out_line != NULL) *out_line = line_number;
    if (out_column != NULL) *out_column = 1 + (location.offset - last_line_start_offset);

    return true;
}

void lyir_context_print_location_info(lyir_context* context, lyir_location location, lyir_status status, FILE* stream, bool use_color) {
    assert(context != NULL);

    lyir_source source = lyir_context_get_source(context, location.sourceid);
    string_view name = string_as_view(source.name);

    const char* col = "";
    const char* status_string = "";

    switch (status) {
        default: break;
        case LYIR_INFO: col = COL(CYAN); status_string = "Info:"; break;
        case LYIR_NOTE: col = COL(BRIGHT_GREEN); status_string = "Note:"; break;
        case LYIR_WARN: col = COL(YELLOW); status_string = "Warning:"; break;
        case LYIR_ERROR: col = COL(RED); status_string = "Error:"; break;
        case LYIR_FATAL: col = COL(BRIGHT_RED); status_string = "Fatal:"; break;
        case LYIR_ICE: col = COL(MAGENTA); status_string = "Internal Compiler Exception:"; break;
    }

    fprintf(stream, "%.*s", STR_EXPAND(name));

    if (context->use_byte_positions_in_diagnostics) {
        fprintf(stream, "[%ld]", location.offset);
    } else {
        int64_t line = 0, column = 0;
        if (lyir_context_get_location_info(context, location, &name, &line, &column)) {
            fprintf(stream, "(%ld, %ld)", line, column);
        } else {
            fprintf(stream, "(0, 0)");
        }
    }

    fprintf(stream, ": %s%s%s", col, status_string, COL(RESET));
}

#define GET_MESSAGE \
    va_list v; \
    va_start(v, format); \
    string message = string_vformat(format, v); \
    va_end(v)

lyir_diag lyir_info(lyir_context* context, lyir_location location, const char* format, ...) {
    GET_MESSAGE;
    return (lyir_diag) {
        .location = location,
        .status = LYIR_INFO,
        .message = message
    };
}

lyir_diag lyir_note(lyir_context* context, lyir_location location, const char* format, ...) {
    GET_MESSAGE;
    return (lyir_diag) {
        .location = location,
        .status = LYIR_NOTE,
        .message = message
    };
}

lyir_diag lyir_warn(lyir_context* context, lyir_location location, const char* format, ...) {
    GET_MESSAGE;
    return (lyir_diag) {
        .location = location,
        .status = LYIR_WARN,
        .message = message
    };
}

lyir_diag lyir_error(lyir_context* context, lyir_location location, const char* format, ...) {
    GET_MESSAGE;
    return (lyir_diag) {
        .location = location,
        .status = LYIR_ERROR,
        .message = message
    };
}

lyir_diag lyir_ice(lyir_context* context, lyir_location location, const char* format, ...) {
    GET_MESSAGE;
    return (lyir_diag) {
        .location = location,
        .status = LYIR_ICE,
        .message = message
    };
}

void lyir_write_diag(lyir_context* context, lyir_diag diag) {
    lyir_context_print_location_info(context, diag.location, diag.status, stderr, context->use_color);
    fprintf(stderr, " %.*s\n", STR_EXPAND(diag.message));
    if (diag.status == LYIR_ERROR || diag.status == LYIR_FATAL || diag.status == LYIR_ICE) {
        context->has_reported_errors = true;
    }
}

void lyir_write_info(lyir_context* context, lyir_location location, const char* format, ...) {
    GET_MESSAGE;
    lyir_context_print_location_info(context, location, LYIR_INFO, stderr, context->use_color);
    fprintf(stderr, " %.*s\n", STR_EXPAND(message));
}

void lyir_write_note(lyir_context* context, lyir_location location, const char* format, ...) {
    GET_MESSAGE;
    lyir_context_print_location_info(context, location, LYIR_NOTE, stderr, context->use_color);
    fprintf(stderr, " %.*s\n", STR_EXPAND(message));
}

void lyir_write_warn(lyir_context* context, lyir_location location, const char* format, ...) {
    GET_MESSAGE;
    lyir_context_print_location_info(context, location, LYIR_WARN, stderr, context->use_color);
    fprintf(stderr, " %.*s\n", STR_EXPAND(message));
}

void lyir_write_error(lyir_context* context, lyir_location location, const char* format, ...) {
    GET_MESSAGE;
    lyir_context_print_location_info(context, location, LYIR_ERROR, stderr, context->use_color);
    fprintf(stderr, " %.*s\n", STR_EXPAND(message));
    context->has_reported_errors = true;
}

void lyir_write_ice(lyir_context* context, lyir_location location, const char* format, ...) {
    GET_MESSAGE;
    lyir_context_print_location_info(context, location, LYIR_ICE, stderr, context->use_color);
    fprintf(stderr, " %.*s\n", STR_EXPAND(message));
    context->has_reported_errors = true;
}

#undef GET_MESSAGE

string_view lyir_context_intern_string_view(lyir_context* context, string_view s) {
    if (s.count + 1 > context->max_interned_string_size) {
        string allocated_string = string_view_to_string(context->allocator, s);
        arr_push(context->allocated_strings, allocated_string);
        return string_as_view(allocated_string);
    }

    // TODO(local): these aren't properly interned yet, do that eventually.

    char* arena_string_data = lca_arena_push(context->string_arena, s.count + 1);
    memcpy(arena_string_data, s.data, (size_t)s.count);
    
    string arena_string = string_from_data(context->allocator, arena_string_data, s.count, s.count + 1);

    return string_as_view(arena_string);
}
