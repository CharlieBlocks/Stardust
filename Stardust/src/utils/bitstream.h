#ifndef _STARDUST_BITSTREAM
#define _STARDUST_BITSTREAM

#include "stardust.h"

typedef char sdBit;

typedef struct
{
	//Memory to stream from
	unsigned char* mem;
	unsigned long memSize; //Max size of the memory

	unsigned long currentByte; //Current byte in mem
	char currentBit;
	
} BitStream;

void bs_CreateBitStream(unsigned char* mem, unsigned long size, BitStream* stream);

unsigned char bs_ReadBit(BitStream* stream);
unsigned char bs_ReadBits(BitStream* stream, int count);

unsigned char bs_ReadByte(BitStream* stream);
unsigned int bs_ReadBytes(BitStream* stream, int count);

#endif //_STARDUST_BITSTREAM
