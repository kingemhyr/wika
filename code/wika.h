#pragma once

#if defined __linux__
	#define ON_PLATFORM_LINUX
#endif

#if defined ON_PLATFORM_LINUX
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <sys/mman.h>
#endif

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <cstdarg>

void set_termination_on_error(bool value);
bool get_termination_on_error(void);

void terminate(void);

void log_error(const char *format, ...);
