#ifndef _STARDUST_FILE_TOOLS
#define _STARDUST_FILE_TOOLS

#include <stdio.h>
#include <stdint.h>

#include "stardust.h"

typedef struct
{
	FILE* file;
	size_t characterIndex;
	size_t eof;
} FileStream;


int sd_FileExists(const char* file);
char** sd_SplitString(char* str, const char delim, size_t* count);
void sd_FreeStringArray(char** str, size_t count);


StardustErrorCode sd_CreateFileStream(const char* file, FileStream** stream);
void sd_CloseStream(FileStream* stream);

char sd_GetCharacter(FileStream* stream);
StardustErrorCode sd_StreamGet(FileStream* stream, char* buffer, uint32_t count);
char* sd_GetRange(FileStream* stream, uint32_t len);

void sd_ReverseArray(char* arr, uint32_t len);

int8_t sd_GetInt8(FileStream* stream);
int16_t sd_GetInt16(FileStream* stream);
int32_t sd_GetInt32(FileStream* stream);
float sd_GetFloat(FileStream* stream);
double sd_GetDouble(FileStream* stream);
int64_t sd_GetInt64(FileStream* stream);

uint32_t sd_GetUint32(FileStream* stream);
uint64_t sd_GetUint64(FileStream* stream);

float* sd_GetFloatArr(FileStream* stream, uint32_t size);
double* sd_GetDoubleArr(FileStream* stream, uint32_t size);
int32_t* sd_GetIntArr(FileStream* stream, uint32_t size);
int64_t* sd_GetLongArr(FileStream* stream, uint32_t size);
int8_t* sd_GetBoolArr(FileStream* stream, uint32_t size);

#endif // _STARDUST_FILE_TOOLS