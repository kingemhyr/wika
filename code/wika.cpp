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
}
compilation_options;

Size compilation_errors_count = 0;

int main(int arguments_count, char **arguments)
{
	initialize();

	int exit_code = 0;

	if (arguments_count <= 1)
	{
		report_error("No source paths given");
		display_help();
		return 0;
	}

	static Arena sources;
	static Arena source_paths;
	static Arena source_datas;
	{
		initialize_arena(&sources, sizeof(Source));
		initialize_arena(&source_paths, 64);

		Size sources_count = 0;

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
					}
					else
					{
						const char option = argument[1];
					}
				}
				else
				{
					// the argument is a source path

					// get the full path
					char path_buffer[MAX_FILE_PATH_SIZE + 1];
					Size path_size = get_full_file_path(argument, path_buffer);
					if (!path_size)
					{
						report_error("Failed to get the full file path of source file: %s", path_buffer);
						continue;
					}
					path_buffer[path_size] = 0;

					// firstly, skip if the path already exists
					struct Iteration_Input
					{
						bool already_exists;
						const char *path_buffer;
						const Size path_size;
					}
					iteration_input = {0, path_buffer, path_size};
					iterate_over_arena(
						&sources,
						[](void *input_pointer, void *pointer) -> bool
						{
							Iteration_Input *input = (Iteration_Input *)input_pointer;
							Source *source = (Source *)pointer;
							if (source->path_size != input->path_size)
								return true;
							if (compare_memory(source->path, input->path_buffer, input->path_size) == 0)
							{
								input->already_exists = true;
								return false;
							}
							return true;
						},
						&iteration_input,
						sizeof(Source),
						alignof(Source));
					if (iteration_input.already_exists)
					{
						report_warning("Source path is already given: %s", path_buffer);
						continue;
					}

					char *path = (char *)reserve_from_arena(&source_paths, path_size);
					copy_memory(path, path_buffer, path_size);

					Source *source = (Source *)reserve_from_arena(&sources, sizeof(Source), alignof(Source));
					source->path_size = path_size;
					source->path = path;
					++sources_count;
				}
			}
		}

		// load the sources
		initialize_arena(&source_datas, sources_count * get_memory_page_size());
		iterate_over_arena(
			&sources,
			[](void *, void *pointer) -> bool
			{
				Source *source = (Source *)pointer;
				Handle handle;
				if (!open_file(&handle, source->path))
				{
					report_error("Failed to open source file: %s", source->path);
					return true;
				}

				Size data_size;
				if (!get_file_size(handle, &data_size))
				{
					close_file(handle);
					report_error("Failed to get source file size: %s", source->path);
					return true;
				}

				U8 *data = (U8 *)reserve_from_arena(&source_datas, data_size + 1);
				Size read_size = data_size;
				if (!read_file(handle, data, &read_size))
				{
					close_file(handle);
					report_error("Failed to read source file: %s", source->path);
					return true;
				}
				if (read_size != data_size)
				{
					close_file(handle);
					report_error("Failed to read entire file: %s", source->path);
					return true;
				}
				data[read_size] = 0;
		
				source->handle = handle;
				source->data_size = data_size;
				source->data = data;
				return true;
			},
			0,
			sizeof(Source),
			alignof(Source));
		if (compilation_errors_count != 0)
			terminate();
	}

	iterate_over_arena(
		&sources,
		[](void *, void *pointer) -> bool
		{
			Source *source = (Source *)pointer;

			print("Compiling %s...\n", source->path);

			Parser parser;
			initialize_parser(&parser, source);

			Size errors_count = parse(&parser);
			if (errors_count)
			{
			}
			return true;
		},
		0,
		sizeof(Source),
		alignof(Source));

	terminate();
	return exit_code;
}

Size format_token(char *buffer, Size size, const Token *token)
{
	char strbuf[16];
	set_memory(strbuf, sizeof(strbuf), 0);

	const char *fmt = "type = %i, position = %lu, size = %lu, representation = \"%s\"";
	char *representation = strbuf;

	switch (token->type)
	{
	case Token_Type_NONE:
		copy_string(strbuf, "");
		break;
	case Token_Type_IDENTIFIER:
		representation = token->representation;
		break;
	case Token_Type_PROC:
		copy_string(strbuf, "proc");
		break;
	default:
		strbuf[0] = token->type;
		break;
	}
	return format(buffer, size, fmt, token->type, token->position, token->size, representation);
}

void initialize_parser(Parser *parser, const Source *source)
{
	set_memory(parser, sizeof(Parser), 0);
	parser->location.source = source;
}

static Size advance(Parser *parser, U32 *codepoint)
{
	U8 *pointer = &parser->location.source->data[parser->location.position];
	Size size = decode_utf8(codepoint, pointer);
	if (!size)
	{
		report_error("Erroneous UTF-8 encoding");
		return 0;
	}
	parser->location.position += size;
	return size;

}

static bool check_whitespace(U32 codepoint)
{
	return iswspace(codepoint);
}

static bool check_letter(U32 codepoint)
{
	return iswalpha(codepoint);
}

static bool check_number(U32 codepoint)
{
	return iswdigit(codepoint);
}

static Token_Type lex(Parser *parser)
{
	Token *token = &parser->token;
	const Source *source = parser->location.source;
	U32 codepoint;
	Size increment;

	// skip whitespace
	do
		increment = advance(parser, &codepoint);
	while (check_whitespace(codepoint));

	token->position = parser->location.position - increment;

	// get the type
	switch (codepoint)
	{
	case Token_Type_NONE:
	case Token_Type_COLON:
	case Token_Type_SEMICOLON:
	case Token_Type_LEFT_PARENTHESIS:
	case Token_Type_RIGHT_PARENTHESIS:
	case Token_Type_LEFT_BRACE:
	case Token_Type_RIGHT_BRACE:
		token->type = (Token_Type)codepoint;
		token->size = 1;
		break;
	default:
		if (check_letter(codepoint) || codepoint == '_')
		{
			token->size = increment;
			do
			{
				token->size += advance(parser, &codepoint);
				if (!codepoint)
					break;
			}
			while (check_letter(codepoint) || codepoint == '_' || check_number(codepoint));
			token->size -= increment;

			// extract it as an identifier. (if it isn't an identifier, it'll be released later).
			token->representation = (char *)reserve_from_buffer(&parser->identifiers, token->size + 1);
			token->representation[token->size] = 0;
			copy_memory(token->representation, &source->data[token->position], token->size);

			// check if it's a keyword
			if (compare_string(token->representation, "proc") == 0)
				token->type = Token_Type_PROC;
			else
				token->type = Token_Type_IDENTIFIER;

			// release if it's an identifier
			if (token->type != Token_Type_IDENTIFIER)
				release_from_buffer(&parser->identifiers, token->size + 1);
		}
		else
		{
			token->type = Token_Type_NONE;
			token->size = 1;
			report_parsing_error(parser, "Unknown token: %c");
		}
		break;
	}

	// print the token
#if defined ENABLE_DEBUGGING
	{
		Size size = format_token(0, 0, token);
		Array string;
		initialize_array(&string, size + 1, 0);
		((char *)string.pointer)[size] = 0;
		format_token((char *)string.pointer, size + 1, token);
		uninitialize_array(&string);
	}
#endif

	return token->type;
}

Size parse(Parser *parser)
{
	Size errors_count = 0;

	for (;;)
	{
		Token_Type type = lex(parser);
		if (type == Token_Type_NONE)
			break;
	}

	if (errors_count)
		return errors_count;
	
	return 0;
}

void report_parsing_error(Parser *parser, const char *message, ...)
{
	++compilation_errors_count;
	fprintf(stderr, "\e[1m%s:%lu: \e[31merror:\e[0m ", parser->location.source->path, parser->location.position);
	va_list args;
	va_start(args, message);
	vfprintf(stderr, message, args);
	va_end(args);
	Size position = parser->location.position;
	char *pointer = (char *)parser->location.source->data;
	for (; position != 0; --position)
	{
		if (pointer[position - 1] == '\n')
			break;
	}
	pointer += position;
	char *end = find_character(pointer, '\n');
	Size linesz = end - pointer;
	char buf[linesz + 1];
	copy_memory(buf, pointer, linesz);
	buf[linesz + 16] = 0;
	Size offset = parser->location.position - position - 1;
	const char esc[] = "\e[1;31m";
	Size escsz = sizeof(esc) - 1;
	{
		char *p = buf + offset;
		move_memory(p + escsz, p, linesz - offset);
		buf[linesz + escsz] = 0;
		copy_memory(p, esc, escsz);
	}
	{
		const char esc2[] = "\e[0m";
		Size esc2sz = sizeof(esc2) - 1;
		char *p = buf + offset + escsz + 1;
		move_memory(p + esc2sz, p, linesz - offset);
		buf[linesz + escsz + esc2sz] = 0;
		copy_memory(p, esc2, esc2sz);
	}
	fprintf(stderr, "\n| %s\n  ", buf);


	for (Size i = 0; i < offset; ++i)
		putc(' ', stderr);

	fprintf(stderr, "\e[1;31m");
	for (Size i = 0; i < parser->token.size; ++i)
		putc('^', stderr);
	fprintf(stderr, "\e[0m\n");
}

Size get_alignment_addition(Address address, Size alignment)
{
	assert(alignment % 2 == 0);
	Size addition = 0;
	if (Size mod = address & (alignment - 1); mod != 0)
		addition = alignment - mod;
	return addition;
}

Size get_alignment_subtraction(Address address, Size alignment)
{
	assert(alignment % 2 == 0);
	Size subtraction = 0;
	if (Size mod = address & (alignment - 1); mod != 0)
		subtraction = mod;
	return subtraction;
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

void report_warning(const char *message, ...)
{
	fprintf(stderr, "warning: ");
	va_list args;
	va_start(args, message);
	vfprintf(stderr, message, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void debug(const char *message, ...)
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

static Size total_program_memory_allocation_size = 0;

void *allocate(Size size)
{
	void *result = malloc(size);
	if (result)
		total_program_memory_allocation_size += size;
	return result;
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
	else
		total_program_memory_allocation_size += align(size, get_memory_page_size());
	return result;
}

void deallocate_virtual_memory(void *pointer, Size size)
{
	if (munmap(pointer, size) == -1)
	{
		// idek
	}
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

Size get_full_file_path(const char *path, char *buffer)
{
	char pathbuf[MAX_FILE_PATH_SIZE + 1];
	if (!realpath(path, pathbuf))
	{
		report_error("system: Failed to get full file path: %s", get_system_error_message());
		return 0;
	}
	Size length = get_length_of_string(pathbuf);
	if (buffer)
		copy_memory(buffer, pathbuf, length);
	return length;
}

void initialize_array(Array *array, Size size, void *pointer)
{
	if (pointer)
	{
		array->pointer = pointer;
		array->size = size;
	}
	else
	{
		array->pointer = 0;
		resize_array(array, size);
	}
}

void uninitialize_array(Array *array)
{
	resize_array(array, 0);
}

void resize_array(Array *array, Size new_size)
{
	array->pointer = (U8 *)reallocate(array->pointer, new_size);
	array->size = new_size;
}

void initialize_buffer(Buffer *buffer, Size size, void *pointer)
{
	initialize_array(buffer, size, pointer);
	buffer->mass = 0;
}

void uninitialize_buffer(Buffer *buffer)
{
	uninitialize_array(buffer);
	buffer->mass = 0;
}

void transform_buffer_into_array(Buffer *buffer, Array *array)
{
	resize_array(buffer, buffer->mass);
	move_memory(array, buffer, sizeof(Array));
}

void expand_buffer(Buffer *buffer, Size size)
{
	Size new_size = size + buffer->size + buffer->size / 2;
	resize_array(buffer, new_size);
}

void *ensure_buffer(Buffer *buffer, Size size)
{
	Size space = buffer->size - buffer->mass;
	if (space < size)
		expand_buffer(buffer, size);
	return &((U8 *)buffer->pointer)[buffer->mass];
}

void *reserve_from_buffer(Buffer *buffer, Size size, Size alignment)
{
	Size addition = get_alignment_addition((Address)buffer->pointer + buffer->mass, alignment);
	U8 *end = (U8 *)ensure_buffer(buffer, alignment + size);
	buffer->mass += addition;
	void *result = end + buffer->mass;
	buffer->mass += size;
	return result;
}

void release_from_buffer(Buffer *buffer, Size size, Size alignment)
{
	buffer->mass -= size;
	Size subtraction = get_alignment_subtraction((Address)buffer->pointer + buffer->mass, alignment);
	buffer->mass -= subtraction;
	// TODO: maybe we should assert that size+alignment <= buffer->mass?
}

template<typename T>
void attach_singly(Singly<T> *singly, Singly<T> *other)
{
	if (singly->other && other)
		other->other = singly->other;
	singly->other = other;
}

static void initialize_arena_buffer(Arena_Buffer *buffer, Size size)
{
	buffer->pointer = buffer;
	buffer->size = size;
	buffer->other = 0;
	buffer->mass = sizeof(Arena_Buffer);
}

void initialize_arena(Arena *arena, Size size)
{
	size = align(sizeof(Buffer) + size, get_memory_page_size());
	arena->first = (Arena_Buffer *)allocate_virtual_memory(0, size);
	initialize_arena_buffer(arena->first, size);
	arena->last = arena->first;
}

void uninitialize_arena(Arena *arena)
{
	Arena_Pointer pointer =
	{
		.arena = arena,
		.buffer = arena->first,
		.offset = 0,
	};
	set_arena(&pointer);
}

void *reserve_from_arena(Arena *arena, Size size, Size alignment)
{
	Arena_Buffer *buffer = arena->last;
	Size addition = get_alignment_addition((Address)buffer->pointer + buffer->mass, alignment);
	Size space = buffer->size - buffer->mass;
	if (space < addition + size)
	{

		addition = get_alignment_addition(get_memory_page_size(), alignment);
		Size buffer_size = align(addition + size, get_memory_page_size());
		buffer = (Arena_Buffer *)allocate_virtual_memory(0, buffer_size);
		initialize_arena_buffer(buffer, size);
		attach_singly(arena->last, buffer);
		arena->last = buffer;
	}
	buffer->mass += addition;
	void *result = (U8 *)buffer->pointer + buffer->mass;
	buffer->mass += size;
	return result;
}

void iterate_over_arena(Arena *arena, bool (*procedure)(void *input, void *i), void *input, Size size, Size alignment)
{
	Arena_Buffer *buffer = arena->first;

	Size count = 0;
	do
	{
		Size initial_offset = sizeof(Arena_Buffer) + get_alignment_addition((Address)buffer->pointer + sizeof(Arena_Buffer), alignment);
		for (Size i = initial_offset; i < buffer->mass; i += size)
		{
			if (!procedure(input, (U8 *)buffer->pointer + i))
				goto stop;
		}
		buffer = buffer->other;
	}
	while (buffer);
stop:
	return;
}

void get_arena_pointer(Arena *arena, Arena_Pointer *pointer)
{
	assert(arena->last);

	*pointer =
	{
		.arena = arena,
		.buffer = arena->last,
		.offset = arena->last->mass,
	};
}

void set_arena(Arena_Pointer *pointer)
{
	Arena_Buffer *buffer = pointer->buffer->other;
	while (buffer)
	{
		Arena_Buffer *other = buffer->other;
		deallocate_virtual_memory(buffer, buffer->size);
		buffer = other;
	}
	pointer->arena->last = pointer->buffer;
	pointer->buffer->mass = pointer->offset;
}

constexpr U8 utf8_class_table[32] =
{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2,
	3, 3,
	4,
	5,
};

Size decode_utf8(U32 *codepoint, const U8 *string)
{
	Size byte_class = utf8_class_table[string[0] >> 3];
	switch (byte_class)
	{
	case 1:
		*codepoint = string[0];
		break;
	case 2:
		if (!utf8_class_table[string[1] >> 3])
		{
			*codepoint  = (string[0] & LMASK5) << 6 |
			              (string[1] & LMASK6) << 0;
		}
		break;
	case 3:
		if (!utf8_class_table[string[1] >> 3] &&
		    !utf8_class_table[string[2] >> 3])
		{
			*codepoint = (string[0] & LMASK4) << 12 |
			             (string[1] & LMASK6) << 6 |
			             (string[2] & LMASK6) << 0;
		}
		break;
	case 4:
		if (!utf8_class_table[string[1] >> 3] &&
		    !utf8_class_table[string[2] >> 3] &&
		    !utf8_class_table[string[3] >> 3])
		{
			*codepoint = (string[0] & LMASK3) << 18 |
			             (string[1] & LMASK6) << 12 |
			             (string[2] & LMASK6) << 6 |
			             (string[3] & LMASK6) << 0;
		}
		break;
	default:
		byte_class = 0;
		break;
	}
	return byte_class;
}

Size get_utf8_size(U32 codepoint)
{
	Size size =
		codepoint <= 0x7f     ? 1 :
        codepoint <= 0x7ff    ? 2 :
    	codepoint <= 0xffff   ? 3 :
    	codepoint <= 0x10ffff ? 4 :
		0;
	return size;
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
