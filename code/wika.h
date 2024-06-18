#pragma once

#include <concepts>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdint.h>

#include <stdlib.h>

#include <unistd.h>
#include <sys/mman.h>

typedef uint8_t U8;
typedef uintptr_t Address;
typedef size_t  Size;
typedef ssize_t Difference;

constexpr Size KIB = 1024;
constexpr Size MIB = KIB * KIB;

constexpr Size DEFAULT_ALIGNMENT = 8;

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

struct Array
{
	U8 *pointer;
	Size size;

};

void initialize_array(Array *array, Size size, Reallocator *reallocator);

void resize_array(Array *array, Size new_size, Reallocator *reallocator);

struct Buffer : Array
{
	Size mass;
};

void initialize_buffer(Buffer *buffer, Size size, Reallocator *reallocator);

void expand_buffer(Buffer *buffer, Size size, Reallocator *reallocator);

void *ensure_buffer(Buffer *buffer, Size size, Reallocator *reallocator);

void *reserve_in_buffer(Buffer *buffer, Size size, Reallocator *reallocator);
