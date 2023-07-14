#ifndef _STARDUST_POSTPROCESSING
#define _STARDUST_POSTPROCESSING

/*
Documentation
*/

#include "stardust.h"

typedef struct
{
	float x;
	float y;
	float z;
	float w;

	uint32_t index;
} NormalPosition;

// ==================== Functions ==================== //

/// <summary>
/// Performs post processing on a mesh based on the provided flags.
/// </summary>
/// <param name="mesh">Mesh pointer to be processed. This will change the underlying object</param>
/// <param name="flags">The appropriate post processing flags</param>
void _post_PerformPostProcessing(StardustMesh* mesh, StardustMeshFlags flags);



// Smoothing //

/// <summary>
/// Smooths the normals of mesh.
/// This may reduce the amount of verticies in the mesh.
/// This is done by getting all verticies of the same position and averaging their normals.
/// </summary>
/// <param name="mesh">Mesh to be smoothed</param>
void _post_SmoothNormals(StardustMesh* mesh);



// Hardening //

/// <summary>
/// Hardens the normals of a mesh
/// This may increase the amount of verticies in the mesh
/// This is done by calculating the face normal of each mesh and creating verticies using it.
/// This function will recalculate the indices of the mesh
/// </summary>
/// <param name="mesh"></param>
void _post_HardenNormals(StardustMesh* mesh);



// Util Funcs //

/// <summary>
/// Removes duplicate verticies from a mesh
/// This will update the mesh verticies and the mesh indices along with their respective counts
/// This is slower than the hash method that can be used by a loader
/// </summary>
/// <param name="mesh">Mesh to be reduced</param>
void _post_RecomputeIndexArray(StardustMesh* mesh);


int _post_ComarePositions(NormalPosition* norm, Vertex* vertex);


#endif