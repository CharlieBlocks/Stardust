#include "fbx.h"

#include "zlib/zlib.h"

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
	struct FBXNode globalNode; //GLOBAL node appears to contain 1 extra child (in counting)
	ret = _fbx_GetGlobalNode(&stream, &globalNode);


	ret = fbx_GetMesh(&globalNode, meshes, flags);

	//Vertices
	//Indices
	//UVs
	//Normals

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

StardustErrorCode fbx_GetMesh(struct FBXNode* globalNode, StardustMesh** meshes, const StardustMeshFlags flags)
{
	// Get list of objects
	//struct FBXNode* objects = fbx_GetNode(globalNode, "Objects", 0);
	int objectTagIdx = fbx_GetNode(globalNode, "Objects", 0);
	if (objectTagIdx == -1)
		return STARDUST_ERROR_FILE_INVALID;
	struct FBXNode* objects = globalNode->children[objectTagIdx];

	//Find meshes
	int meshCount = 0;
	for (unsigned int i = 0; i < objects->childCount; i++)
	{
		if (strcmp(objects->children[i]->name, "Geometry") == 0)
			meshCount += 1;
	}

	//Allocate meshes
	*meshes = malloc(sizeof(StardustMesh) * meshCount);
	if (*meshes == 0)
		return STARDUST_ERROR_MEMORY_ERROR;

	// Get all models
	int meshIdx = 0;
	while (1)
	{
		int geoIdx = fbx_GetNode(objects, "Geometry", meshIdx);
		if (geoIdx == -1)
			break;
		struct FBXNode* geometry = objects->children[geoIdx];

		FBXRawData data = { 0 }; // Initialise FBXRawData struct to 0
		StardustErrorCode ret;

		// Get all attributes of mesh
		int vertexIdx = fbx_GetNode(geometry, "Vertices", 0);
		int indexIdx = fbx_GetNode(geometry, "PolygonVertexIndex", 0);
		
		// Check that we have both vertexIdx and indexIdx. These are requrired for a mesh
		if (vertexIdx == -1 || indexIdx == -1) { return STARDUST_ERROR_FILE_INVALID; }

		int normalIdx = fbx_GetNode(geometry, "LayerElementNormal", 0);
		int uvIdx = fbx_GetNode(geometry, "LayerElementUV", 0);


		// Get vertices
		ret = fbx_GetVertices(geometry->children[vertexIdx], &data);
		if (ret != 0) { fbx_FreeRawData(&data); free(*meshes); return ret; }

		// Get indices
		ret = fbx_GetIndices(geometry->children[indexIdx], &data);
		if (ret != 0) { fbx_FreeRawData(&data); free(*meshes); return ret; }

		if (normalIdx != -1)
		{
			ret = fbx_GetNormals(geometry->children[normalIdx], &data);
			if (ret != 0) { fbx_FreeRawData(&data); free(*meshes); return ret; }

			if (data.normalApplication == FBX_DIRECT)
			{
				data.normalIndexData = fbx_GenerateDirectIndices(data.normalCount);
				data.normalIndexCount = data.normalCount;
				data.normalApplication = FBX_INDEX_TO_DIRECT;
			}
			if (data.normalIndexCount != data.indexCount) { fbx_FreeRawData(&data); return ret; }

			ret = fbx_CompactArray(data.normalData, data.normalIndexData, &data.normalCount, 3, data.normalIndexCount);
			if (ret != 0) { fbx_FreeRawData(&data); free(*meshes); return ret; }
		}

		if (uvIdx != -1)
		{
			ret = fbx_GetTextureCoords(geometry->children[uvIdx], &data);
			if (ret != 0) { fbx_FreeRawData(&data); free(*meshes); return ret; }

			if (data.uvApplication == FBX_DIRECT)
			{
				data.uvIndexData = fbx_GenerateDirectIndices(data.uvCount);
				data.uvIndexCount = data.uvCount;
				data.uvApplication = FBX_INDEX_TO_DIRECT;
			}
			if (data.uvIndexCount != data.indexCount) { fbx_FreeRawData(&data); return ret; }

			ret = fbx_CompactArray(data.uvData, data.uvIndexData, &data.uvCount, 2, data.uvIndexCount);
			if (ret != 0) { fbx_FreeRawData(&data); free(*meshes); return ret; }
		}

		
		// Create hashes and indices
		struct FBXVertexHash* hashArr;
		unsigned int hashArrSize; // This is not the actual size of allocated memory. Just the amount of hashses contained within an array of data.indexCount size
		ret = fbx_ComputeHashAndIndexArray(&data, &hashArr, data.indexData, &hashArrSize);

		// Create vertex array
		Vertex* vertexArray = malloc(sizeof(Vertex) * hashArrSize);
		if (vertexArray == 0) { fbx_FreeRawData(&data); free(*meshes); free(hashArr); return ret; }

		// Create vertices
		for (unsigned int i = 0; i < hashArrSize; i++)
		{
			// Fill Vertex
			int vtxIdx = abs(hashArr[i].vtxIdx) * 3; // Vertices are 3 elements wide. W is always 1
			vertexArray[i].x = data.vertexData[vtxIdx];
			vertexArray[i].y = data.vertexData[vtxIdx + 1];
			vertexArray[i].z = data.vertexData[vtxIdx + 2];
			vertexArray[i].w = 1;

			if (hashArr[i].uvIdx != -1) // Check if UVs were packaged with the mesh
			{
				int uvIdx = hashArr[i].uvIdx * 2;
				vertexArray[i].texU = data.uvData[uvIdx];
				vertexArray[i].texV = data.uvData[uvIdx + 1];
				vertexArray[i].texW = 0; // FBX does not allow for W coordinates (As far as I know)
			}

			if (hashArr[i].nrmIdx != -1) // Check if Normals were packaged with the mesh
			{
				int nrmIdx = hashArr[i].nrmIdx * 3;
				vertexArray[i].normX = data.normalData[nrmIdx];
				vertexArray[i].normY = data.normalData[nrmIdx + 1];
				vertexArray[i].normZ = data.normalData[nrmIdx + 2];
			}
		}

		// Free used data
		free(hashArr);

		// Create meshes
		StardustMesh* currMesh = &(*meshes)[meshIdx];
		
		// Set Vertices
		currMesh->vertices = vertexArray;
		currMesh->vertexCount = hashArrSize;

		// Set Indices
		fbx_FormatIndexArray(&data, &(currMesh->indices));
		currMesh->indexCount = data.indexCount;

		// Temporary
		currMesh->vertexStride = 3;

		// Datatypes
		currMesh->dataType = STARDUST_VERTEX_DATA | STARDUST_INDEX_DATA;

		if (uvIdx != -1)
			currMesh->dataType |= STARDUST_TEXTURE_DATA;

		if (normalIdx != -1)
			currMesh->dataType |= STARDUST_NORMAL_DATA;
		
		fbx_FreeRawData(&data);

		meshIdx++;
	}

	return STARDUST_ERROR_SUCCESS;
}

int fbx_GetNode(struct FBXNode* globalNode, const char* label, int startAt)
{
	for (unsigned int i = startAt; i < globalNode->childCount; i++)
	{
		if (strcmp(globalNode->children[i]->name, label) == 0)
			return i;
	}

	return -1;
}

void fbx_FreeRawData(FBXRawData* data)
{
	if ((data->dataType & FBX_VERTICES) == FBX_VERTICES)
		free(data->vertexData);

	if ((data->dataType & FBX_INDICES) == FBX_INDICES)
		free(data->indexData);

	if ((data->dataType & FBX_NORMALS) == FBX_NORMALS)
	{
		free(data->normalData);
		if (data->normalApplication != FBX_DIRECT) { free(data->normalIndexData); }
	}

	if ((data->dataType & FBX_UVS) == FBX_UVS)
	{
		free(data->uvData);
		if (data->uvApplication != FBX_DIRECT) { free(data->uvIndexData); }
	}
}

StardustErrorCode fbx_GetVertices(struct FBXNode* node, FBXRawData* data)
{
	StardustErrorCode ret;
	char* vertexBytes;
	unsigned int vertexByteCount;

	if (node->properties[0].enc)
	{
		ret = zlib_Inflate(&vertexBytes, &vertexByteCount, node->properties[0].rawArr);
		if (ret != 0)
			return ret;
	}
	else
	{
		vertexBytes = node->properties[0].rawArr;
		vertexByteCount = node->properties[0].length;
	}

	unsigned int elementCount = vertexByteCount / 8; // Assuming double for now. Advance this later?

	
	//Allocate vertex array
	data->vertexData = malloc(sizeof(float) * elementCount);
	if (data->vertexData == 0) { if (node->properties[0].enc) { free(vertexBytes); } return STARDUST_ERROR_MEMORY_ERROR; }

	int idx = 0;
	for (unsigned int i = 0; i < elementCount; i++)
	{
		double v;
		for (int j = 0; j < 8; j++)
			((char*)&v)[j] = vertexBytes[idx++];
		(data->vertexData)[i] = (float)v;
	}

	data->vertexCount = elementCount / 3;
	data->dataType |= FBX_VERTICES;

	if (node->properties[0].enc) { free(vertexBytes); }
	return STARDUST_ERROR_SUCCESS;
}

StardustErrorCode fbx_GetIndices(struct FBXNode* node, FBXRawData* data)
{
	StardustErrorCode ret;
	char* indexBytes;
	unsigned int indexByteCount;

	// Decompress memory
	if (node->properties[0].enc)
	{
		ret = zlib_Inflate(&indexBytes, &indexByteCount, node->properties[0].rawArr);
		if (ret != 0)
			return ret;
	}
	else
	{
		indexBytes = node->properties[0].rawArr;
		indexByteCount = node->properties[0].length;
	}

	data->indexCount = indexByteCount / 4;


	// Allocate array
	data->indexData = malloc(sizeof(int) * data->indexCount);
	if (*data->indexData == 0) { if (node->properties[0].enc) { free(indexBytes); } return STARDUST_ERROR_MEMORY_ERROR; }

	// Unpack memory
	int idx = 0;
	for (unsigned int i = 0; i < data->indexCount; i++)
	{
		for (int j = 0; j < 4; j++)
			((char*)(&data->indexData[i]))[j] = indexBytes[idx++];
		//data->indexData[i] -= 1; // Subtract 1 to get an index starting at 0
	}

	data->dataType |= FBX_INDICES;

	if (node->properties[0].enc) { free(indexBytes); }
	return STARDUST_ERROR_SUCCESS;
}

StardustErrorCode fbx_GetNormals(struct FBXNode* node, FBXRawData* data)
{
	StardustErrorCode ret;

	// Get nodes
	int normIdx = fbx_GetNode(node, "Normals", 0);
	int refIdx = fbx_GetNode(node, "ReferenceInformationType", 0);

	if (normIdx == -1 || refIdx == -1)
		return STARDUST_ERROR_FILE_INVALID;

	// Get application type. Defer assignment of type until all data has been successfully loaded 
	FBXApplicationType type = fbx_GetApplicationType(node->children[refIdx]->properties[0].strArr, node->children[refIdx]->properties[0].length);

	{ // Get normals
		char* normalBytes;
		unsigned int normalByteCount;

		// Decompress memory
		if (node->children[normIdx]->properties[0].enc)
		{
			ret = zlib_Inflate(&normalBytes, &normalByteCount, node->children[normIdx]->properties[0].rawArr);
			if (ret != 0)
				return ret;
		}
		else
		{
			normalBytes = node->properties[0].rawArr;
			normalByteCount = node->properties[0].length;
		}


		unsigned int normalElementCount = normalByteCount / 8; // Assuming double

		// Allocate array
		data->normalData = malloc(sizeof(float) * normalElementCount);
		if (data->normalData == 0) { if (node->children[normIdx]->properties[0].enc) { free(normalBytes); } return STARDUST_ERROR_MEMORY_ERROR; }

		// Unpack memory
		int idx = 0;
		for (unsigned int i = 0; i < normalElementCount; i++)
		{
			double v;
			for (int j = 0; j < 8; j++)
				((char*)&v)[j] = normalBytes[idx++];
			data->normalData[i] = (float)v;
		}

		data->normalCount = normalElementCount / 3;

		if (node->children[normIdx]->properties[0].enc) { free(normalBytes); }
	}

	data->dataType |= FBX_NORMALS;

	if (type == FBX_DIRECT)
	{ 
		data->normalApplication = type;
		return STARDUST_ERROR_SUCCESS;
	}

	{ // Get Index data
		int normIdxIdx = fbx_GetNode(node, "NormalsIndex", 0);
		if (normIdxIdx == -1) { return STARDUST_ERROR_FILE_INVALID; }

		char* indexBytes;
		unsigned int indexByteCount;

		if (node->children[normIdxIdx]->properties[0].enc)
		{
			ret = zlib_Inflate(&indexBytes, &indexByteCount, node->children[normIdxIdx]->properties[0].rawArr);
			if (ret != 0)
				return ret;
		}
		else
		{
			indexBytes = node->properties[0].rawArr;
			indexByteCount = node->properties[0].length;
		}

		data->normalIndexCount = indexByteCount / 4;

		// Allocate memory
		data->normalIndexData = malloc(sizeof(unsigned int) * data->normalIndexCount);
		if (data->normalIndexData == 0) { if (node->children[normIdxIdx]->properties[0].enc) { free(indexBytes); }return STARDUST_ERROR_MEMORY_ERROR; }

		// Unpack memory
		int idx = 0;
		for (unsigned int i = 0; i < data->normalIndexCount; i++)
		{
			for (int j = 0; j < 4; j++)
				((char*)(&data->normalIndexData[i]))[j] = indexBytes[idx++];
			data->normalIndexData[i] -= 1; // Subtract 1 to get an index starting at 0
		}

		data->normalApplication = type;

		if (node->children[normIdx]->properties[0].enc) { free(indexBytes); }
	}

	return STARDUST_ERROR_SUCCESS;
}

StardustErrorCode fbx_GetTextureCoords(struct FBXNode* node, FBXRawData* data)
{
	StardustErrorCode ret;

	// Get nodes
	int normIdx = fbx_GetNode(node, "UV", 0);
	int refIdx = fbx_GetNode(node, "ReferenceInformationType", 0);

	if (normIdx == -1 || refIdx == -1)
		return STARDUST_ERROR_FILE_INVALID;

	// Get application type
	FBXApplicationType type = fbx_GetApplicationType(node->children[refIdx]->properties[0].strArr, node->children[refIdx]->properties[0].length);

	{ // Get normals
		char* uvBytes;
		unsigned int ubByteCount;

		// Decompress memory
		if (node->children[normIdx]->properties[0].enc)
		{
			ret = zlib_Inflate(&uvBytes, &ubByteCount, node->children[normIdx]->properties[0].rawArr);
			if (ret != 0)
				return ret;
		}
		else
		{
			uvBytes = node->children[normIdx]->properties[0].rawArr;
			ubByteCount = node->children[normIdx]->properties[0].length;
		}

		unsigned int uvElementCount = ubByteCount / 8; // Assuming double

		// Allocate array
		data->uvData = malloc(sizeof(float) * uvElementCount);
		if (data->uvData == 0) { if (node->children[normIdx]->properties[0].enc) { free(uvBytes); } return STARDUST_ERROR_MEMORY_ERROR; }

		// Unpack memory
		int idx = 0;
		for (unsigned int i = 0; i < uvElementCount; i++)
		{
			double v;
			for (int j = 0; j < 8; j++)
				((char*)&v)[j] = uvBytes[idx++];
			(data->uvData)[i] = (float)v;
		}

		data->uvCount = uvElementCount / 2;

		if (node->children[normIdx]->properties[0].enc) { free(uvBytes); }
	}

	data->dataType |= FBX_UVS;

	if (type == FBX_DIRECT)
	{
		data->uvApplication = type;
		return STARDUST_ERROR_SUCCESS;
	}

	{ // Get Index data
		int uvIdxIdx = fbx_GetNode(node, "UVIndex", 0);
		if (uvIdxIdx == -1) { return STARDUST_ERROR_MEMORY_ERROR; }

		char* indexBytes;
		unsigned int indexByteCount;

		if (node->children[uvIdxIdx]->properties[0].enc)
		{
			ret = zlib_Inflate(&indexBytes, &indexByteCount, node->children[uvIdxIdx]->properties[0].rawArr);
			if (ret != 0) { return STARDUST_ERROR_FILE_INVALID; }
		}
		else
		{
			indexBytes = node->children[uvIdxIdx]->properties[0].rawArr;
			indexByteCount = node->children[uvIdxIdx]->properties[0].length;
		}

		data->uvIndexCount = indexByteCount / 4;

		// Allocate memory
		data->uvIndexData = malloc(sizeof(unsigned int) * data->uvIndexCount);
		if (data->uvIndexData == 0) { if (node->children[uvIdxIdx]->properties[0].enc) { free(indexBytes); } return STARDUST_ERROR_MEMORY_ERROR; }

		// Unpack memory
		int idx = 0;
		for (unsigned int i = 0; i < data->uvIndexCount; i++)
		{
			for (int j = 0; j < 4; j++)
				((char*)(&data->uvIndexData[i]))[j] = indexBytes[idx++];
			data->uvIndexData[i] -= 1; // Subtract 1 to get an index starting at 0
		}

		data->uvApplication = type;
		if (node->children[uvIdxIdx]->properties[0].enc) { free(indexBytes); }
	}

	return STARDUST_ERROR_SUCCESS;
}

unsigned int* fbx_GenerateDirectIndices(unsigned int count)
{
	unsigned int* arr = malloc(sizeof(unsigned int) * count);
	if (arr == 0)
		return 0;

	for (unsigned int i = 0; i < count; i++)
		arr[i] = i;

	return arr;
}

StardustErrorCode fbx_CompactArray(float* arr, unsigned int* indices, unsigned int* arrSize, unsigned int elementStride, unsigned int indexSize)
{
	float* newArr = malloc(sizeof(float) * *arrSize); // Allocate for worst case scenario
	if (newArr == 0)
		return STARDUST_ERROR_MEMORY_ERROR;
	int newIdx = 0;

	for (unsigned int i = 0; i < indexSize; i++)
	{
		int index = indices[i];

		// Check if vertex in in arr
		for (int j = 0; j < index; j++)
		{
			int testIndex = indices[i];

			int match = 1;
			for (unsigned int k = 0; k < elementStride; k++)
				match &= arr[index + k] == arr[testIndex + k];

			if (match)
				indices[i] = testIndex;
			else
			{
				for (unsigned int k = 0; k < elementStride; k++)
					newArr[newIdx++] = arr[index + k];
				indices[i] = newIdx - 1;
			}
		}
	}

	return STARDUST_ERROR_SUCCESS;
}

StardustErrorCode fbx_ComputeHashAndIndexArray(FBXRawData* data, struct FBXVertexHash** hashArr, unsigned int* indexArr, unsigned int* arrSize)
{
	// Create hash array
	*hashArr = malloc(sizeof(struct FBXVertexHash) * data->indexCount);
	if (*hashArr == 0)
		return STARDUST_ERROR_MEMORY_ERROR;

	*arrSize = 0;
	for (unsigned int i = 0; i < data->indexCount; i++)
	{
		// Get sign of index
		unsigned int polyIndex = (unsigned int)data->indexData[i];
		int sign = data->indexData[i] < 0 ? -1 : 1; // Get sign (would bit shifting be more optimized?)
		//polyIndex *= sign; // Ensure positive

		struct FBXVertexHash currHash = { 0 };

		// Hash current index
		{
			currHash.nrmIdx = -1; // Initialise to -1. If -1 then the attrib is not used
			currHash.uvIdx = -1;

			// Normal
			if ((data->dataType & FBX_NORMALS) == FBX_NORMALS)
			{
				currHash.hash += (data->normalIndexData[i] * data->uvCount * data->vertexCount);
				currHash.nrmIdx = data->normalIndexData[i];
			}

			// UV
			if ((data->dataType & FBX_UVS) == FBX_UVS)
			{
				currHash.hash += (data->uvIndexData[i] * data->vertexCount);
				currHash.uvIdx = data->uvIndexData[i];
			}

			// Vertex 
			currHash.hash += polyIndex * sign;
			currHash.vtxIdx += polyIndex * sign;
			if (sign == -1)
				currHash.vtxIdx -= 1;
		}

		// Check if currHash is within hashArr
		char found = 0; // Using char because an entire int isn't needed
		for (unsigned int j = 0; j < *arrSize; j++)
		{
			if ((*hashArr)[j].hash == currHash.hash)
			{
				// Hash already exists. Ammend index arr
				indexArr[i] = (int)j * sign; // Ensure that the sign is correct
				found = 1; // Set flag
				break;
			}
		}

		if (found) // Skip
			continue;

		// First occurance of hash. Add to arr
		(*hashArr)[*arrSize] = currHash;

		// Update index
		indexArr[i] = (*arrSize) * sign; // Ensure sign is correct

		// Increment arrSize
		(*arrSize)++;
	}

	return STARDUST_ERROR_SUCCESS;
}

StardustErrorCode fbx_FormatIndexArray(FBXRawData* data, unsigned int** indexArr)
{
	// The FBX format stores polygons in loops of indices. The end of a loop is denoted by a negative indice
	// Ignore loop size for now
	// TODO: Add dynamic polygon sizes


	// Allocated indexArr
	*indexArr = malloc(sizeof(unsigned int) * data->indexCount);
	if (*indexArr == 0)
		return STARDUST_ERROR_MEMORY_ERROR;

	// Copy in indices and ABS
	for (unsigned int i = 0; i < data->indexCount; i++)
	{
		// Get sign
		int sign = data->indexData[i] < 0 ? -1 : 1;

		// Absolute index
		(*indexArr)[i] = data->indexData[i] * sign;
	}

	return STARDUST_ERROR_SUCCESS;
}

FBXApplicationType fbx_GetApplicationType(const char* attrib, unsigned int len)
{
	int match = 1;

	// Direct
	int idx = 0;
	while (1)
	{
		if (FBXApplicationDirectStr[idx] == 0)
			break;

		if ((unsigned int)idx >= len)
		{
			match = 0;
			break;
		}
		
		match &= attrib[idx] == FBXApplicationDirectStr[idx];
		
		idx++;
	}

	if (match)
		return FBX_DIRECT;

	match = 1;
	idx = 0;

	while (1)
	{
		if (FBXApplicationIndexToDirectStr[idx] == 0)
			break;

		if ((unsigned int)idx >= len)
		{
			match = 0;
			break;
		}

		match &= attrib[idx] == FBXApplicationIndexToDirectStr[idx];

		idx++;
	}

	if (match)
		return FBX_INDEX_TO_DIRECT;

	return 0;

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

	char* nodeName = malloc(nameLen + 1);
	if (nodeName == 0)
	{
		*result = STARDUST_ERROR_MEMORY_ERROR;
		return 0;
	}
	*result = fs_ReadBytes(stream, nodeName, nameLen);
	if (*result != 0)
	{
		free(nodeName);
		*result = STARDUST_ERROR_IO_ERROR;
		return 0;
	}

	nodeName[nameLen] = 0;

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
