#include "bitstream.h"

void bs_CreateBitStream(unsigned char* mem, unsigned long size, BitStream* stream)
{
	stream->mem = mem;
	stream->memSize = size;

	stream->currentByte = 0;
	stream->currentBit = 0;
}

unsigned char bs_ReadBit(BitStream* stream)
{
	if (stream->currentBit > 7)
	{
		stream->currentByte++;
		stream->currentBit = 0;
	}

	return (*(stream->mem + stream->currentByte) & (1 << stream->currentBit)) >> stream->currentBit++;
}

unsigned char bs_ReadBits(BitStream* stream, int count)
{
	char val = 0;
	for (int i = 0; i < count; i++)
		val |= bs_ReadBit(stream) << i;

	return val;
}

unsigned char bs_ReadByte(BitStream* stream)
{
	stream->currentBit = 0;
	return *(stream->mem + stream->currentByte++);
}

unsigned int bs_ReadBytes(BitStream* stream, int count)
{
	int val = 0;
	for (int i = 0; i < 4 || i < count; i++)
		val |= bs_ReadByte(stream) << (8 * i);
	
	return val;
}


