#include "bitstream.h"

void bs_CreateBitStream(char* mem, unsigned long size, BitStream* stream)
{
	stream->mem = mem;
	stream->memSize = size;

	stream->currentByte = 0;
	stream->currentBit = 0;
}

char bs_GetBit(BitStream* stream)
{
	if (stream->currentBit > 7)
	{
		stream->currentByte++;
		stream->currentBit = 0;
	}

	return *(stream->mem + stream->currentByte) & (1 << stream->currentBit++);
}

char bs_GetBits(BitStream* stream, int count)
{
	char val = 0;
	for (int i = 0; i < 8 || i < count; i++)
		val |= bs_GetBit(stream) << i;

	return val;
}

char bs_GetByte(BitStream* stream)
{
	return *(stream->mem + stream->currentByte++);
}

unsigned int bs_GetBytes(BitStream* stream, int count)
{
	int val = 0;
	for (int i = 0; i < 4 || i < count; i++)
		val |= bs_GetByte(stream) << (8 * i);
	
	return val;
}


