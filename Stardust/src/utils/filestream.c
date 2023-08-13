#include "filestream.h"

StardustErrorCode fs_OpenStream(const char* path, FileStream* stream)
{
	StardustErrorCode ret;
	
	ret = f_OpenFile(path, FileMode_ReadBinary, &stream->file);
	if (ret != 0)
		return ret;

	//Set character index
	stream->characterIndex = 0;

	//Get file length
	f_Seek(stream->file, 0, FileOrigin_End);
	stream->eof = f_Tell(stream->file);
	f_Seek(stream->file, 0, FileOrigin_Start);

	return STARDUST_ERROR_SUCCESS;
}

StardustErrorCode fs_CloseStream(FileStream* stream)
{
	StardustErrorCode ret = f_CloseFile(stream->file);
	return ret;
}

StardustErrorCode fs_ReadBytes(FileStream* stream, char* buf, long count)
{
	int eof = f_ReadBytes(stream->file, buf, count);
	stream->characterIndex += count;
	if (eof)
		return STARDUST_ERROR_EOF;
	return STARDUST_ERROR_SUCCESS;
}

char fs_ReadInt8(FileStream* stream)
{
	char c;
	fs_ReadBytes(stream, &c, 1);

	return c;
}

unsigned int fs_ReadUint32(FileStream* stream)
{
	unsigned int val;
	fs_ReadBytes(stream, (char*)&val, 4);

	return val;
}

