#include "filetools.h"

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#include <assert.h>

int sd_FileExists(const char* file)
{
	struct stat buffer;
	return stat(file, &buffer) == 0;
}

//Could potentially have some problems with memory where it won't produce a STARDUST_ERROR_MEMORY_ERROR if it goes wrong.
char** sd_SplitString(char* str, const char delim, size_t* count)
{
	char** result = 0; //Initialise memory to zero
	*count = 0; //Number of splits

	//Find ocourances of delim in str
	char* tmp = str;
	while (*tmp != 0) //While charcter is not equal to null terminator
	{
		if (*tmp == delim)
			(*count)++;
		tmp++;
	}

	(*count)++; //Increment by 1 to account for the trailing string

	//Allocate result array
	result = malloc(sizeof(char*) * (*count));
	//Validate malloc
	if (!result)
	{
		*count = 0;
		return 0; //return nullptr
	}

	//Iterate over string and create substrings
	tmp = str; //Reset to begining
	size_t lastChar = 0; //Distance to the previous character
	int idx = 0;

	while (*tmp != 0)
	{
		//Check if character is delim
		if (*tmp == delim)
		{
			//Copy previous into memory
			result[idx] = malloc(sizeof(char) * (lastChar + 1)); //Allocate string space + null terminator
			if (result[idx] == 0)
			{
				free(result);
				*count = 0;
				return 0;
			}

			strncpy_s(result[idx], sizeof(char) * lastChar + 1, tmp - lastChar, lastChar);

			lastChar = 0;
			idx++;
		}
		else
			lastChar++;
	
		tmp++;
	}

	//Add trailing string
	result[idx] = malloc(sizeof(char) * (lastChar + 1));
	if (result[idx] == 0)
	{
		for (int i = 0; i < (*count) - 1; i++)
			free(result[i]);
		free(result);
		*count = 0;
		return 0;
	}

	const char* start = tmp - lastChar;
	strncpy_s(result[idx], sizeof(char) * lastChar + 1, tmp - lastChar, lastChar);

	return result;
}

void sd_FreeStringArray(char** str, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		free(str[i]);
	}

	free(str);
}

StardustErrorCode sd_CreateFileStream(const char* file, FileStream** stream)
{
	*stream = malloc(sizeof(FileStream));
	if (*stream == 0)
		return STARDUST_ERROR_MEMORY_ERROR;

	errno_t error = fopen_s(&(*stream)->file, file, "rb");

	if (error != 0)
		return STARDUST_ERROR_IO_ERROR;

	(*stream)->characterIndex = 0;

	fseek((*stream)->file, 0, SEEK_END);
	(*stream)->eof = ftell((*stream)->file);
	fseek((*stream)->file, 0, SEEK_SET);

	return STARDUST_ERROR_SUCCESS;
}

void sd_CloseStream(FileStream* stream)
{
	fclose(stream->file);

	free(stream);
}

char sd_GetCharacter(FileStream* stream)
{
	char c;
	size_t eof = fread(&c, 1, 1, stream->file);

	stream->characterIndex++;

	return c;
}

StardustErrorCode sd_StreamGet(FileStream* stream, char* buffer, uint32_t count)
{
	//fgets(buffer, count, stream->file);
	size_t out = fread(buffer, 1, count, stream->file);

	stream->characterIndex += count;

	return STARDUST_ERROR_SUCCESS;
}

//TODO: ADD ERROR CHECKING
char* sd_GetRange(FileStream* stream, uint32_t len)
{
	char* str = malloc(sizeof(char) * len);
	assert(str);

	fread(str, 1, len, stream->file);

	stream->characterIndex += len;

	return str;
}

void sd_ReverseArray(char* arr, uint32_t len)
{
	for (uint32_t i = 0; i < len / 2; i++)
	{
		int inverse = len - i - 1;
		char temp = arr[i];
		arr[i] = arr[inverse];
		arr[inverse] = temp;
	}
}

int8_t sd_GetInt8(FileStream* stream)
{
	char c = sd_GetCharacter(stream);
	return (int8_t)c;
}

int16_t sd_GetInt16(FileStream* stream)
{
	int16_t val;
	sd_StreamGet(stream, (char*)&val, 2);

	return val;
}

int32_t sd_GetInt32(FileStream* stream)
{
	int32_t val;
	sd_StreamGet(stream, (char*)&val, 4);

	return val;
}

float sd_GetFloat(FileStream* stream)
{
	float val;
	sd_StreamGet(stream, (char*)&val, 4);

	return val;
}

double sd_GetDouble(FileStream* stream)
{
	double val;
	sd_StreamGet(stream, (char*)&val, 8);

	return val;
}

int64_t sd_GetInt64(FileStream* stream)
{
	int64_t val;
	sd_StreamGet(stream, (char*)&val, 8);

	return val;
}

uint32_t sd_GetUint32(FileStream* stream)
{
	uint32_t val;
	sd_StreamGet(stream, (char*)&val, 4);

	return val;
}

uint64_t sd_GetUint64(FileStream* stream)
{
	uint64_t val;
	sd_StreamGet(stream, (char*)&val, 8);

	return val;
}

float* sd_GetFloatArr(FileStream* stream, uint32_t size)
{
	/*float* arr = malloc(sizeof(float) * size);
	assert(arr != 0);

	for (uint32_t i = 0; i < size; i++)
		arr[i] = sd_GetFloat(stream);*/
	
	float* arr = malloc(sizeof(float) * size);
	assert(arr != 0);

	sd_StreamGet(stream, (char*)arr, size * 4);
	
	return arr;
}

double* sd_GetDoubleArr(FileStream* stream, uint32_t size)
{
	double* arr = malloc(sizeof(double) * size);
	assert(arr != 0);

	//for (uint32_t i = 0; i < size; i++)
	//	arr[i] = sd_GetDouble(stream);

	sd_StreamGet(stream, (char*)arr, size * 8);

	return arr;
}

int32_t* sd_GetIntArr(FileStream* stream, uint32_t size)
{
	int32_t* arr = malloc(sizeof(int32_t) * size);
	assert(arr != 0);

	//for (uint32_t i = 0; i < size; i++)
	//	arr[i] = sd_GetInt32(stream);

	sd_StreamGet(stream, (char*)arr, size * 4);

	return arr;
}

int64_t* sd_GetLongArr(FileStream* stream, uint32_t size)
{
	int64_t* arr = malloc(sizeof(int64_t) * size);
	assert(arr != 0);

	//for (uint32_t i = 0; i < size; i++)
		//arr[i] = sd_GetInt64(stream);

	sd_StreamGet(stream, (char*)arr, size * 8);

	return arr;
}

int8_t* sd_GetBoolArr(FileStream* stream, uint32_t size)
{
	int8_t* arr = malloc(sizeof(int8_t) * size);
	assert(arr != 0);

	//for (uint32_t i = 0; i < size; i++)
	//	arr[i] = sd_GetInt8(stream);

	sd_StreamGet(stream, (char*)arr, size);

	return arr;
}

