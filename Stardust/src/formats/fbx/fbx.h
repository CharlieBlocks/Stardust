#ifndef _FBX
#define _FBX

/*
TODO: 
	Use bitwise sign and abs func
	Add support for n-gons and iregular meshes
	Add support for materials
*/

/*
FBX is a proprietry file format developed by autodesk. Due to this some hackiness/reverse engineering is required and not all files will work
Shoutout to Blender for both being open source and having a blogpost on how they are structured.

Header:
	To identify an FBX file check for the magic header of "Kaydara FBX Binary\x00\x20\x00\x1a\x00".

	Versions:
		FBX have a couple versions denoted in the header of the file. These are represented as integers like 7100 for 7.1 (Blender exports 7.4 | 7400).
		In more recent fbx versions (>=7.5|7500) they switch endOffset, propCount and porpListLen to uint64s for LFS.


FBXs are structured in a simillar mannger to XMLs

Node : prop1, prop2, ... {
	SubNodes
}

In binary each node is prepreseted by the format
	endOffset - uint32 - The distance from the beginning of the file to the end of the node
	propCount - uint32 - The number of properties in the node
	propListLen - uint32 - The length of the property list, in bytes I think
	nameLen - uint8 - Length of the node noame
	name - char - The name for length nameLen. Does not have a null terminator

	Property Structure:
		type - char - a character that represents the following data type
		data - ? - The data

	... - May contain a list of properties. This can be calculated by checking if the character index is less than the endoffset-13. See below

	nullTerminator - 13 bytes of \x00 - If a node contains 1 or more nested nodes it will have a 13 byte null terminator. If there are 0 nodes present then it will not contain a terminator 

When creating an FBX tree there is an implicit 'global' node. This node encapsulates all nodes in the file.



Property data types
	Scalars:
		'Y': int16
		'C': bool (least sig bit of a byte)
		'I': int32
		'F': float (4 bytes, IEEE754)
		'D': double (8 bytes)
		'L': long (int64)

	Arrays: (Have a special format. See below)
		'f': Array of floats
		'd': Array of doubles
		'l': Array of longs
		'i': Array of int32s
		'b': Array of booleans (still 1 byte each)
	
	Special: (Have special format. See below)
		'S': String (Strings are not null terminated by may contain null characters)
		'R': Raw binary

Array Property format:
	Array Length - uint32 - Array length (int elements not bytes)
	Encoding - uint32 - Blender devs have only observed two forms of this. 0 for raw, 1 for encoded using zlib
	CompressedLength - uint32 - The lenght of the array if it is compressed. If encoding is 1 this should be used as the length instead. (Raw byte count)
	Contents - ? - The array contents

Special Property format:
	Length - uint32 - The length of the data in raw bytes
	Contents - byte/char - Array of data type of length L


When loading a mesh from an FBX file all the data will be contained within the "Objects" node.
Within this node there is a "Geometry" node which contains all the mesh geometry. This geometry node contains the UUID of the mesh, it's name. And mesh/line/... identifiers

A "Geometry" node has it's vertices, indices and edges as requirements. It can then contain other "layers" of information like normal data and texture coordinates
	Vertex data is in the subnode "Vertices"
	indices are in the subnode "PolygonVertexIndex"
	edges are in the subnode "Edges"

The mesh may also contain other layers. These are denoted by the labels LayerElement<type> (no space intended). They will also have the version number 101.
Each layer contains information about how it should be mapped to the mesh aswell as it's raw data
	MappingInformationType - How should the mesh data be passed? ByPolygonVertex = Per vertex, AllSame = Uniform?
	ReferenceInformationType - How should the mesh data be referenced? Direct = Part of each individual vertex, IndexToDirect = By indices. Not 100% sure about this




Current Features: 
	None

In progress Features:
	Mesh Loading for 7100 - 7400


Loading flow
	1. Call _fbx_loadMesh(path, flags, meshes, meshcount);
	2. Read in FBX header. If version is >= 7500 call _fbx_createTree_64() else if version >= 7100 _fbx_createTree_32() else ERROR
	3. Validate the presence of the 'Objects' tag
	4. Locate meshes ('Geometry' tag)
	5. Read in vertices and indices
	6. Read in other layers
	7. Structure stardust Vertex types
	8. Create mesh
*/

#include "stardust.h"
#include "utils/filestream.h"
//#include "filetools.h"

#define MIN_FBX_VER 7100
#define MAX_FBX_VER 7400

typedef enum
{
	FBX_INT16 = 0,
	FBX_BOOL = 1,
	FBX_INT32 = 2,
	FBX_FLOAT = 3,
	FBX_DOUBLE = 4,
	FBX_LONG = 5,

	FBX_FLOAT_ARR = 6,
	FBX_DOUBLE_ARR = 7,
	FBX_LONG_ARR = 8,
	FBX_INT_ARR = 9,
	FBX_BOOL_ARR = 10,

	FBX_STRING = 11,
	FBX_RAW = 12

} FBXPropertyType;

typedef enum
{
	FBX_DIRECT = 1,
	FBX_INDEX_TO_DIRECT = 2
} FBXApplicationType;

typedef enum
{
	FBX_VERTICES = 1 << 0,
	FBX_INDICES = 1 << 1,
	FBX_NORMALS = 1 << 2,
	FBX_UVS = 1 << 3
} FBXDataType;

static const char* FBXApplicationDirectStr = "Direct";
static const char* FBXApplicationIndexToDirectStr = "IndexToDirect";

static int FBXPropertyDictInit = 0;
static FBXPropertyType FBXPropertyDict[109];

typedef struct {
	
	FBXPropertyType type;

	uint32_t length;
	int8_t enc;
	uint32_t compLen;

	union {
		void* raw;
		int16_t* i16;
		int8_t* b;
		int32_t* i32;
		float* f;
		double* d;
		int64_t* i64;

		float* fArr;
		double* dArr;
		int64_t* i64Arr;
		int32_t* i32Arr;
		int8_t* bArr;

		char* strArr;
		char* rawArr;
	};

} FBXProperty;

struct FBXNode
{
	uint32_t endOffset;
	uint32_t propertyCount;

	int8_t nameLen;
	char* name;

	FBXProperty* properties;

	uint32_t childCount;
	struct FBXNode** children;
};

typedef struct
{
	FBXDataType dataType;

	float* vertexData;
	unsigned int vertexCount;

	int* indexData;
	unsigned int indexCount;

	FBXApplicationType normalApplication;
	float* normalData;
	unsigned int* normalIndexData;
	unsigned int normalCount;
	unsigned int normalIndexCount;

	FBXApplicationType uvApplication;
	float* uvData;
	unsigned int* uvIndexData;
	unsigned int uvCount;
	unsigned int uvIndexCount;
} FBXRawData;

struct FBXVertexHash
{
	unsigned int nrmIdx;
	unsigned int uvIdx;
	unsigned int vtxIdx;
	unsigned int hash;
};

/// <summary>
/// Loads an FBX file
/// </summary>
/// <param name="path">Path to the fbx file</param>
/// <param name="flags">Load flags</param>
/// <param name="meshes">Pointer to mesh array</param>
/// <param name="meshCount">Pointer to a value of the number of meshes that were loaded</param>
/// <returns>Error code</returns>
StardustErrorCode _fbx_LoadMesh(const char* path, const StardustMeshFlags flags, StardustMesh** meshes, size_t* meshCount);

StardustErrorCode _fbx_GetHeader(FileStream* stream, uint32_t* version);

// Mesh functions
StardustErrorCode fbx_GetMesh(struct FBXNode* globalNode, StardustMesh** meshes, const StardustMeshFlags flags);
int fbx_GetNode(struct FBXNode* globalNode, const char* label, int startAt);
void fbx_FreeRawData(FBXRawData* data);

StardustErrorCode fbx_GetVertices(struct FBXNode* node, FBXRawData* data);
StardustErrorCode fbx_GetIndices(struct FBXNode* node, FBXRawData* data);
StardustErrorCode fbx_GetNormals(struct FBXNode* node, FBXRawData* data);
StardustErrorCode fbx_GetTextureCoords(struct FBXNode* node, FBXRawData* data);
unsigned int* fbx_GenerateDirectIndices(unsigned int count);

StardustErrorCode fbx_CompactArray(float* arr, unsigned int* indices, unsigned int* arrSize, unsigned int elementStride, unsigned int indexSize);
StardustErrorCode fbx_ComputeHashAndIndexArray(FBXRawData* data, struct FBXVertexHash** hashArr, unsigned int* indexArr, unsigned int* arrSize);
StardustErrorCode fbx_FormatIndexArray(FBXRawData* data, unsigned int** indexArr);

FBXApplicationType fbx_GetApplicationType(const char* attrib, unsigned int len);

// Node Functions
StardustErrorCode _fbx_GetGlobalNode(FileStream* stream, struct FBXNode* globalNode);
struct FBXNode* _fbx_GetNode(FileStream* stream, StardustErrorCode* result);
StardustErrorCode _fbx_GetProperty(FileStream* stream, FBXProperty* prop);

void _fbx_FreeGlobalNode(struct FBXNode* node);
void _fbx_FreeNode(struct FBXNode* node);
void _fbx_FreeProperty(FBXProperty* prop);

//Dict function
void _fbx_InitFBXPropertyDict();

// Version load

/// <summary>
/// Loads an FBX file that has uint32 property sizes 
/// </summary>
/// <param name="path"></param>
/// <param name="flags"></param>
/// <param name="meshes"></param>
/// <param name="meshCount"></param>
/// <returns></returns>
//StardustErrorCode _fbx_LoadMesh32(const char* path, const StardustMeshFlags flags, StardustMesh** meshes, size_t* meshCount);


#endif