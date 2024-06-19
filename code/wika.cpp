#include "wika.h"

constexpr char help_message[] =
	"USAGE: wika [options] path...\n"
	"\n"
	"OPTIONS:\n";

void display_help(void)
{
	printf("%s", help_message);
}

// the following two are forward-declared at the end of the file
void initialize(void);
void terminate(void);

struct
{
	bool print_tokens;
}
compilation_options;

Size compilation_errors_count = 0;

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

	static Size sources_count = 0;
	static Buffer source_paths;
	initialize_buffer(&source_paths, sizeof(char *), reallocate);

	// parse commandline
	{
		// parse commandline arguments
		for (Size i = 1; i < arguments_count; ++i)
		{
			char *argument = arguments[i];
			if (argument[0] == '-')
			{
				// the argument is an option
				if (argument[1] == '-')
				{
					const char *option = &argument[2];
					if (compare_string(option, "print_tokens") == 0)
						compilation_options.print_tokens = true;
				}
				else
				{
					const char option = argument[1];
				}
			}
			else
			{
				// the argument is an source path

				// firstly, skip if the path already exists
				bool already_exists = 0;
				for (Size i = 0; i < sources_count; ++i)
				{
					char *path = ((char **)source_paths.pointer)[i];
					if (compare_string(path, argument) == 0)
						already_exists = 1;
				}
				if (already_exists)
					continue;

				char **reservation = (char **)reserve_from_buffer(&source_paths, sizeof(char *), reallocate);
				*reservation = argument;
				++sources_count;
			}
		}
	}

	// load the sources
	static Buffer sources, source_datas;
	initialize_buffer(&sources, sizeof(Source) * sources_count, reallocate);
	initialize_buffer(&source_datas, sources_count * get_memory_page_size(), reallocate);
	{
		// load sources
		for (Size i = 0; i < sources_count; ++i)
		{
			const char *file_path = ((char **)source_paths.pointer)[i];
		
			Handle file_handle;
			if (!open_file(&file_handle, file_path, false))
			{
				report_error("Failed to open source file: %s", file_path);
				continue;
			}

			Size file_size;
			if (!get_file_size(file_handle, &file_size))
			{
				close_file(file_handle);
				report_error("Failed to get source file size: %s", file_path);
				continue;
			}

			U8 *data_pointer = (U8 *)reserve_from_buffer(&source_datas, file_size + 1 /* null-terminator */, reallocate);
			Size read_size = file_size;
			if (!read_file(file_handle, data_pointer, &read_size))
			{
				close_file(file_handle);
				report_error("Failed to read source file: %s", file_path);
				continue;
			}
			if (read_size != file_size)
			{
				close_file(file_handle);
				report_error("Failed to read entire file: %s", file_path);
				continue;
			}
			data_pointer[read_size] = 0; // null-terminator
			
			Source *source = (Source *)reserve_from_buffer(&sources, sizeof(Source), 0);
			source->file_path = file_path;
			source->file_handle = file_handle;
			source->file_size = file_size;
			source->data_pointer = data_pointer;
		}
	}

	if (compilation_errors_count != 0)
	{
		goto finish;
	}

	for (Size i = 0; i < sources_count; ++i)
	{
		Source *source = &((Source *)sources.pointer)[i];

		report_debug("compiling %s:\n%s\n", source->file_path, source->data_pointer);

		Parser parser;
		initialize_parser(&parser, source);

		Array tokens;
		Size error_count = lex(&parser, &tokens);
		for (Size i = 0; i < tokens.size; ++i)
		{
			Token *token = &((Token *)tokens.pointer)[i];
			print("Token { type = %i; position = %lu; size = %lu }\n", token->type, token->position, token->size);
		}
	}

finish:
	terminate();
	return exit_code;
}

void initialize_parser(Parser *parser, const Source *source)
{
	set_memory(parser, sizeof(Parser), 0);
	parser->source = source;
}

Size lex(Parser *parser, Array *tokens)
{
	Size errors_count = 0;
	set_memory(tokens, sizeof(Array), 0);
	return errors_count;
}

Size get_alignment_addition(Address address, Size alignment)
{
	assert(alignment % 2 == 0);
	Size addition = 0;
	if (Size mod = address & (alignment - 1); mod != 0)
		addition = alignment - mod;
	return addition;
}

[[gnu::format(printf, 1, 2)]]
void print(const char *message, ...)
{
	va_list args;
	va_start(args, message);
	vprintf(message, args);
	va_end(args);
}

void report_error(const char *message, ...)
{
	++compilation_errors_count;

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

const char *get_system_error_message(void)
{
	return strerror(errno);
}

bool open_file(Handle *handle, const char *path, bool writable)
{
	int oflags = writable ? O_RDWR : O_RDONLY;
	int fd = open(path, oflags);
	if (fd == -1)
	{
		report_error("system: Failed to open file: %s", get_system_error_message());
		return 0;
	}
	*handle = fd;
	return 1;
}

void close_file(Handle handle)
{
	(void)close(handle);
}

bool get_file_size(Handle handle, Size *size)
{
	struct stat st;
	if (fstat(handle, &st) == -1)
	{
		report_error("system: Failed to get file size: %s", get_system_error_message());
		return 0;
	}
	*size = st.st_size;
	return 1;
}

bool read_file(Handle handle, void *buffer, Size *size)
{
	ssize_t r = read(handle, buffer, *size);
	if (r == -1)
	{
		*size = 0;
		report_error("system: Failed to read file: %s", get_system_error_message());
		return 0;
	}
	*size = r;
	return 1;
}

void initialize_array(Array *array, Size size, Reallocator *reallocator)
{
	array->pointer = 0;
	resize_array(array, size, reallocator);
}

void uninitialize_array(Array *array, Reallocator *reallocator)
{
	resize_array(array, 0, reallocator);
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

void uninitialize_buffer(Buffer *buffer, Reallocator *reallocator)
{
	uninitialize_array(buffer, reallocator);
	buffer->mass = 0;
}

void transform_buffer_into_array(Buffer *buffer, Array *array, Reallocator *reallocator)
{
	resize_array(buffer, buffer->mass, reallocator);
	move_memory(array, buffer, sizeof(Array));
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

void *reserve_from_buffer(Buffer *buffer, Size size, Reallocator *reallocator)
{
	void *end = ensure_buffer(buffer, size, reallocator);
	buffer->mass += size;
	return end;
}

// end of file stuff

// initialize any global objects/states
void initialize(void)
{
	setbuf(stdout, 0);
	setbuf(stderr, 0);
}

void terminate(void)
{
}
