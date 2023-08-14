#ifndef _STARDUST_ZLIB
#define _STARDUST_ZLIB

/*
Documentation:

*/

#include "stardust.h"
#include "utils/bitstream.h"
#include "huffman_tree.h"

#define ZLIBRESULT_EXPAND_COUNT 3

typedef struct
{
	char** data; //An array of arrays containing each block
	unsigned long* lengths; //An array containing the lengths of each block (in bytes)

	unsigned int blockCount; //The number of blocks of data stored in the result
	unsigned int capacity;
} ZLIBResult;

typedef struct
{
	HuffmanNode* litLenTree;
	HuffmanNode* distTree;
} ZLIBStaticTree; 


/*
Static compression / decompression bit lengths.Huffman trees are built from this.

zlib_StaticLiteralLengthBitLengths are the bit lengths for len/lit huffman tree.
- 0-143 = 8
- 144-255 = 9
- 256-279 = 7
- 280-287 = 8
Total Length: 286 values

zlib_StaticDistanceBitLengths are the bit lengths for the distance huffamn tree
- 0-29 = 5
Total Length: 30 values
*/
static const int zlib_StaticLiteralLengthBitLengths[] = { 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8 };
static const int zlib_StaticLiteralLengthIndices[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284, 285 };
static const int zlib_StaticDistanceBitLengths[] = { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 };
static const int zlib_StaticDistanceIndices[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29 };


/*
LZ77 distance to read and copy amounts.

Base is the base value that the read length is.
ExtraBits is the number of extra bits to read and add to the base.
*/
static const int zlib_LengthExtraBits[] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
static const int zlib_LengthBase[] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
static const int zlib_DistanceExtraBits[] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
static const int zlib_DistanceBase[] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };


/*
The order in which the code length huffman codes are read from a compresed block of data.
*/
static const int zlib_CodeLengthCodesOrder[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
static const int zlib_CodeLengthIndices[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 18 };

/// <summary>
/// Inflates the given data using the Z-LIB spec.
/// This involves decoding through huffman trees and reconstructing from the LZ77 compression.
/// Allocates dst and places the resultant data into it.
/// Places the length of dst into dataLength.
/// This assumes that src is the entire zlib string. If this is invalid then it will reutrn STARDUST_FILE_INVALID
/// </summary>
/// <param name="dstBuffer">A pointer to the destination buffer</param>
/// <param name="uncompressedLength>The length of dstBuffer</param>
/// <param name="srcBuffer">The src data buffer</param>
/// <returns>Error code</returns>
StardustErrorCode zlib_Inflate(char** dstBuffer, unsigned long* uncompressedLength, char* srcBuffer);

/// <summary>
/// Validates that the zlib container holds the correct info.
/// This will check for CMF, CINFO, FLG, and FDICT.
/// Assert that CMF is 8, CINFO is greater than 7, the checksum using FLG and CMF is correct and that no FDICT is used.
/// </summary>
/// <param name="stream"></param>
/// <returns>STARDUST_ERROR_SUCCESS for a valid container, otherwise the relevant error code (usually STARDUST_ERROR_FILE_INVALID)</returns>
StardustErrorCode zlib_ValidateContainer(BitStream* stream);


/// <summary>
/// Adds the given data to the zlib result
/// </summary>
/// <param name="data">Data to append</param>
/// <param name="dataLen">Length of the data</param>
/// <param name="result">Result to append to</param>
/// <returns>0 for success</returns>
StardustErrorCode zlib_AppendResult(char* data, unsigned long dataLen, ZLIBResult* result);


/// <summary>
/// Concatenates a zlib result into a single array.
/// Returns the the array and the array length.
/// This function will consume the result, thereby deallocating it
/// </summary>
/// <param name="result">Filled zlib result</param>
/// <param name="dst">Pointer to destination array</param>
/// <param name="size">The length of dst</param>
/// <returns></returns>
StardustErrorCode zlib_ConcatenateResult(ZLIBResult* result, char** dst, int* size);


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
StardustErrorCode zlib_InflateStaticCompression(BitStream* stream, ZLIBResult* result, ZLIBStaticTree* staticTrees);


/// <summary>
/// Inflates the current block using dynamic compression (Huffman trees are derived from the block data)
/// the result is placed into the given ZLIBResult pointer.
/// The bit stream is expected to have passed through zlib_EnumerateBlocks
/// </summary>
/// <param name="stream"></param>
/// <param name="result"></param>
/// <returns></returns>
StardustErrorCode zlib_InflateDynamicCompression(BitStream* stream, ZLIBResult* result);


/// <summary>
/// Inflates a block of compressed data using the given trees.
/// </summary>
/// <param name="stream"></param>
/// <param name="result"></param>
/// <param name="litLenTree"></param>
/// <param name="distTree"></param>
/// <returns></returns>
StardustErrorCode zlib_InflateFromTrees(BitStream* stream, ZLIBResult* result, HuffmanNode* litLenTree, HuffmanNode* distTree);


/// <summary>
/// Decodes the dynamic huffman trees from the given stream.
/// </summary>
/// <param name="stream"></param>
/// <param name="litLenTree"></param>
/// <param name="distTree"></param>
/// <returns></returns>
StardustErrorCode zlib_DecodeTrees(BitStream* stream, HuffmanNode** litLenTree, HuffmanNode** distTree);


/// <summary>
/// Initialises the litLenTree and distTree using the static values.
/// </summary>
/// <param name="trees"></param>
/// <returns></returns>
StardustErrorCode zlib_InitialiseStaticTrees(ZLIBStaticTree** trees);


/// <summary>
/// Frees memory used by a ZLIBStaticTree object
/// </summary>
/// <param name="trees"></param>
void zlib_FreeStaticTrees(ZLIBStaticTree* trees);

#endif //_STARDUST_ZLIB