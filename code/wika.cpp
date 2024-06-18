#include "wika.h"

constexpr char help_message[] =
	"USAGE: wika [options] path...\n"
	"\n"
	"OPTIONS:\n";

void display_help(void)
{
	printf("%s", help_message);
}

/* the following two are forward-declared at the end of the file */
void initialize(void);
void terminate(void);

struct
{
	bool print_tokens;
}
compiler_options;

int main(int arguments_count, char **arguments)
{
	initialize();

	int exit_code = 0;

	if (arguments_count <= 1)
	{
          report_error("no source paths");
          display_help();
          return 0;
	}

	Buffer source_paths;
	initialize_buffer(&source_paths, sizeof(char *), reallocate);
	{
		/* parse commandline arguments */
		for (Size i = 1; i < arguments_count; ++i)
		{
			char *argument = arguments[i];
			if (argument[0] == '-')
			{
				/* the argument is an option */
				if (argument[1] == '-')
				{
					const char *option = &argument[2];
					if (compare_string(option, "print_tokens") == 0)
						compiler_options.print_tokens = true;
				}
				else
				{
					const char option = argument[1];
				}
			}
			else
			{
				/* the argument is an source path */
				char **reservation = (char **)reserve_in_buffer(&source_paths, sizeof(char *), reallocate);
				*reservation = argument;
			}
		}

		Size source_paths_count = source_paths.mass / sizeof(char *);
		for (Size i = 0; i < source_paths_count; ++i)
		{
			char *source_path = ((char **)source_paths.pointer)[i];
			report_debug("source_paths[%lu]: %s", i, source_path);
		}
	}

	terminate();

	return exit_code;
}

Size get_alignment_addition(Address address, Size alignment)
{
	assert(alignment % 2 == 0);
	Size addition = 0;
	if (Size mod = address & (alignment - 1); mod != 0)
		addition = alignment - mod;
	return addition;
}

void report_error(const char *message, ...)
{
	fprintf(stderr, "error: ");
	va_list args;
	va_start(args, message);
	vfprintf(stderr, message, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void report_debug(const char *message, ...)
{
	fprintf(stderr, "debug: ");
	va_list args;
	va_start(args, message);
	vfprintf(stderr, message, args);
	va_end(args);
	fprintf(stderr, "\n");
}

Size get_memory_page_size(void)
{
	return getpagesize();
}

void *allocate(Size size)
{
	return malloc(size);
}

void deallocate(void *pointer)
{
	free(pointer);
}

void *reallocate(void *pointer, Size size)
{
	return realloc(pointer, size);
}

void *allocate_virtual_memory(void *address, Size size)
{
	void *result = mmap(address, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (result == MAP_FAILED)
		result = 0;
	return result;
}

void initialize_array(Array *array, Size size, Reallocator *reallocator)
{
	array->pointer = 0;
	resize_array(array, size, reallocator);
}

void resize_array(Array *array, Size new_size, Reallocator *reallocator)
{
	array->pointer = (U8 *)reallocator(array->pointer, new_size);
	array->size = new_size;
}

void initialize_buffer(Buffer *buffer, Size size, Reallocator *reallocator)
{
	initialize_array(buffer, size, reallocator);
	buffer->mass = 0;
}

void expand_buffer(Buffer *buffer, Size size, Reallocator *reallocator)
{
	Size new_size = size + buffer->size + buffer->size / 2;
	resize_array(buffer, new_size, reallocator);
}

void *ensure_buffer(Buffer *buffer, Size size, Reallocator *reallocator)
{
	Size space = buffer->size - buffer->mass;
	if (space < size)
		expand_buffer(buffer, size, reallocator);
	return &buffer->pointer[buffer->mass];
}

void *reserve_in_buffer(Buffer *buffer, Size size, Reallocator *reallocator)
{
	void *end = ensure_buffer(buffer, size, reallocator);
	buffer->mass += size;
	return end;
}

/* end of file stuff */

/* initialize any global objects/states */
void initialize(void)
{
	setbuf(stdout, 0);
	setbuf(stderr, 0);
}

void terminate(void)
{

}
