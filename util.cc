#include "util.h"

uint64_t sizeoffile(FILE *handle)
{
	uint64_t filesize;
	long oldpos = ftell(handle);
	fseek(handle, 0, SEEK_END);
	filesize = ftell(handle);
	fseek(handle, oldpos, SEEK_SET);
	return filesize;
}

size_t readcontent_unlocked(FILE *handle, char *p, uint64_t isize, int *end)
{
	long oldpos = ftell(handle);
	uint64_t size = fread_unlocked(p, 1, isize, handle);
	if (size < isize)
	{
		*end = 1;
	}
	else
	{
		*end = 0;
	}
	while (size > 0 && p[size - 1] != '\n')
	{
		--size;
	}
	fseek(handle, oldpos + size, SEEK_SET);
	return size;
}

int setfilesplitsize(uint64_t inputfilesize, int maxsort)
{
	uint64_t filesplitsize;
	filesplitsize = inputfilesize / maxsort + 129;
	if (filesplitsize > FILESPLITSIZE)
	{
		filesplitsize = FILESPLITSIZE;
	}
	return (uint32_t)filesplitsize;
}