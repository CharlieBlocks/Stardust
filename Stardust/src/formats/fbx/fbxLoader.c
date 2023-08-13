#include "fbx.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

const char* FBX_MAGIC = "Kaydara FBX Binary\x20\x20\x00\x1a\x00";

//Array where the element corresponding to the FBXPropertyType value is it's size
const uint32_t _fbx_Sizes[] = {
	2,
	1,
	4,
	4,
	8,
	8,

	4,
	8,
	8,
	4,
	1,

	1,
	1
};

StardustErrorCode _fbx_LoadMesh(const char* path, const StardustMeshFlags flags, StardustMesh** meshes, size_t* meshCount)
{
	if (!FBXPropertyDictInit)
		_fbx_InitFBXPropertyDict();

	//Predfined vars
	StardustErrorCode ret;	//	Return code
	FileStream stream;		//	File stream

	uint32_t fbxVersion;	//	FBX version number

	// Open File Stream //
	//ret = sd_CreateFileStream(path, &stream);
	ret = fs_OpenStream(path, &stream);
	if (ret != 0) //0 is the value of STARDUST_ERROR_SUCCESS
		return ret;

	// Validate header //
	ret = _fbx_GetHeader(&stream, &fbxVersion);
	if (ret != 0)
		return ret;

	// Create root Node
	struct FBXNode globalNode;
	ret = _fbx_GetGlobalNode(&stream, &globalNode);

	//Close
	_fbx_FreeGlobalNode(&globalNode);
	fs_CloseStream(&stream);

	return STARDUST_ERROR_SUCCESS;
}

StardustErrorCode _fbx_GetHeader(FileStream* stream, uint32_t* version)
{
	char header[23];
	StardustErrorCode ret = fs_ReadBytes(stream, header, 23);
	if (ret != 0)
		return ret;

	//Compare header
	int match = 1;
	for (int i = 0; i < 23; i++)
		match &= header[i] == FBX_MAGIC[i];

	if (!match)
		return STARDUST_ERROR_FILE_INVALID;

	//Get version number
	uint32_t ver = fs_ReadUint32(stream);

	if (ver > MAX_FBX_VER || ver < MIN_FBX_VER)
		return STARDUST_ERROR_FILE_INVALID;
	
	*version = ver;

	return STARDUST_ERROR_SUCCESS;
}

StardustErrorCode _fbx_GetGlobalNode(FileStream* stream, struct FBXNode* globalNode)
{
	//Fill global node
	globalNode->endOffset = 0;
	globalNode->propertyCount = 0;
	globalNode->properties = 0;
	globalNode->nameLen = 0;
	globalNode->name = 0;

	globalNode->childCount = 0;
	globalNode->children = 0;

	StardustErrorCode result = 0;

	while (stream->characterIndex < stream->eof - 162) //162 is the size of the footer. The contents of this is currently unknown. Appears to just be binary garbage
	{
		//Increase storage
		struct FBXNode** children = malloc(sizeof(struct FBXNode*) * (globalNode->childCount + 1)); //Allocate new buffer
		if (children == 0) //Verify allocation
		{
			//_fbx_FreeNode(globalNode);
			for (uint32_t i = 0; i < globalNode->childCount; i++)
				_fbx_FreeNode(globalNode->children[i]);
			free(globalNode->children);

			return STARDUST_ERROR_MEMORY_ERROR;
		}

		//Copy in nodes
		memcpy(children, globalNode->children, globalNode->childCount * sizeof(struct FBXNode*));

		//Free old memory
		if (globalNode->children != 0)
			free(globalNode->children);

		//Set new memory
		globalNode->children = children;
		globalNode->childCount += 1;

		//Create new node
		struct FBXNode* childNode = _fbx_GetNode(stream, &result);
		if (result != 0)
		{
			for (uint32_t i = 0; i < globalNode->childCount - 1; i++)
				_fbx_FreeNode(globalNode->children[i]);
			free(globalNode->children);
			return result;
		}

		globalNode->children[globalNode->childCount - 1] = childNode;
	}

	return result;
}

struct FBXNode* _fbx_GetNode(FileStream* stream, StardustErrorCode* result)
{
	// Read node header
	uint32_t endOffset = fs_ReadUint32(stream);	//Distance from beginning of file to end of this node
	uint32_t propCount = fs_ReadUint32(stream);	//Number of properties
	uint32_t propLen = fs_ReadUint32(stream);	//Property size in bytes
	int8_t nameLen = fs_ReadInt8(stream);		//Length of the name in bytes

	char* nodeName = malloc(nameLen);
	if (nodeName == 0)
	{
		*result = STARDUST_ERROR_MEMORY_ERROR;
		return 0;
	}
	*result = fs_ReadBytes(stream, nodeName, nameLen);
	if (*result != 0)
		return 0;

	// Create node
	struct FBXNode* node = malloc(sizeof(struct FBXNode));
	if (node == 0)
	{
		free(nodeName);
		*result = STARDUST_ERROR_MEMORY_ERROR;
		return 0;
	}


	// Fill node
	node->endOffset = endOffset;
	node->propertyCount = propCount;
	node->nameLen = nameLen;
	node->name = nodeName;

	node->childCount = 0;
	node->children = 0;

	node->properties = malloc(sizeof(FBXProperty) * node->propertyCount);

	//Read properties
	for (uint32_t i = 0; i < node->propertyCount; i++)
	{
		_fbx_GetProperty(stream, node->properties + i);
	}


	//Get children
	while ((int32_t)stream->characterIndex < (int32_t)node->endOffset - 13) //Convert to int32_t to handle unexpected null terminators
	{
		//Increase storage
		struct FBXNode** children = malloc(sizeof(struct FBXNode*) * (node->childCount + 1)); //Allocate new buffer
		if (children == 0) //Verify allocation
		{
			_fbx_FreeNode(node);
			*result = STARDUST_ERROR_MEMORY_ERROR;
			return 0;
		}

		//Copy in nodes
		memcpy(children, node->children, node->childCount * sizeof(struct FBXNode*));
	
		//Free old memory
		if (node->children != 0)
			free(node->children);

		//Set new memory
		node->children = children;
		node->childCount += 1;

		//Create new node
		struct FBXNode* childNode = _fbx_GetNode(stream, result);
		if (*result != 0)
		{
			_fbx_FreeNode(node); //This will error. Implement more generalised solution
			return 0;
		}

		node->children[node->childCount - 1] = childNode;
	}

	if (node->childCount != 0)
	{
		f_Seek(stream->file, 13, FileOrigin_Current);
		stream->characterIndex += 13;
	}

	return node;
}

StardustErrorCode _fbx_GetProperty(FileStream* stream, FBXProperty* prop)
{
	//Get type
	char type;
	fs_ReadBytes(stream, &type, 1);

	prop->type = FBXPropertyDict[(int8_t)type]; //Use ASCII value of type for dict index
	prop->length = 1;

	//Validate type
	if (prop->type < 0 || prop->type > 12)
		return STARDUST_ERROR_FILE_INVALID;

	//Check if type is non-trivial
	if (prop->type > 5) //String or raw
	{
		prop->length = fs_ReadUint32(stream);

		if (prop->type < 10)
		{
			prop->enc = (int8_t)fs_ReadUint32(stream);
			prop->compLen = fs_ReadUint32(stream);
		}
		else
			prop->enc = 0;
	}
	
	if (prop->enc == 1)
	{
		prop->rawArr = malloc(prop->compLen);
		if (prop->rawArr == 0)
			return STARDUST_ERROR_MEMORY_ERROR;

		StardustErrorCode ret = fs_ReadBytes(stream, prop->rawArr, prop->compLen);
		return ret;
	}

	prop->raw = malloc(prop->length * _fbx_Sizes[prop->type]);
	if (prop->raw == 0)
		return STARDUST_ERROR_MEMORY_ERROR;

	StardustErrorCode ret = fs_ReadBytes(stream, prop->raw, prop->length * _fbx_Sizes[prop->type]);
	return ret;
}

void _fbx_FreeGlobalNode(struct FBXNode* node)
{
	if (node->children != 0)
	{
		for (uint32_t i = 0; i < node->childCount; i++)
			_fbx_FreeNode(node->children[i]);
		free(node->children);
	}
}

void _fbx_FreeNode(struct FBXNode* node)
{
	if (node->name != 0)
		free(node->name);

	if (node->propertyCount != 0)
	{
		for (uint32_t i = 0; i < node->propertyCount; i++)
			_fbx_FreeProperty(&node->properties[i]);
		free(node->properties);
	}

	if (node->children != 0)
	{
		for (uint32_t i = 0; i < node->childCount; i++)
			_fbx_FreeNode(node->children[i]);
		free(node->children);
	}

	free(node);
}

void _fbx_FreeProperty(FBXProperty* prop)
{
	free(prop->raw);
}

void _fbx_InitFBXPropertyDict()
{
	FBXPropertyDict[89] = FBX_INT16;
	FBXPropertyDict[67] = FBX_BOOL;
	FBXPropertyDict[73] = FBX_INT32;
	FBXPropertyDict[70] = FBX_FLOAT;
	FBXPropertyDict[68] = FBX_DOUBLE;
	FBXPropertyDict[76] = FBX_LONG;
	FBXPropertyDict[102] = FBX_FLOAT_ARR;
	FBXPropertyDict[100] = FBX_DOUBLE_ARR;
	FBXPropertyDict[108] = FBX_LONG_ARR;
	FBXPropertyDict[105] = FBX_INT_ARR;
	FBXPropertyDict[98] = FBX_BOOL_ARR;
	FBXPropertyDict[83] = FBX_STRING;
	FBXPropertyDict[82] = FBX_RAW;

	FBXPropertyDictInit = 1;
}