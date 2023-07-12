#include "filetools.h"

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

int sd_FileExists(const char* file)
{
	struct stat buffer;
	return stat(file, &buffer) == 0;
}

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
		return 0; //return nullptr

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
				return 0;

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
		return 0;

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
