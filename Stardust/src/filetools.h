#ifndef _STARDUST_FILE_TOOLS
#define _STARDUST_FILE_TOOLS

#include <stdio.h>

int sd_FileExists(const char* file);
char** sd_SplitString(char* str, const char delim, size_t* count);
void sd_FreeStringArray(char** str, size_t count);

#endif // _STARDUST_FILE_TOOLS