#ifndef _STARDUST_OBJ_LOADER
#define _STARDUST_OBJ_LOADER

#include "stardust.h"

#include <stdio.h>

enum OBJTagTypes
{
	OBJTAG_VERTEX = 1 << 0,
	OBJTAG_TEXCOORD = 1 << 1,
	OBJTAG_NORMAL = 1 << 2,
	OBJTAG_FACE = 1 << 3,
	OBJTAG_SMOOTH = 1 << 4
};

typedef struct
{
	//Inuse tags
	unsigned int tags;

	//Tag counters
	uint32_t vertexTagCount;
	uint32_t texCoordTagCount;
	uint32_t normalTagCount;
	uint32_t faceTagCount;

	//Per component
	uint32_t elementsPerVertex;
	uint32_t elementsPerTexCoord;
	uint32_t elementsPerFace;
	uint32_t indicesPerVertex;

	//Position counters
	uint32_t vertexPosition;
	uint32_t texCoordPosition;
	uint32_t normalPosition;
	uint32_t hashPosition;
	uint32_t indexPosition;

	//Tag arrays
	float* vertices;
	float* texCoords;
	float* normals;
	uint32_t* vertexHashes; //Unique hash
	uint32_t* indices; //List of hash indices

} OBJTags;

typedef struct
{
	char* name;

	OBJTags* tags;
} OBJObject;

//Functions

StardustErrorCode _obj_LoadMesh(const char* file, const StardustMeshFlags flags, StardustMesh** mesh, size_t* count);

OBJObject* _obj_GetObjects(FILE* file, StardustErrorCode* result, size_t* objectCount, const int useFirstMesh);
StardustErrorCode _obj_AllocateTagArrays(OBJTags* tags);

void _obj_RemoveTags(OBJObject* objects, const size_t objectCount, const StardustMeshFlags flags);

StardustErrorCode _obj_FillObjectData(FILE* file, OBJObject* objects, const size_t objectCount);
StardustErrorCode _obj_FillMeshes(StardustMesh* meshes, OBJObject* objects, const size_t objectCount);

void _obj_ParseVector(char** string, float* arr, const uint32_t elemCount);

uint32_t _obj_GetHash(char* string, OBJTags* tags);
int _obj_HashInArray(uint32_t* arr, size_t arrSize, uint32_t hash);
void _obj_DecomposeHash(uint32_t hash, OBJTags* tags, uint32_t* vI, uint32_t* tI, uint32_t* nI);

void _obj_FreeObject(OBJObject* obj);
void _obj_FreeObjects(OBJObject* objs, size_t count);
void _obj_FreeTags(OBJTags* tags);
/*StardustErrorCode sd_LoadOBJFromFile(const char* file, StardustMeshFlags flags, StardustMesh** mesh);

int _obj_Parse3ComponentLine(char** line, size_t elemCount, float* arr);
int _obj_Parse2ComponentLine(char** line, size_t elemCount, float* arr);
int _obj_ParseFaceLine(char** line, size_t elemCount, size_t verticesPerFace, uint16_t* indices);
uint64_t _obj_GetVertexHash(char* vertexStr, size_t totalVertices, size_t totalUVS, size_t totalNormals);
void _obj_DecomposeVertexHash(uint64_t hash, uint32_t vertexCount, uint32_t uvCount, uint32_t* vertexIndex, uint32_t* uvIndex, uint32_t* normalIndex);
int _obj_isInArray(uint64_t* arr, uint32_t arrSize, uint64_t val);*/

#endif // _STARDUST_OBJ_LOADER