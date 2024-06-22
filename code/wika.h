#pragma once

#define ENABLE_DEBUGGING

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
#include <wctype.h>
#include <limits.h>

#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

using U8  = uint8_t;
using U32 = uint32_t;

using Address    = uintptr_t;
using Size       = size_t;
using Difference = ssize_t;

constexpr Size KIB = 1024;
constexpr Size MIB = KIB * KIB;

constexpr Size DEFAULT_ALIGNMENT = 8;

constexpr Size MAX_FILE_PATH_SIZE = PATH_MAX;

constexpr U32 LMASK1  = 0x00000001;
constexpr U32 LMASK2  = 0x00000003;
constexpr U32 LMASK3  = 0x00000007;
constexpr U32 LMASK4  = 0x0000000f;
constexpr U32 LMASK5  = 0x0000001f;
constexpr U32 LMASK6  = 0x0000003f;
constexpr U32 LMASK7  = 0x0000007f;
constexpr U32 LMASK8  = 0x000000ff;
constexpr U32 LMASK9  = 0x000001ff;
constexpr U32 LMASK10 = 0x000003ff;
constexpr U32 LMASK11 = 0x000007ff;
constexpr U32 LMASK12 = 0x00000fff;
constexpr U32 LMASK13 = 0x00001fff;
constexpr U32 LMASK14 = 0x00003fff;
constexpr U32 LMASK15 = 0x00007fff;
constexpr U32 LMASK16 = 0x0000ffff;
constexpr U32 LMASK17 = 0x0001ffff;
constexpr U32 LMASK18 = 0x0003ffff;
constexpr U32 LMASK19 = 0x0007ffff;
constexpr U32 LMASK20 = 0x000fffff;
constexpr U32 LMASK21 = 0x001fffff;
constexpr U32 LMASK22 = 0x003fffff;
constexpr U32 LMASK23 = 0x007fffff;
constexpr U32 LMASK24 = 0x00ffffff;
constexpr U32 LMASK25 = 0x01ffffff;
constexpr U32 LMASK26 = 0x03ffffff;
constexpr U32 LMASK27 = 0x07ffffff;
constexpr U32 LMASK28 = 0x0fffffff;
constexpr U32 LMASK29 = 0x1fffffff;
constexpr U32 LMASK30 = 0x3fffffff;
constexpr U32 LMASK31 = 0x7fffffff;
constexpr U32 LMASK32 = 0xffffffff;

inline Size min(Size a, Size b)
{
	return a < b ? a : b;
}

inline Size format(char *output, Size size, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	size = vsnprintf(output, size, format, args);
	va_end(args);
	return size;
}

inline void copy_memory(void *output, const void *input, Size size)
{
	memcpy(output, input, size);
}

inline void set_memory(void *output, Size size, U8 value)
{
	memset(output, value, size);
}

inline Difference compare_memory(const void *a, const void *b, Size s)
{
	return memcmp(a, b, s);
}

inline void move_memory(void *output, const void *input, Size size)
{
	memmove(output, input, size);
}

#if 0
inline void insert_memory(void *output, const void *input, Size size)
{
}
#endif

inline char *find_string(char *haystack, const char *needle)
{
	return strstr(haystack, needle);
}

inline char *find_character(char *haystack, char needle)
{
	return strchr(haystack, needle);
}

inline Size get_length_of_string(const char *string)
{
	return strlen(string);
}

inline void copy_string(void *output, const void *input)
{
	strcpy((char *)output, (const char *)input);
}

inline Difference compare_string(const void *former, const void *latter)
{
	return strcmp((const char *)former, (const char *)latter);
}

Size get_alignment_addition(Address address, Size alignment);

Size get_alignment_subtraction(Address address, Size alignment);

inline Size align(Address address, Size alignment)
{
	return address + get_alignment_addition(address, alignment);
}

[[gnu::format(printf, 1, 2)]]
void print(const char *message, ...);

[[gnu::format(printf, 1, 2)]]
void report_error(const char *message, ...);

[[gnu::format(printf, 1, 2)]]
void report_warning(const char *message, ...);

[[gnu::format(printf, 1, 2)]]
void debug(const char *message, ...);

constexpr Size MEMORY_PAGE_SIZE = KIB * 4;

Size get_memory_page_size(void);

void *allocate(Size size);

void deallocate(void *pointr);

void *reallocate(void *pointer, Size size);

void *allocate_virtual_memory(void *address, Size size);

void deallocate_virtual_memory(void *pointer, Size size);

//using Allocator = void *(void *, Size);

// platform-specific stuff

using Handle = intptr_t;

const char *get_system_error_message(void);

bool open_file(Handle *handle, const char *path, bool writable = 0);

void close_file(Handle handle);

bool get_file_size(Handle handle, Size *size);

bool read_file(Handle handle, void *buffer, Size *size);

Size get_full_file_path(const char *path, char *buffer);

// containers

struct Array
{
	void *pointer;
	Size size;

};

void initialize_array(Array *array, Size size, void *pointer);

void uninitialize_array(Array *array);

void resize_array(Array *array, Size new_size);

struct Buffer : Array
{
	Size mass;
};

void initialize_buffer(Buffer *buffer, Size size, void *pointer);

void uninitialize_buffer(Buffer *buffer);

void transform_buffer_into_array(Buffer *buffer, Array *array);

void expand_buffer(Buffer *buffer, Size size);

void *ensure_buffer(Buffer *buffer, Size size);

void *reserve_from_buffer(Buffer *buffer, Size size, Size alignment = DEFAULT_ALIGNMENT);

void release_from_buffer(Buffer *buffer, Size size, Size alignment = DEFAULT_ALIGNMENT);

template<typename T>
struct Singly : T
{
	Singly *other;
};

template<typename T>
void attach_singly(Singly<T> *singly, Singly<T> *other);

using Arena_Buffer = Singly<Buffer>;

struct Arena
{
	Arena_Buffer *first;
	Arena_Buffer *last;
};

void initialize_arena(Arena *arena, Size size);

void uninitialize_arena(Arena *arena);

void *reserve_from_arena(Arena *arena, Size size, Size alignment = DEFAULT_ALIGNMENT);

void iterate_over_arena(Arena *arena, bool (*procedure)(void *input_pointer, void *pointer), void *input, Size size, Size alignment);

struct Arena_Pointer
{
	Arena *arena;
	Arena_Buffer *buffer;
	Size offset;
};

void get_arena_pointer(Arena *arena, Arena_Pointer *pointer);

void set_arena(Arena_Pointer *pointer);

// Unicode

using Utf8 = U8;

Size decode_utf8(U32 *codepoint, const U8 *string);

Size get_utf8_size(U32 codepoint);

// language-specific artifacts

struct Source
{
	Size path_size;
	const char *path;
	Handle handle;
	Size data_size;
	U8 *data;
};

enum Token_Type
{
	Token_Type_NONE              = 0,
	Token_Type_IDENTIFIER        = 1,
	Token_Type_PROC              = 2,
	Token_Type_COLON             = ':',
	Token_Type_SEMICOLON         = ';',
	Token_Type_LEFT_PARENTHESIS  = '(',
	Token_Type_RIGHT_PARENTHESIS = ')',
	Token_Type_LEFT_BRACE        = '{',
	Token_Type_RIGHT_BRACE       = '}',
};

struct Token
{
	Token_Type type;
	Size position;
	Size size;
	char *representation;
};

Size format_token(char *buffer, Size size, const Token *token);

struct Location
{
	const Source *source;
	Size position;
};

struct Parser
{
	Location location;
	Token token;
	Buffer identifiers;
};

void initialize_parser(Parser *parser, const Source *source);

Size parse(Parser *parser);

void report_parsing_error(Parser *parser, const char *format, ...);
