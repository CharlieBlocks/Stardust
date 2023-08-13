#ifndef _STARDUST_BITSTREAM
#define _STARDUST_BITSTREAM

#include "stardust.h"

typedef char sdBit;

typedef struct
{
	//Memory to stream from
	char* mem;
	unsigned long memSize; //Max size of the memory

	unsigned long currentByte; //Current byte in mem
	char currentBit;
	
} BitStream;

void bs_CreateBitStream(char* mem, unsigned long size, BitStream* stream);

char bs_GetBit(BitStream* stream);
char bs_GetBits(BitStream* stream, int count);

char bs_GetByte(BitStream* stream);
unsigned int bs_GetBytes(BitStream* stream, int count);

#endif //_STARDUST_BITSTREAM
