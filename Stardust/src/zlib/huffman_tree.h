#ifndef _STARDUST_HUFFMANTREE
#define _STARUDST_HUFFMANTREE

/*
Documentation:

*/

#include "stardust.h"
#include "utils/bitstream.h"


struct HuffmanLeaf;
struct HuffmanLeaf
{
	int symbol;

	struct HuffmanLeaf* left;
	struct HuffmanLeaf* right;
};

typedef struct HuffmanLeaf HuffmanNode;


/// <summary>
/// Inserts a given symbol into a huffman tree using codeword <codeword>
/// </summary>
/// <param name="root">The root node of the tree</param>
/// <param name="codeword">The codeword (represented in bits) of the value</param>
/// <param name="codewordLen">The length of the codeword (in bits)</param>
/// <param name="symbol">The symbol to insert</param>
/// <returns>STARDUST_ERROR_SUCCESS if ok</returns>
StardustErrorCode InsertIntoTree(HuffmanNode* root, int codeword, const int codewordLen, int symbol);

/// <summary>
/// Derives a huffman tree from the symbols and their corresponding bit lengths.
/// Requires everything to be in order.
/// </summary>
/// <param name="bitLengths">An array of integers containing the length of each codeword</param>
/// <param name="symbols">An array of chars that have a symbol corresponding to a bit length</param>
/// <param name="count">The length of the given arrays. Arrays should be at least the same length</param>
/// <returns>A Huffman node which is the root of the derived tree</returns>
HuffmanNode* DeriveTreeFromBitLengths(const unsigned int* bitLengths, const int* symbols, const int count);

/// <summary>
/// Decodes a symbol from the given bitstream
/// </summary>
/// <param name="root">The root node of the tree</param>
/// <param name="stream">A bit stream containing encoded data</param>
/// <returns>The symbol</returns>
int DecodeFromTree(HuffmanNode* root, BitStream* stream);

/// <summary>
/// Free's the given tree from memory
/// </summary>
/// <param name="root"></param>
void FreeTree(HuffmanNode* root);


#endif //_STARDUST_HUFFMANTREE 