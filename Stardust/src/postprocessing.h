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
StardustErrorCode _post_PerformPostProcessing(StardustMesh* mesh, StardustMeshFlags flags);



// Smoothing //

/// <summary>
/// Smooths the normals of mesh.
/// This may reduce the amount of verticies in the mesh.
/// This is done by getting all verticies of the same position and averaging their normals.
/// </summary>
/// <param name="mesh">Mesh to be smoothed</param>
StardustErrorCode _post_SmoothNormals(StardustMesh* mesh);



// Hardening //

/// <summary>
/// Hardens the normals of a mesh
/// This may increase the amount of verticies in the mesh
/// This is done by calculating the face normal of each mesh and creating verticies using it.
/// This function will recalculate the indices of the mesh
/// </summary>
/// <param name="mesh"></param>
StardustErrorCode _post_HardenNormals(StardustMesh* mesh);



// Triangulation //

/// <summary>
/// Triangulates a mesh using the ear clipping method.
/// This will only effect the indices of the mesh and will not generate any extra vertices.
/// Can be called on a mesh that is already triangulated as it will early exit.
/// </summary>
/// <param name="mesh"></param>
StardustErrorCode _post_TriangulateMeshEC(StardustMesh* mesh);

/// <summary>
/// Triangulates a polygon and places the triangulated indices into indexArray
/// Assumes a CCW winding order
/// </summary>
/// <param name="mesh">Mesh containing the vertices that the poly is indexed from<\param>
/// <param name="poly"></param>
/// <param name="indexArray"></param>
/// <param name="returnCode">Return code from the function</param>
/// <returns></returns>
StardustErrorCode _post_TriangulatePolygonEC(StardustMesh* mesh, Polygon* poly, uint32_t* indexArray, uint32_t* polygonCount);

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
/// <return>Returns the error code</return>
StardustErrorCode _post_RecomputeIndexArray(StardustMesh* mesh);

/// <summary>
/// Compares the position of a NormalPosition object to a Vertex object.
/// </summary>
/// <param name="norm">Pointer to a normal position object</param>
/// <param name="vertex">Pointer to a Vertex object</param>
/// <returns>1 for a match. Otherwise 0</returns>
int _post_ComarePositions(NormalPosition* norm, Vertex* vertex);

/// <summary>
/// Indicates wheteher a vertex is convex.
/// Takes a triangle of the adjacent vertices to "vertex", (Vprev, Vnext) and the normal of the polygon
/// Proceeds to check that the vertex winding faces the normal direction.
/// Assumes a CCW winding order
/// Credit for Idea goes to https://github.com/NMO13/earclipper/blob/master/EarClipperLib/Misc.cs#L13
/// </summary>
/// <param name="vertex">Vertex to evalutate</param>
/// <param name="Vprev">Previous vertex in the polygon</param>
/// <param name="Vnext">Next vertex in the polygon</param>
/// <param name="normal">Polygon normal</param>
/// <returns>1 for convex, -1 for concave, 0 is an error</returns>
int _post_IsVertexConvex(Vertex* vertex, Vertex* Vprev, Vertex* Vnext, Vertex* normal);

/// <summary>
/// An algorithm for checking whether triangle (A,B,C) contains vertex P
/// Assumes a CCW winding order
/// </summary>
/// <param name="a">Point A of the triangle</param>
/// <param name="b">Point B of the traingle</param>
/// <param name="c">Point C of the triangle</param>
/// <param name="p">Point to test</param>
/// <returns>1 if the ABC contains P, otherwise returns 0</returns>
int _post_TriangleContainsPoint(Vertex* a, Vertex* b, Vertex* c, Vertex* p);

/// <summary>
/// Removes a indices at the given index from the polygon object.
/// Does not reallocate any memory. Simply overwrites the value with the values above it using memcpy.
/// Will decrease poly->vertexCount attribute
/// </summary>
/// <param name="poly">Polygon to remove an index from</param>
/// <param name="idx">Index to remove</param>
void _post_RemoveFromPolygon(Polygon* poly, uint32_t idx);

/// <summary>
/// Calculates the normal of a projected polygon.
/// This is, I think, called Newell's method.
/// Credit for implementation goes to -> https://github.com/NMO13/earclipper/blob/master/EarClipperLib/EarClipping.cs#L49
/// </summary>
/// <param name="mesh"></param>
/// <param name="poly"></param>
void _post_CalculatePolygonNormal(StardustMesh* mesh, Polygon* poly);


#endif