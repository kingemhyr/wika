#include "wika_platform.h"

Handle open_file(const char *path, bool writable)
{
	int oflags = O_RDONLY;
	if (writable)
	{
		oflags = O_RDWR;
	}
	Handle result = open(path, oflags);
	if (result == -1)
	{
		log_error("failed to invoke open file: %s", strerror(errno));
		if (get_termination_on_error())
		{
			terminate();
		}
	}
	return result;
}

ssize_t get_file_size(Handle file)
{
	struct stat st;
	if (fstat(file, &st) < 0)
	{
		log_error("failed to get file size: %s", strerror(errno));
		if (get_termination_on_error())
		{
			terminate();
		}
		return -1;
	}
	return st.st_size;
}

ssize_t read_file(void *buffer, size_t size, Handle file)
{
	ssize_t result = read(file, buffer, size);
	if (result < 0)
	{
		log_error("failed to read file: %s", strerror(errno));
		if (get_termination_on_error())
		{
			terminate();
		}
	}
	return result;
}
