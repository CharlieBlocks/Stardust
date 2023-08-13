#ifdef _STARDUST_WIN32
#ifndef _FILE_WIN32
#define _FILE_WIN32

#include "file.h"

#include <Windows.h>

struct File
{
	HANDLE file;
};

DWORD _win32_GetReadWrite(FileMode mode)
{
	switch (mode)
	{
	case FileMode_Read: return GENERIC_READ;
	case FileMode_Write: return GENERIC_WRITE;
	case FileMode_Append : return GENERIC_WRITE;

	case FileMode_ReadBinary: return GENERIC_READ;
	case FileMode_WriteBinary: return GENERIC_WRITE;
	case FileMode_AppendBinary: return GENERIC_WRITE;
	}

	return 0;
}

DWORD _win32_GetCreationFlags(FileMode mode)
{
	if (mode == FileMode_Read || mode == FileMode_ReadBinary)
		return OPEN_EXISTING;
	else if (mode == FileMode_Write || mode == FileMode_WriteBinary)
		return CREATE_ALWAYS;
	else if (mode == FileMode_Append || mode == FileMode_AppendBinary)
		return OPEN_ALWAYS;

	return OPEN_EXISTING;
}

StardustErrorCode f_FileExists(const char* path)
{
	DWORD dwAttrib = GetFileAttributesA(path);

	int exists = (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
	if (exists)
		return STARDUST_ERROR_SUCCESS;
	return STARDUST_ERROR_FILE_NOT_FOUND;
}

StardustErrorCode f_OpenFile(const char* path, FileMode mode, struct File** f)
{
	//Get parameters
	DWORD readWrite = _win32_GetReadWrite(mode);
	DWORD createFlag = _win32_GetCreationFlags(mode);

	//Allocate file
	*f = malloc(sizeof(struct File));
	if (*f == 0)
		return STARDUST_ERROR_MEMORY_ERROR;

	//Open file
	(*f)->file = CreateFileA(
		path,				// Path to the file
		readWrite,			// Read/Write access
		FILE_SHARE_READ,	// What kind of lock can we hold. In this case we are able to read the file while we read/write to it
		NULL,				// Secuirty attributes
		createFlag,			// Create file or not?
		FILE_ATTRIBUTE_NORMAL, //Creation Attributes
		NULL				// Template
		);

	return STARDUST_ERROR_SUCCESS;
}

StardustErrorCode f_CloseFile(struct File* f)
{
	int b = CloseHandle(f->file);
	free(f);
	if (b == 0)
		return STARDUST_ERROR_IO_ERROR;
	return STARDUST_ERROR_SUCCESS;
}

void f_Seek(struct File* f, long offset, FileOrigin origin)
{
	DWORD method = 0;
	switch (origin)
	{
	case FileOrigin_Start: method = FILE_BEGIN; break;
	case FileOrigin_Current: method = FILE_CURRENT; break;
	case FileOrigin_End: method = FILE_END; break;
	}

	//Did someone say error catching? I didn't.
	SetFilePointer(
		f->file,
		offset,
		NULL,
		method
	);
}

unsigned long f_Tell(struct File* f)
{
	return SetFilePointer(
		f->file,
		0,
		NULL,
		FILE_CURRENT
	);
}

int f_ReadBytes(struct File* f, char* buffer, size_t count)
{
	DWORD bytesRead = 0;
	int b = ReadFile(
		f->file,
		buffer,
		(DWORD)count,
		&bytesRead,
		NULL
	);


	if (b == 0 || bytesRead != count)
		return 1;
	return 0;
}

void f_ReadLine(struct File* f, char* buffer, int maxLen)
{
	int idx = 0;
	DWORD bytesRead = 0;
	int stop = 0;

	while (1)
	{
		int err = ReadFile(f->file, buffer + idx, 1, &bytesRead, NULL);

		if (!err)
			return;
		if (*(buffer + idx) == '\n')
			return;
		
		idx++;
	}
}





#endif //_FILE_WIN32
#endif //_WIN32_