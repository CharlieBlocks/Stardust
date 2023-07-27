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

typedef struct 
{
	uint32_t* indices;
	uint32_t vertexCount;
	Vertex normal;
} Polygon;

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



// Triangulation //

/// <summary>
/// Triangulates a mesh using the ear clipping method.
/// This will only effect the indices of the mesh and will not generate any extra vertices.
/// Can be called on a mesh that is already triangulated as it will early exit.
/// </summary>
/// <param name="mesh"></param>
void _post_TriangulateMeshEC(StardustMesh* mesh);

/// <summary>
/// Triangulates a polygon and places the triangulated indices into indexArray
/// </summary>
/// <param name="mesh">Mesh containing the vertices that the poly is indexed from<\param>
/// <param name="poly"></param>
/// <param name="indexArray"></param>
/// <returns></returns>
uint32_t _post_TriangulatePolygonEC(StardustMesh* mesh, Polygon* poly, uint32_t* indexArray);

/// <summary>
/// Calculates which vertices of a mesh polygon are convex or reflexive.
/// Uses the provided mesh to find the vertices.
/// This is kind of unsafe as the memory for the array is allocated outside of the function and many C programmers are screaming at me for that.
/// However, for this usecase where I'm going to reuse this function and would like to minimise allocations. This works fine.
/// Just don't call this function without an array of size mesh->vertexCount or larger.
/// </summary>
/// <param name="mesh">Mesh with vertices to check</param>
/// <param name="poly">Polygon to find vertices from</param>
/// <param name="convexIndices">Array of indices to fill for convex vertices</param>
/// <param name="concaveVertices">Array of indices to fill for concave vertices</param>
/// <returns>Number of convex vertices found</returns>
uint32_t _post_FillConvexConcaveVertices(StardustMesh* mesh, Polygon* poly, uint32_t* convexIndices, uint32_t* concaveVertices);

/// <summary>
/// Checks if a given convex vertex is an ear.
/// It checks that no concave vertex is present within the triangle formed by the two adjacent vertices around the convex index.
/// See above function for memory managment justification. Same here, ensure that the given array is of size mesh->vertexCount
/// </summary>
/// <param name="mesh">Mesh with vertices to check</param>
/// <param name="convexIndices">Array filled with convex indices</param>
/// <param name="convexCount">Size of convex array</param>
/// <param name="concaveIndices">Array filled with concave indices</param>
/// <param name="concaveCount">Size of concave array</param>
/// <param name="earArray">Array to fill with ear indices</param>
/// <returns>The number of ears in the earArray</returns>
uint32_t _post_FillEars(StardustMesh* mesh, Polygon* poly, uint32_t* convexIndices, uint32_t convexCount, uint32_t* concaveIndices, uint32_t concaveCount, uint32_t* earArray);


// Util Funcs //

/// <summary>
/// Removes duplicate verticies from a mesh
/// This will update the mesh verticies and the mesh indices along with their respective counts
/// This is slower than the hash method that can be used by a loader
/// </summary>
/// <param name="mesh">Mesh to be reduced</param>
void _post_RecomputeIndexArray(StardustMesh* mesh);

int _post_ComarePositions(NormalPosition* norm, Vertex* vertex);
int _post_IsVertexConvex(Vertex* vertex, Vertex* Vprev, Vertex* Vnext, Vertex* normal);
int _post_TriangleContainsPoint(Vertex* a, Vertex* b, Vertex* c, Vertex* p);
void _post_RemoveFromPolygon(Polygon* poly, uint32_t count, uint32_t idx);
void _post_CalculatePolygonNormal(StardustMesh* mesh, Polygon* poly);


#endif