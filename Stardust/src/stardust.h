
#ifndef _STARDUST_HEADER
#define _STARDUST_HEADER

#include "internal.h"

/*

MIT License

Copyright (c) 2023 Matthew Tindley

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

/*
DOCUMENTATION:

	Startdust is a library for loading resources.
	It's primary job is loading 3D files but this is set to expand to loading images

	Function naming convention:
		All functions have a prefix containing their category followed by their name in camel case
		Function Prefixes:
			sd -> stardust function. Usually user facing
			_post -> postprocessing function. Internal
			_obj -> OBJ loader function. Internal
			
	Defined Types:
		Stardust contains a couple of custom types to help with organistion

		StardustMeshFlags -> unsigned integer to hold mesh flags
		StardustErrorCode -> unsigned integer to hold error enums
		StardustMeshDataType -> unsigned integer to hold the type of data in a mesh


	StardustMesh Object:
		Meshes are stored in StardustMesh structs. These contain the vertex and index arrays of the mesh
		
		StardustMeshDataType dataType -> The forms of data stored in the vertices. This includes normal data, vertex data, uv data, smoothing, etc...
		Vertex* vertices -> The vertex array. This contains the mesh vertex data
		uint32_t* indices -> The index array. This contains the mesh index data
		uint32_t vertexCount -> The amount of vertices in the vertex array
		uint32_t indexCount -> the amount of indices in the index array
		uint32_t vertexStride -> The amount of indices per face.


	Loading Meshes:
		Loading Meshes is done by calling the StardustErrorCode sd_LoadMesh(const char* filename, StardustMeshFlags flags, StardustMesh** meshes, size_t* count);

		This function will load the given file and, if the format is supported, load it into an array of meshes.
		The length of this array is given by meshCount and the array is placed in a pointer pointed to by meshes
		The function returns a StardustErrorCode, if this is equal to STARDUST_ERROR_SUCCESS the operation completed succesfully
		and the data inside can be trusted.

	Deleting Meshes:
		To delete a mesh call the sd_FreeMesh() function on the mesh. This works on individual meshes so ensure that every mesh in the returned array is deleted

*/

// ================== Defines ================== //
#define MAX_LINE_BUFFER_SIZE 256 //Maximum line size when reading files. Only applies to file formats that use EOLs

// ================== Types ================== //
#include <stdint.h>

enum MeshFlags
{
	STARDUST_MESH_IGNORE_NORMALS = 1 << 0,			//Doesn't load normals from file.
	STARDUST_MESH_GENERATE_NORMALS = 1 << 1,		//Genrates mesh normals after loading the mesh
	STARDUST_MESH_HARDEN_NORMALS = 1 << 1,			//Hardens any normals
	STARDUST_MESH_SMOOTH_NORMALS = 1 << 2,			//Smooths any normals

	STARDUST_MESH_IGNORE_TEXCOORDS = 1 << 4,				//Doesn't load texture coordinates from file

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
	STARDUST_ERROR_SUCCESS = 0,
	STARDUST_ERROR_FILE_NOT_FOUND = 1,
	STARDUST_ERROR_FORMAT_NOT_SUPPORTED = 2,
	STARDUST_ERROR_LINE_EXCCEDS_BUFFER = 3,
	STARDUST_ERROR_FILE_INVALID = 4,
	STARDUST_ERROR_IO_ERROR = 5,
	STARDUST_ERROR_MEMORY_ERROR = 6,
	STARDUST_ERROR_EOF = 7
};

typedef unsigned int StardustMeshFlags;
typedef unsigned int StardustErrorCode;
typedef unsigned int StardustMeshDataType;

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
	StardustMeshDataType dataType;		//Types of data contained in the mesh

	Vertex*			vertices;		//Vertex array
	uint32_t*		indices;		//Index Array	

	uint32_t		vertexCount;	//Number of vertices in the vertices array
	uint32_t		indexCount;		//Number of indices in the indices array

	uint32_t		vertexStride;//Number of verticies per face

} StardustMesh; //Mesh structure. Retured in arrays of each individual componenets

//Function prototypes
STARDUST_FUNC StardustErrorCode sd_LoadMesh(const char* filename, const StardustMeshFlags flags, StardustMesh** meshes, size_t* meshCount);
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

//Error Functions
STARDUST_FUNC const char* sd_TranslateError(StardustErrorCode error);

#endif //_STARDUST_HEADER