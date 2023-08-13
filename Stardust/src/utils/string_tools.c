#include "string_tools.h"
#include <stdlib.h>
#include <string.h>

//Could potentially have some problems with memory where it won't produce a STARDUST_ERROR_MEMORY_ERROR if it goes wrong.
char** s_SplitString(char* str, const char delim, unsigned long long* count)
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
	unsigned long long lastChar = 0; //Distance to the previous character
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
			//memcpy(result[idx], tmp - lastChar, lastChar);
			//s_StrCpy(result[idx], tmp - lastChar, lastChar);

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
	//s_StrCpy(result[idx], tmp - lastChar, lastChar);
	//memcpy(result[idx], tmp - lastChar, lastChar);

	return result;
}

void s_FreeStringArray(char** str, unsigned long long count)
{
	for (unsigned long long i = 0; i < count; i++)
	{
		free(str[i]);
	}

	free(str);
}

int s_StrCmp(char* a, char* b)
{
	int idx = 0;
	while (1)
	{
		if (a[idx] != b[idx])
			return 0;

		if (a[idx] == 0 || b[idx] == 0)
			break;

		idx++;
	}

	return 1;
}

void s_StrCpy(char* dst, char* src)
{
	int idx = 0;
	while (1)
	{
		dst[idx] = src[idx];

		if (src[idx] == 0)
			return;

		idx++;
	}

}

