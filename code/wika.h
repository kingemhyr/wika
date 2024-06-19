#pragma once

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>

#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

typedef uint8_t U8;
typedef uintptr_t Address;
typedef size_t  Size;
typedef ssize_t Difference;

constexpr Size KIB = 1024;
constexpr Size MIB = KIB * KIB;

constexpr Size DEFAULT_ALIGNMENT = 8;

inline void copy_memory(void *output, const void *input, Size size)
{
	memcpy(output, input, size);
}

inline void set_memory(void *output, Size size, U8 value)
{
	memset(output, value, size);
}

inline void move_memory(void *output, void *input, Size size)
{
	copy_memory(output, input, size);
	set_memory(input, size, 0);
}

inline Difference compare_string(const char *former, const char *latter)
{
	return strcmp(former, latter);
}

Size get_alignment_addition(Address address, Size alignment);

inline Size align(Address address, Size alignment)
{
	return address + get_alignment_addition(address, alignment);
}

[[gnu::format(printf, 1, 2)]]
void print(const char *message, ...);

[[gnu::format(printf, 1, 2)]]
void report_error(const char *message, ...);

[[gnu::format(printf, 1, 2)]]
void report_debug(const char *message, ...);

constexpr Size MEMORY_PAGE_SIZE = KIB * 4;

Size get_memory_page_size(void);

void *allocate(Size size);

void deallocate(void *pointr);

void *reallocate(void *pointer, Size size);

void *allocate_virtual_memory(void *address, Size size);

using Reallocator = void *(void *, Size);

// platform-specific stuff

using Handle = intptr_t;

const char *get_system_error_message(void);

bool open_file(Handle *handle, const char *path, bool writable);

void close_file(Handle handle);

bool get_file_size(Handle handle, Size *size);

bool read_file(Handle handle, void *buffer, Size *size);

// containers

struct Array
{
	U8 *pointer;
	Size size;

};

void initialize_array(Array *array, Size size, Reallocator *reallocator);

void uninitialize_array(Array *array, Reallocator *reallocator);

void resize_array(Array *array, Size new_size, Reallocator *reallocator);

struct Buffer : Array
{
	Size mass;
};

void initialize_buffer(Buffer *buffer, Size size, Reallocator *reallocator);

void uninitialize_buffer(Buffer *buffer, Reallocator *reallocator);

void transform_buffer_into_array(Buffer *buffer, Array *array, Reallocator *reallocator);

void expand_buffer(Buffer *buffer, Size size, Reallocator *reallocator);

void *ensure_buffer(Buffer *buffer, Size size, Reallocator *reallocator);

void *reserve_from_buffer(Buffer *buffer, Size size, Reallocator *reallocator);

// language-specific artifacts

struct Source
{
	const char *file_path;
	Handle file_handle;
	Size file_size;
	U8 *data_pointer;
};

enum Token_Type
{

};

struct Token
{
	Token_Type type;
	Size position;
	Size size;
};

struct Parser
{
	const Source *source;
	Size position;
	Token token;
};

void initialize_parser(Parser *parser, const Source *source);

Size lex(Parser *parser, Array *tokens);
