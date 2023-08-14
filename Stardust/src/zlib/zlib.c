#include "zlib.h"

#include <stdlib.h>


StardustErrorCode zlib_Inflate(char** dstBuffer, unsigned long* uncompressedLength, char* srcBuffer)
{
	StardustErrorCode ret;
	
	// Create BitStream using srcBuffer
	BitStream stream;
	bs_CreateBitStream(
		srcBuffer, 
		0, 
		&stream
	); // Creates a bit stream on the source buffer.

	
	// Validate header. 
	ret = zlib_ValidateContainer(&stream);
	if (ret != 0) // Check that the return code was a success (STARDUST_ERROR_SUCCESS = 0)
		return ret;


	// Create ZLIBResult
	ZLIBResult uncompressedResult; // Defined on stack as it isn't more than an array of pointers
	uncompressedResult.blockCount = 0; uncompressedResult.capacity = 0; uncompressedResult.data = 0; uncompressedResult.lengths = 0;

	// Iterate over compressed blocks and decompress them
	ret = zlib_EnumerateBlocks(&stream, &uncompressedResult);
	if (ret != 0) // Did we error?
		return ret;


	// Concatenate together result
	ret = zlib_ConcatenateResult(&uncompressedResult, dstBuffer, uncompressedLength);
	if (ret != 0)
		return ret;


	// Return code of 0. Function complete successfully
	return STARDUST_ERROR_SUCCESS; 
}


// Validates that the container is valid
StardustErrorCode zlib_ValidateContainer(BitStream* stream)
{
	//Compression method and compression info.
	//bits 0-3 are the compression methdo flag. Only CM = 8 is supported by the ZLIB spec
	//bits 4-7 are the compression info. This is the window used for LZ77 compression. Should be less than 7
	unsigned char CMF = bs_ReadByte(stream);
	
	//CM flag
	if ((CMF & 15) != 8) //15 in binary is 0b1111 which masks the first 4 bits
		return STARDUST_ERROR_FILE_INVALID;

	//CINFO flag
	if ((CMF & 240) > 112) //240 in binary is 0b11110000 which masks the last 4 bits
		return STARDUST_ERROR_FILE_INVALID;


	//FLG byte which contains flags for compression info.
	//bits 0-4 is defined as FCHECK which is used in the equation (CMF * 256 + FLG) check sum. This must be a multiple of 32
	//bit 5 is set when using a dictionary. These dictionarys are known to the decompresser at compile time. Zlib does not define any dictionarys so we are not supporting them currently
	//bits 6-7 are the compression level. This is not needed for decompression so we can ignore it
	unsigned char FLG = bs_ReadByte(stream);

	//CHECKSUM
	if ((CMF * 256 + FLG) % 31 != 0) //Not masking the FLG parameter as all subsequent fields should be 0 for us to begin decompression. This measns that we can check for all of the above using a single if statment (+ no anding)
		return STARDUST_ERROR_FILE_INVALID;

	//Return 0 for valid container
	return STARDUST_ERROR_SUCCESS;
}

//Appends some data to a zlib result
StardustErrorCode zlib_AppendResult(char* data, unsigned long dataLen, ZLIBResult* result)
{
	// Check if we are at capacity
	if (result->blockCount == result->capacity)
	{
		// Get new capacity
		result->capacity += ZLIBRESULT_EXPAND_COUNT;

		// Expand capacity 
		char** nData = malloc(sizeof(char*) * result->capacity); // Expand data array
		if (nData == 0) // Verify allocation
			return STARDUST_ERROR_MEMORY_ERROR;

		unsigned long* nLen = malloc(sizeof(unsigned long) * result->capacity); // Expand length array
		if (nLen == 0) // Verify allocation
			return STARDUST_ERROR_MEMORY_ERROR;
		
		// Copy in old data
		for (unsigned int i = 0; i < result->blockCount; i++)
		{
			nData[i] = result->data[i];
			nLen[i] = result->lengths[i];
		}

		// Free old memory
		free(result->data);
		free(result->lengths);

		// Copy in new pointers
		result->data = nData;
		result->lengths = nLen;
	}

	// Append data
	result->data[result->blockCount] = data;
	result->lengths[result->blockCount++] = dataLen;

	// Success. Return code 0
	return STARDUST_ERROR_SUCCESS;
}


StardustErrorCode zlib_ConcatenateResult(ZLIBResult* result, char** dst, int* size)
{
	// Get total length of new array
	long totalLength = 0;
	for (unsigned int i = 0; i < result->blockCount; i++)
		totalLength += result->lengths[i]; // Sum up lengths
	
	// Set new array size
	*size = totalLength;

	// Allocate new array
	*dst = malloc(totalLength);
	if (*dst == 0) // Verify allocation
		return STARDUST_ERROR_MEMORY_ERROR;

	// Copy in data
	long idx = 0;
	for (unsigned int i = 0; i < result->blockCount; i++)
	{
		long len = result->lengths[i];
		for (long j = 0; j < len; j++)
			(*dst)[idx++] = result->data[i][j];
		
		// Consume data block
		free(result->data[i]);
	}

	
	// Consume the ZLIBResult
	free(result->data); // Clear up pointer arr. The memory that the pointers pointed to was cleaned up in the above loop
	free(result->lengths); // Free up lengths


	//Return code 0. OK
	return STARDUST_ERROR_SUCCESS;
}

//Enumerates over the compressed blocks in the zlib data and decompresses them.
StardustErrorCode zlib_EnumerateBlocks(BitStream* stream, ZLIBResult* result)
{
	/*
	ZLIB blocks have 3 bits in their header. 
	bit 1 is BFINAL and specifies whether this is the last block. If this bit is set, stop decompressing after the current block
	bits 2-3 are BTYPE. This specifies the compression method used on the block.

	Compression methods can be 
		- 00 = No compression
		- 01 = Static compression
		- 10 = Dynamic compression
		- 11 = Reserved (Error if encountered)
	*/

	unsigned char BFINAL = 0;
	unsigned char BTYPE = 0;
	ZLIBStaticTree tree; tree.litLenTree = 0; tree.distTree;

	while (!BFINAL)
	{
		// Read header of block
		BFINAL = bs_ReadBit(stream);
		BTYPE = bs_ReadBits(stream, 2);

		// Switch on BTYPE
		if (BTYPE == 0) // 00
			// No compression means that the data was not compressed and we can read the plain bytes
			zlib_InflateNoCompression(stream, result);
		else if (BTYPE == 1) // 01
			// Static compresion means that the data was compressed using trees already known to the decompressor (defined by the ZLIB spec)
			zlib_InflateStaticCompression(stream, result, &tree);
		else if (BTYPE == 2) // 10
			// Dynamic compression means that the huffman trees are decoded from the given block
			zlib_InflateDynamicCompression(stream, result);
		else // 11
			// Not allowed. Return error
			zlib_FreeStaticTrees(&tree);
			return STARDUST_ERROR_FILE_INVALID;
	}

	zlib_FreeStaticTrees(&tree);

	// Succesfully completed function. Return code 0
	return STARDUST_ERROR_SUCCESS;
}


//Inflates the current bit stream using not compression.
StardustErrorCode zlib_InflateNoCompression(BitStream* stream, ZLIBResult* result)
{
	/*
	An compressed block of data in zlib has a 4 byte header
	bytes 0-1 are the length in bytes of the data
	bytes 2-3 are the ones compliment of the length
	*/

	unsigned short int len = bs_ReadBytes(stream, 2);	// Bytes 0-1
	unsigned short int nlen = bs_ReadBytes(stream, 2); // Bytes 2-3

	// Create array to hold data
	char* data = malloc(len); //Allocate len bytes
	if (data == 0) // Verify allocation was successful
		return STARDUST_ERROR_MEMORY_ERROR;

	// Read in data
	for (unsigned short int i = 0; i < len; i++) // Loop for len bytes
		data[i] = bs_ReadByte(stream); // Read in byte

	// Append data to result
	zlib_AppendResult(data, len, result);

	// Successful. Return code 0
	return STARDUST_ERROR_SUCCESS;
}


//Inflates a compressed block using predefined lit/len and dist trees
StardustErrorCode zlib_InflateStaticCompression(BitStream* stream, ZLIBResult* result, ZLIBStaticTree* staticTrees)
{
	// Initialise function works regardless of current initalisation state. Will early exit if already initialised
	StardustErrorCode ret = zlib_InitialiseStaticTrees(&staticTrees);
	if (ret != 0) //Check return code
		return ret;

	zlib_InflateFromTrees(stream, result, staticTrees->litLenTree, staticTrees->distTree);

	return STARDUST_ERROR_SUCCESS;
}

StardustErrorCode zlib_InflateDynamicCompression(BitStream* stream, ZLIBResult* result)
{
	HuffmanNode* litLenTree = 0;
	HuffmanNode* distTree = 0;
	
	StardustErrorCode ret = zlib_DecodeTrees(stream, &litLenTree, &distTree);
	if (ret != 0)
		return ret;

	zlib_InflateFromTrees(stream, result, litLenTree, distTree);

	FreeTree(litLenTree);
	FreeTree(distTree);

	return STARDUST_ERROR_SUCCESS;
}


//Inflates a block of compressed data using the given trees.
StardustErrorCode zlib_InflateFromTrees(BitStream* stream, ZLIBResult* result, HuffmanNode* litLenTree, HuffmanNode* distTree)
{
	// Create list to hold data
	unsigned long len = 0;
	unsigned long capacity = 50;
	char* o = malloc(capacity);
	if (o == 0)
		return STARDUST_ERROR_MEMORY_ERROR;


	// Loop until we reach a break character
	while (1)
	{
		// Decode the next symbol in the block
		int sym = DecodeFromTree(litLenTree, stream);

		// Switch on sym
		if (sym < 256) // Symbol is a literal (fits in a char)
		{
			if (len == capacity) // Not nice code. Not nice at all
			{
				capacity += 50;
				char* nO = malloc(capacity);
				if (nO == 0)
				{
					free(o);
					return STARDUST_ERROR_MEMORY_ERROR;
				}
				for (unsigned int i = 0; i < len; i++)
					nO[i] = o[i];
				free(o);
				o = nO;
			}
			o[len++] = sym;
		}

		else if (sym == 256) // Break character. End of Block
			break;
	
		else // Otherwise it is a length. Decompress LZ77
		{
			sym -= 257; // Translate

			// Get length to read back
			int length = zlib_LengthBase[sym] + (int)bs_ReadBits(stream, zlib_LengthExtraBits[sym]);

			// Get distance. After a length we expect a distance symbol
			int distSym = DecodeFromTree(distTree, stream);
			int dist = zlib_DistanceBase[distSym] + (int)bs_ReadBits(stream, zlib_DistanceExtraBits[distSym]);

			// Append copied data
			for (int i = 0; i < length; i++)
			{
				if (len == capacity)
				{
					capacity += 50;
					char* nO = malloc(capacity);
					if (nO == 0)
					{
						free(o);
						return STARDUST_ERROR_MEMORY_ERROR;
					}
					for (unsigned int i = 0; i < len; i++)
						nO[i] = o[i];
					free(o);
					o = nO;
				}

				o[len++] = o[len - dist];
			}
				//AppendElement(o, GetElement(o, o->size - dist - 1));
		}
	}

	// Allocate data
	char* data = malloc(len);
	if (data == 0)
	{
		free(o);
		return STARDUST_ERROR_MEMORY_ERROR;
	}

	// Copy in data
	for (unsigned int i = 0; i < len; i++)
		data[i] = o[i];

	
	StardustErrorCode ret = zlib_AppendResult(data, len, result);
	free(o);

	return ret;
}


// Decodes the dynamic huffman trees from the given stream.
StardustErrorCode zlib_DecodeTrees(BitStream* stream, HuffmanNode** litLenTree, HuffmanNode** distTree)
{
	// Get HLIT which is the number of literal codes
	int HLIT = (int)bs_ReadBits(stream, 5) + 257;

	// Get HDIST which is the number of distance codes
	int HDIST = (int)bs_ReadBits(stream, 5) + 1;

	// Get HCLEN which is the number of length codes
	int HCLEN = (int)bs_ReadBits(stream, 4) + 4;


	// Read codelengths
	unsigned int codeLengths[19] = { 0 };
	for (int i = 0; i < HCLEN; i++)
	{
		unsigned char v = bs_ReadBits(stream, 3);
		codeLengths[zlib_CodeLengthCodesOrder[i]] = (unsigned int)v;
	}

	// Build code length tree
	HuffmanNode* codeTree = DeriveTreeFromBitLengths(codeLengths, zlib_CodeLengthIndices, 19);


	// Read literal/length tree and distance tree
	int codes[300] = { 0 }; // I don't paticuarly like this but it does save some allocations/deallocatinos and keeps the memory on the stack so.....?
	int idx = 0;
	while (idx < HLIT + HDIST)
	{
		int sym = DecodeFromTree(codeTree, stream);
		
		if (sym < 16) // Literal value
			codes[idx++] = sym;

		else if (sym == 16) // Copy previou scode length 3 to 6 times
		{
			int prevCodeLen = codes[idx - 1]; // Get last code length
			int repeatLength = (int)bs_ReadBits(stream, 2) + 3; // Get extended amount to copy
			for (int i = 0; i < repeatLength; i++, idx++) // Copy in previuos values
				codes[idx] = prevCodeLen;
		}

		else if (sym == 17) // Copy code length 0 for 3 to 10 times
		{
			int repeatLength = (int)bs_ReadBits(stream, 3) + 3;
			for (int i = 0; i < repeatLength; i++, idx++)
				codes[idx] = 0;
		}

		else if (sym == 18) // Copy code length 0 for 11 to 138 times
		{
			int repeatLength = (int)bs_ReadBits(stream, 7) + 11;
			for (int i = 0; i < repeatLength; i++, idx++)
				codes[idx] = 0;
		}

		else // Invalid symbol
		{
			FreeTree(codeTree);
			return STARDUST_ERROR_FILE_INVALID;
		}
	}

	// Create trees
	*litLenTree = DeriveTreeFromBitLengths(codes, zlib_StaticLiteralLengthIndices, HLIT);
	*distTree = DeriveTreeFromBitLengths(codes + HLIT, zlib_StaticDistanceIndices, 30);

	free(codeTree);

	return STARDUST_ERROR_SUCCESS;
}

StardustErrorCode zlib_InitialiseStaticTrees(ZLIBStaticTree** trees)
{
	// Derive trees
	if ((*trees)->litLenTree == 0)
		(*trees)->litLenTree = DeriveTreeFromBitLengths(zlib_StaticLiteralLengthBitLengths, zlib_StaticLiteralLengthIndices, 288); // Literal and Length Tree

	// Assume that both trees are allocated/deallocated together
	(*trees)->distTree = DeriveTreeFromBitLengths(zlib_StaticDistanceBitLengths, zlib_StaticDistanceIndices, 30); // Distance Tree

	// OK. Return code 0
	return STARDUST_ERROR_SUCCESS;
}

void zlib_FreeStaticTrees(ZLIBStaticTree* trees)
{
	if (trees->litLenTree == 0) // Assume that both are allocated together
		return;

	// Free contained trees
	FreeTree(trees->litLenTree);
	FreeTree(trees->distTree);
}


