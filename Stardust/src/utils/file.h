#ifndef _FILE
#define _FILE

#include "stardust.h"

typedef enum
{
	FileMode_Read = 0,
	FileMode_Write = 1,
	FileMode_Append = 2,

	FileMode_ReadBinary = 3,
	FileMode_WriteBinary = 4,
	FileMode_AppendBinary = 5
} FileMode;

typedef enum
{
	FileOrigin_Start = 0,
	FileOrigin_Current = 1,
	FileOrigin_End = 2
} FileOrigin;

struct File;

StardustErrorCode f_FileExists(const char* path);

/// <summary>
/// Open a file.
/// Cannot chose createNew if using STD. It will automatically create a new file if given FileMode_Write or FileMode_Read
/// </summary>
/// <param name="path"></param>
/// <param name="mode"></param>
/// <param name="f"></param>
/// <param name="createNew"></param>
/// <returns></returns>
StardustErrorCode f_OpenFile(const char* path, FileMode mode, struct File** f);

/// <summary>
/// Close a file
/// </summary>
/// <param name="f"></param>
/// <returns></returns>
StardustErrorCode f_CloseFile(struct File* f);


//Move
void f_Seek(struct File* f, long offset, FileOrigin origin);
unsigned long f_Tell(struct File* f);


// Read

/// <summary>
/// Reads a given amount of bytes
/// </summary>
/// <param name="f"></param>
/// <param name="buffer"></param>
/// <param name="count"></param>
/// <returns>1 if it reaches EOF, otherwise 0</returns>
int f_ReadBytes(struct File* f, char* buffer, size_t count);

/// <summary>
/// Reads up until a newline/eof is hit or it reaches maxLen
/// </summary>
/// <param name="f"></param>
/// <param name="buffer"></param>
/// <param name="maxLen"></param>
void f_ReadLine(struct File* f, char* buffer, int maxLen);


#endif 
