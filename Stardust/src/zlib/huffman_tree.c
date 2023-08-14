#include "huffman_tree.h"
#include <stdlib.h>

StardustErrorCode InsertIntoTree(HuffmanNode* root, int codeword, int codewordLength, char symbol)
{
	for (int i = codewordLength - 1; i >= 0; i--)
	{
		int b = codeword & (1 << i);

		if (b)
		{
			if (root->right == 0)
			{
				root->right = malloc(sizeof(HuffmanNode));
				if (root->right == 0)
					return STARDUST_ERROR_MEMORY_ERROR;
				root->right->right = 0; root->right->left = 0;
			}
			root = root->right;
		}
		else
		{
			if (root->left == 0)
			{
				root->left = malloc(sizeof(HuffmanNode));
				if (root->left == 0)
					return STARDUST_ERROR_MEMORY_ERROR;
				root->left->right = 0; root->left->left = 0;
			}
			root = root->left;
		}
	}

	root->symbol = symbol;

	return STARDUST_ERROR_SUCCESS;
}

HuffmanNode* DeriveTreeFromBitLengths(unsigned int* bitLengths, char* symbols, const int count)
{
	// Calculate the longest bitlength //
	int maxBitLen = 0;
	for (int i = 0; i < count; i++)
		if (bitLengths[i] > maxBitLen)
			maxBitLen = bitLengths[i];

	// Compute the number of codes for each bitlength //
	int* bitLengthCounts = malloc(sizeof(int) * (maxBitLen + 1)); //Allocate array
	if (bitLengthCounts == 0)
		return 0;

	//Sum up all bit lengths
	for (int i = 0; i < maxBitLen + 1; i++)
	{
		bitLengthCounts[i] = 0;
		for (int j = 0; j < count; j++)
			bitLengthCounts[i] += bitLengths[j] == i && i != 0;
	}

	// Create nextCodes //
	int* nextCodes = malloc(sizeof(int) * ((maxBitLen > 2 ? maxBitLen : 2) + 1)); //Allocate with size of min 2
	if (nextCodes == 0)
		return 0;

	nextCodes[0] = 0;
	nextCodes[1] = 0;

	for (int i = 2; i < maxBitLen + 1; i++)
		nextCodes[i] = (nextCodes[i - 1] + bitLengthCounts[i - 1]) << 1;


	//Create Huffman tree
	HuffmanNode* tree = malloc(sizeof(HuffmanNode));
	if (tree == 0)
		return 0;
	tree->right = 0; tree->left = 0;

	for (int i = 0; i < count; i++)
	{
		if (bitLengths[i] == 0)
			continue;
		int bitLen = bitLengths[i];
		InsertIntoTree(tree, nextCodes[bitLen], bitLen, symbols[i]);
		nextCodes[bitLen]++;
	}

	return tree;
}

char DecodeFromTree(HuffmanNode* root, BitStream* stream)
{
	while (root->right || root->left)
	{
		char b = bs_GetBit(stream);
		if (b)
			root = root->right;
		else
			root = root->left;
	}

	return root->symbol;
}

void FreeTree(HuffmanNode* root)
{
	if (root->left)
		FreeTree(root->left);
	if (root->right)
		FreeTree(root->right);

	free(root);
}
