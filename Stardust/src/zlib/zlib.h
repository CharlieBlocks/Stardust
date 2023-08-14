#ifndef _STARDUST_ZLIB
#define _STARDUST_ZLIB

/*
Documentation:

*/

#include "stardust.h"
#include "utils/bitstream.h"

typedef struct
{
	char** data; //An array of arrays containing each block
	unsigned long* lengths; //An array containing the lengths of each block (in bytes)

	unsigned long blockCount; //The number of blocks of data stored in the result
} ZLIBResult;

/// <summary>
/// Inflates the given data using the Z-LIB spec.
/// This involves decoding through huffman trees and reconstructing from the LZ77 compression.
/// Allocates dst and places the resultant data into it.
/// Places the length of dst into dataLength.
/// This assumes that src is the entire zlib string. If this is invalid then it will reutrn STARDUST_FILE_INVALID
/// </summary>
/// <param name="dstBuffer">The destination buffer</param>
/// <param name="uncompressedLength>The length of dstBuffer</param>
/// <param name="srcBuffer">The src data buffer</param>
/// <returns>Error code</returns>
StardustErrorCode zlib_inflate(char* dstBuffer, unsigned long* uncompressedLength, char* srcBuffer);

/// <summary>
/// Validates that the zlib container holds the correct info.
/// This will check for CMF, CINFO, FLG, and FDICT.
/// Assert that CMF is 8, CINFO is greater than 7, the checksum using FLG and CMF is correct and that no FDICT is used.
/// </summary>
/// <param name="stream"></param>
/// <returns>STARDUST_ERROR_SUCCESS for a valid container, otherwise the relevant error code (usually STARDUST_ERROR_FILE_INVALID)</returns>
StardustErrorCode zlib_ValidateContainer(BitStream* stream);

/// <summary>
/// Enumerates over all blocks in the compressed data and inflates them. This is placed into the ZLIBResult
/// </summary>
/// <param name="stream">The BitStream containing the data. This should have passed through zlib_ValidateContainer first</param>
/// <param name="result">A valid pointer to a ZLIBResult struct to store the resultant data</param>
/// <returns></returns>
StardustErrorCode zlib_EnumerateBlocks(BitStream* stream, ZLIBResult* result);


/// <summary>
/// Inflates the current block (as given by the bit stream) using no compression.
/// The result is placed into the given ZLIBResult pointer.
/// The bit stream is expected to have passed through zlib_EnumerateBlocks
/// </summary>
/// <param name="stream"></param>
/// <param name="result"></param>
/// <returns></returns>
StardustErrorCode zlib_InflateNoCompression(BitStream* stream, ZLIBResult* result);

/// <summary>
/// Inflates the current block using static compression (where the huffman trees are predefined)
/// the result is placed into the given ZLIBResult pointer.
/// The bit stream is expected to have passed through zlib_EnumerateBlocks
/// </summary>
/// <param name="stream"></param>
/// <param name="result"></param>
/// <returns></returns>
StardustErrorCode zlib_InflateStaticCompression(BitStream* stream, ZLIBResult* result);

/// <summary>
/// Inflates the current block using dynamic compression (Huffman trees are derived from the block data)
/// the result is placed into the given ZLIBResult pointer.
/// The bit stream is expected to have passed through zlib_EnumerateBlocks
/// </summary>
/// <param name="stream"></param>
/// <param name="result"></param>
/// <returns></returns>
StardustErrorCode zlib_InflateDynamicCompression(BitStream* stream, ZLIBResult* result);

#endif //_STARDUST_ZLIB