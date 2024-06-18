#pragma once

#include "wika.h"

using Handle = intptr_t;

Handle open_file(const char *path, bool writable = false);

ssize_t get_file_size(Handle file);

ssize_t read_file(void *buffer, size_t size, Handle file);
