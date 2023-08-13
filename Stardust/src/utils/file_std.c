#ifdef _STARDUST_STD
#ifndef _FILE_STD
#define _FILE_STD

#include "file.h"

#include <stdio.h>
#include <stdlib.h>

struct File
{
	FILE* file;
};

static const char* f_fileModeStrings[] = {"r", "w", "a", "rb", "wb", "ab"};

StardustErrorCode f_OpenFile(const char* path, FileMode mode, struct File** f)
{
	*f = malloc(sizeof(struct File));
	if (*f == 0)
		return STARDUST_ERROR_MEMORY_ERROR;

	int err = fopen_s(&(*f)->file, path, f_fileModeStrings[mode]);
	if (err == 0)
		return STARDUST_ERROR_SUCCESS;
	return STARDUST_ERROR_IO_ERROR;
}

StardustErrorCode f_CloseFile(struct File* f)
{
	int err = fclose(f->file);
	free(f);
	if (err == 0)
		return STARDUST_ERROR_SUCCESS;
	return STARDUST_ERROR_IO_ERROR;
}

void f_Seek(struct File* f, long offset, FileOrigin origin)
{
	int method = 0;
	switch (origin)
	{
	case FileOrigin_Start: method = SEEK_SET;
	case FileOrigin_Current: method = SEEK_CUR;
	case FileOrigin_End: method = SEEK_END;
	}

	fseek(f->file, offset, method);
}

unsigned long f_Tell(struct File* f)
{
	return (unsigned long)ftell(f->file);
}

int f_ReadBytes(struct File* f, char* buffer, size_t count)
{
	size_t o = fread(buffer, 1, count, f->file);
	if (o != count)
		return 1; //EOF
	return 0;
}

void f_ReadLine(struct File* f, char* buffer, int maxLen)
{
	fgets(buffer, maxLen, f->file);
}

#endif //_FILE_STD
#endif //_STD_
