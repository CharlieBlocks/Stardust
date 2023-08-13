#ifndef _STARDUST_STRINGTOOLS
#define _STARDUST_STRINGTOOLS

char** s_SplitString(char* str, const char delim, unsigned long long* count);
void s_FreeStringArray(char** str, unsigned long long count);

int s_StrCmp(char* a, char* b);
void s_StrCpy(char* dst, char* src);


#endif //_STARDUST_STRINGTOOLS
