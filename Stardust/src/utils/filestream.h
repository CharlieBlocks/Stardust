#pragma once

#include "stardust.h"

#include "file.h"
//#include <stdio.h>

typedef struct
{
	struct File* file;
	long characterIndex;
	long eof;
} FileStream;

// Open/Close
StardustErrorCode fs_OpenStream(const char* file, FileStream* stream);
StardustErrorCode fs_CloseStream(FileStream* stream);

//Read
StardustErrorCode fs_ReadBytes(FileStream* stream, char* buf, long count);


//Read Types
char fs_ReadInt8(FileStream* stream);
unsigned int fs_ReadUint32(FileStream* stream);
