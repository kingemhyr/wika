/*
	* scanning data
	* transforming data
	* allocating data
	* retrieving data
*/

// headers
#include "wika.h"
#include "wika_platform.h"

// sources
#include "wika_platform.cpp"

constexpr size_t DEFAULT_ALIGNMENT = 8;

size_t get_alignment_addition(uintptr_t address, size_t alignment)
{
	assert(alignment % 2 == 0);
	size_t mod = address & (alignment - 1);
	size_t addition = 0;
	if (mod)
	{
		addition = alignment - mod;
	}
	return addition;
}

uintptr_t align(uintptr_t address, size_t alignment)
{
	return address + get_alignment_addition(address, alignment);
}

void copy_memory(void *destination, const void *source, size_t size)
{
	std::memcpy(destination, source, size);
}

size_t get_string_length(const char *string)
{
	return std::strlen(string);
}

template<typename T>
concept LinearAllocator =
	requires(size_t s)
	{
		{ T::allocate(s) } -> std::same_as<void *>;
		{ T::deallocate(s) } -> std::same_as<void>;
	};

template<typename T>
concept VariableAllocator =
	requires(void *p, size_t s)
	{
		{ T::allocate(s) } -> std::same_as<void *>;
		{ T::allocate(p, s) } -> std::same_as<void *>;
		{ T::reallocate(p, s, s) } -> std::same_as<void *>;
		{ T::deallocate(p, s) } -> std::same_as<void>;
	};

template<typename T>
concept HeapAllocator =
	requires(void *p, size_t s)
	{
		{ T::allocate(s) } -> std::same_as<void *>;
		{ T::deallocate(p) } -> std::same_as<void>;
	};

template<VariableAllocator Allocator>
struct Array
{
	uint8_t *pointer;
	size_t size;

	void resize(size_t size)
	{
		this->pointer = (uint8_t *)Allocator::reallocate(this->pointer, this->size, size);
		this->size = size;
	}

	void expand(size_t minimum_size = 0)
	{
		this->resize(this->size + minimum_size + this->size / 2);
	}
};

template<VariableAllocator A>
struct Buffer : Array<A>
{
	size_t mass;

	size_t space()
	{
		return this->size - this->mass;
	}

	void *end()
	{
		return &this->pointer[this->mass];
	}

	void subtract(size_t size)
	{
		assert(size <= this->mass);
		this->mass -= size;
	}

	template<typename T>
	T *reserve(size_t count = 1, size_t alignment = DEFAULT_ALIGNMENT)
	{
		size_t size = count * sizeof(T);
		size_t space = this->space();
		size_t alignment_addition = get_alignment_addition((uintptr_t)this->end(), alignment);
		size_t additional_mass = size + alignment_addition;
		if (space < additional_mass)
		{
			this->expand(additional_mass);
		}
		this->mass += alignment;
		void *end_pointer = this->end();
		this->mass += size;
		return (T *)end_pointer;
	}
};

template<typename T>
struct Doubly : T
{
	Doubly *prior;
	Doubly *next;

	void detach()
	{
		if (this->prior)
			this->prior->next = next;
		if (this->next)
			this->next->prior = this->prior;
	}

	void attach(Doubly *next)
	{
		next->next = prior->next;
		next->prior = this;
		this->next->prior = next;
		this->next = next;
	}
};

void *allocate_virtual_memory(void *address, size_t size)
{
	void *result = ::mmap(address, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (result == MAP_FAILED)
	{
		result = 0;
	}
	return result;
}

void deallocate_virtual_memory(void *address, size_t size)
{
	(void)::munmap(address, size);
}

struct Virtual_Memory_Allocator
{
	static void *allocate(void *pointer, size_t size)
	{
		return allocate_virtual_memory(pointer, size);
	}

	static void *allocate(size_t size)
	{
		return allocate_virtual_memory(0, size);
	}

	static void *reallocate(void *pointer, size_t size, size_t new_size)
	{
		void *new_memory = allocate_virtual_memory(0, new_size);
		copy_memory(new_memory, pointer, size);
		deallocate_virtual_memory(pointer, size);
		return new_memory;
	}

	static void deallocate(void *pointer, size_t size)
	{
		deallocate_virtual_memory(pointer, size);
	}
};

size_t get_memory_page_size()
{
	return getpagesize();
}

struct ArenaBuffer : Doubly<Buffer<Virtual_Memory_Allocator>>
{
	void *operator new(size_t size, size_t minimum_size)
	{
		size = align(size + minimum_size, get_memory_page_size());
		ArenaBuffer *pointer = (ArenaBuffer *)allocate_virtual_memory(0, size);
		pointer->pointer = (uint8_t *)pointer;
		pointer->size = size;
		pointer->mass = 0;
		return pointer;
	}

	void *operator new[](size_t) = delete;

	void operator delete(void *p)
	{
		ArenaBuffer *pointer = (ArenaBuffer *)p;
		deallocate_virtual_memory(pointer, pointer->size);
	}
};

struct ArenaFlag
{
	ArenaBuffer *buffer;
	size_t offset;
};

struct Arena : ArenaBuffer
{
	ArenaBuffer *last;

	template<typename T>
	T *allocate(size_t count = 1, size_t alignment = DEFAULT_ALIGNMENT)
	{
		size_t size = count * sizeof(T);
		size_t space = this->last->space();
		size_t alignment_addition = get_alignment_addition((uintptr_t)this->last->end(), alignment);
		size_t additional_mass = alignment_addition + size;
		if (space < additional_mass)
		{
			ArenaBuffer *new_arena = new(additional_mass) ArenaBuffer;
			this->last->attach(new_arena);
			this->last = new_arena;
		}
		this->last->mass += alignment_addition;
		void *end = this->last->end();
		this->last->mass += size;
		return end;
	}

	template<typename T>
	void deallocate(size_t count = 1)
	{
		size_t size = count * sizeof(T);
		this->last->subtract(size);
		if (!this->last->mass && this->last->prior)
		{
			ArenaBuffer *new_last = static_cast<ArenaBuffer *>(this->last->prior);
			this->last->detach();
			this->last = new_last;
		}
	}

	// this assumes that `flag.buffer` is within the arena
	void set(ArenaFlag flag)
	{
		this->last = flag.buffer;
		this->last->mass = flag.offset;
	}
};

struct Source
{
	Handle handle;
	char *path;
	size_t path_size;
	uint8_t *data;
	size_t data_size;
};

Arena sources;
Arena sources_path;
Arena sources_data;

int main(int arguments_count, char **arguments)
{
	int exit_code = 0;
	const char source_path[] = "/home/emhyr/code/wika/examples/hello.w";
	{
		set_termination_on_error(true);
		size_t path_size = get_string_length(source_path);
		char *path = sources_path.reserve<char>(path_size + 1);
		copy_memory(path, source_path, path_size);
		path[path_size] = 0;
		Handle handle = open_file(path);
		ssize_t data_size = get_file_size(handle);
		uint8_t *data = sources_data.reserve<uint8_t>(data_size + 1);
		ssize_t read_size = read_file(data, data_size, handle);
		if (read_size != data_size)
		{
			log_error("Failed to read entire file");
			terminate();
		}
		data[data_size] = 0;
		Source *source = sources.reserve<Source>();
		source->handle = handle;
		source->path = path;
		source->path_size = path_size;
		source->data = data;
		source->data_size = data_size;
		set_termination_on_error(false);
		printf("%s", data);
	}

	return exit_code;
}

void log_error(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}

static bool termination_on_error = false;

void set_termination_on_error(bool value)
{
	termination_on_error = value;
}

bool get_termination_on_error(void)
{
	return termination_on_error;
}

void terminate(void)
{

}
