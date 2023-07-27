
#ifndef _STARDUST_HEADER
#define _STARDUST_HEADER

#include "internal.h"

/*
DOCUMENTATION:

	Startdust is a library for loading resources.
	It's primary job is loading 3D files but this is set to expand to loading images

	Function naming convention:
		sd_ followed by the function name. Function names are written in camel case.
		I.E -> sd_LoadMesh

	Loading Meshes:
		Loading Meshes is done by calling the sd_LoadMesh(const char* filepath, MeshLoadFlags flags) which will return a 
		StardustMesh object.

		Once you have extracted the data from StardustMesh object the object can be deleted using the sd_FreeMesh(StardustMesh* mesh)
		function

*/

// ================== Defines ================== //
#define MAX_LINE_BUFFER_SIZE 256

// ================== Types ================== //
#include <stdint.h>

enum MeshFlags
{
	STARDUST_MESH_IGNORE_NORMALS = 1 << 0,			//Doesn't load normals from file.
	STARDUST_MESH_GENERATE_NORMALS = 1 << 1,		//Genrates mesh normals after loading the mesh
	STARDUST_MESH_SMOOTH_NORMALS = 1 << 2,			//Smooths any normals
	STARDUST_MESH_HARDEN_NORMALS = 1 << 3,			//Hardens any normals

	STARDUST_MESH_IGNORE_UVS = 1 << 4,				//Doesn't load texture coordinates from file

	STARDUST_MESH_TRIANGULATE = 1 << 5,			//Triangulate mesh. Safe to call on pretriangulated meshes.
	
	STARDUST_MESH_MERGE_MESHES = 1 << 6,			//Merges all meshes into a single mesh
	STARDUST_MESH_USE_FIRST_MESH = 1 << 7			//Only uses first mesh found in file
};

enum MeshDataFlags
{
	STARDUST_VERTEX_DATA = 1 << 0,
	STARDUST_INDEX_DATA = 1 << 1,
	STARDUST_TEXTURE_DATA = 1 << 2,
	STARDUST_NORMAL_DATA = 1 << 3,
	STARDUST_COLOR_DATA = 1 << 4,
	STARDUST_SMOOTHSHADING = 1 << 5
};

enum ErrorCodes
{
	STARDUST_ERROR_SUCCESS = 1 << 0,
	STARDUST_ERROR_FILE_NOT_FOUND = 1 << 1,
	STARDUST_ERROR_FORMAT_NOT_SUPPORTED = 1 << 2,
	STARDUST_ERROR_LINE_EXCCEDS_BUFFER = 1 << 3,
	STARDUST_ERROR_FILE_INVALID = 1 << 4,
	STARDUST_ERROR_IO_ERROR = 1 << 5,
};

typedef unsigned int StardustMeshFlags;
typedef unsigned int StardustErrorCode;
typedef unsigned int StardustDataType;

// ================== Structs ================== //
typedef struct
{
	float		x;				// X position of the vertex
	float		y;				// Y position of the vertex
	float		z;				// Z position of the vertex
	float		w;				// W positoon of the vertex. Defaults to 1.0

	float		r;				// Red value for vertex color
	float		g;				// Green value for vertex color
	float		b;				// Blue value for vertex color

	float		normX;			//Normal X component
	float		normY;			//Normal Y component
	float		normZ;			//Normal Z component

	float		texU;			//U coord of UVW
	float		texV;			//V coord of UVW
	float		texW;			//W coord of UVW
} Vertex;

typedef struct 
{
	StardustDataType dataType;		//Types of data contained in the mesh

	Vertex*			vertices;		//Vertex array
	uint32_t*		indices;		//Index Array	

	uint32_t		vertexCount;	//Number of vertices in the vertices array
	uint32_t		indexCount;		//Number of indices in the indices array

	uint32_t		vertexStride;//Number of verticies per face

} StardustMesh; //Mesh structure. Retured in arrays of each individual componenets

//Function prototypes
STARDUST_FUNC size_t sd_LoadMesh(const char* filename, StardustMeshFlags flags, StardustErrorCode* result, StardustMesh** meshes);
STARDUST_FUNC void sd_FreeMesh(StardustMesh* mesh);

STARDUST_FUNC int sd_isFormatSupported(const char* format);

// Vertex Functions

/// <summary>
/// Prints a given vertex
/// </summary>
/// <param name="v">Vertex to print</param>
/// <returns></returns>
STARDUST_FUNC void sd_PrintVertex(Vertex* v);

/// <summary>
/// Compares two vertices' positions
/// </summary>
/// <param name="a">Vertex A</param>
/// <param name="b">Vertex B</param>
/// <returns>1 for eq, 0 for neq</returns>
STARDUST_FUNC int sd_CompareVertexPosition(Vertex* a, Vertex* b);
STARDUST_FUNC int sd_CompareVertex(Vertex* a, Vertex* b);

//STARDUST_FUNC const char* sd_TranslateError(StardustErrorCode code);

#endif //_STARDUST_HEADER