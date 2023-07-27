#include "postprocessing.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <stdio.h>

StardustErrorCode _post_PerformPostProcessing(StardustMesh* mesh, StardustMeshFlags flags)
{
	StardustErrorCode ret;

	//Switch on flags

	//Triangulation of meshes
	if ((flags & STARDUST_MESH_TRIANGULATE) == STARDUST_MESH_TRIANGULATE)
	{
		ret = _post_TriangulateMeshEC(mesh);
		if (ret != STARDUST_ERROR_SUCCESS) { return ret; }
	}

	//Normal Generation / Normal Hardening
	if ((flags & STARDUST_MESH_GENERATE_NORMALS) == STARDUST_MESH_GENERATE_NORMALS)
	{
		ret = _post_GenerateNormals(mesh);
		if (ret != STARDUST_ERROR_SUCCESS) { return ret; }
	}

	//Normal Smoothing
	if ((flags & STARDUST_MESH_SMOOTH_NORMALS) == STARDUST_MESH_SMOOTH_NORMALS && (mesh->dataType & STARDUST_SMOOTHSHADING) != STARDUST_SMOOTHSHADING)
	{
		if ((mesh->dataType & STARDUST_NORMAL_DATA) != STARDUST_NORMAL_DATA) //Generate normal data if need be
		{
			ret = _post_GenerateNormals(mesh);
			if (ret != STARDUST_ERROR_SUCCESS) { return ret; }
		}

		ret = _post_SmoothNormals(mesh); //Smooth normals if flag is present and mesh does not contain smooth normals
		if (ret != STARDUST_ERROR_SUCCESS) { return ret; }
	}

	return STARDUST_ERROR_SUCCESS;
}

// ================= Smooth Normals ================= //

StardustErrorCode _post_SmoothNormals(StardustMesh* mesh)
{
	// Create index list of simillar position vertices //
	
	uint32_t bucketCount = 0;
	
	//Set buckets for wost case scenario
	NormalPosition** normalBuckets = malloc(mesh->vertexCount * sizeof(NormalPosition*)); //Assume that every vertex is different
	if (normalBuckets == 0) { return STARDUST_ERROR_MEMORY_ERROR; }
	uint32_t* bucketSizes = malloc(mesh->vertexCount * sizeof(uint32_t));
	if (bucketSizes == 0) { free(normalBuckets); return STARDUST_ERROR_MEMORY_ERROR; }

	memset(normalBuckets, 0, mesh->vertexCount * sizeof(Vertex*));
	memset(bucketSizes, 0, mesh->vertexCount * sizeof(uint32_t));

	for (uint32_t i = 0; i < mesh->vertexCount; i++)
	{
		//Iterate through buckets
		uint32_t j = 0;
		for (; j < bucketCount; j++)
		{
			if (_post_ComarePositions((normalBuckets[j]), &(mesh->vertices[i])))
				break;
		}
		
		//Allocate new bucket
		NormalPosition* newBucket = malloc(((size_t)bucketSizes[j] + 1) * sizeof(NormalPosition));
		if (newBucket == 0)
		{
			for (uint32_t k = 0; k < bucketCount; k++)
				free(normalBuckets[k]);
			free(normalBuckets);
			free(bucketSizes);
			return STARDUST_ERROR_MEMORY_ERROR;
		}

		//Copy in old data
		memcpy(newBucket, normalBuckets[j], (size_t)bucketSizes[j] * sizeof(NormalPosition));

		//Add new vertex to bucket
		newBucket[bucketSizes[j]].x = mesh->vertices[i].x;
		newBucket[bucketSizes[j]].y = mesh->vertices[i].y;
		newBucket[bucketSizes[j]].z = mesh->vertices[i].z;
		newBucket[bucketSizes[j]].w = mesh->vertices[i].w;
		newBucket[bucketSizes[j]].index = i;

		//Free old bucket
		if (normalBuckets[j] != 0)
			free(normalBuckets[j]);
		else
			bucketCount++; //Only increment when adding a new bucket. Which would not replace an old one

		//Set new pointer
		normalBuckets[j] = newBucket;

		bucketSizes[j]++;
	}

	// Average normals
	for (uint32_t i = 0; i < bucketCount; i++)
	{
		//Acumulate values
		float totalX = 0, totalY = 0, totalZ = 0;
		for (uint32_t j = 0; j < bucketSizes[i]; j++)
		{
			totalX += mesh->vertices[normalBuckets[i][j].index].normX;
			totalY += mesh->vertices[normalBuckets[i][j].index].normY;
			totalZ += mesh->vertices[normalBuckets[i][j].index].normZ;
		}

		//Calculate magnitude
		float mag = sqrtf(totalX * totalX + totalY * totalY + totalZ * totalZ);

		//Divide by total
		totalX /= mag;
		totalY /= mag;
		totalZ /= mag;

		//Set normals of all vertices
		for (uint32_t j = 0; j < bucketSizes[i]; j++)
		{
			mesh->vertices[normalBuckets[i][j].index].normX = totalX;
			mesh->vertices[normalBuckets[i][j].index].normY = totalY;
			mesh->vertices[normalBuckets[i][j].index].normZ = totalZ;
		}
	}

	mesh->dataType |= STARDUST_SMOOTHSHADING;

	free(bucketSizes);
	for (uint32_t i = 0; i < bucketCount; i++)
		free(normalBuckets[i]);
	free(normalBuckets);

	_post_RecomputeIndexArray(mesh);

	return STARDUST_ERROR_SUCCESS;
}





// ================= Generate Normals ================= //

StardustErrorCode _post_GenerateNormals(StardustMesh* mesh)
{
	//Check that mesh is triangulated
	if (mesh->vertexStride != 3)
	{
		StardustErrorCode ret = _post_TriangulateMeshEC(mesh); //If not, triangulate
		if (ret != STARDUST_ERROR_SUCCESS) { return ret; }
	}

	//Loop by face.
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;

	Vertex* vertexArray = malloc(mesh->indexCount * mesh->vertexStride * sizeof(Vertex));
	if (vertexArray == 0) { return STARDUST_ERROR_MEMORY_ERROR; }
	uint32_t* indexArray = malloc(mesh->indexCount * mesh->vertexStride * sizeof(uint32_t));
	if (indexArray == 0) { free(vertexArray);  return STARDUST_ERROR_MEMORY_ERROR; }

	
	for (uint32_t i = 0; i < mesh->indexCount; i += mesh->vertexStride)
	{
		// a == 1
		// b == 2
		// c == 3
		// norm(cross(b - a, c - a))

		uint32_t a = mesh->indices[i];
		uint32_t b = mesh->indices[i + 1];
		uint32_t c = mesh->indices[i + 2];

		//Edge AB
		float edgeAx = mesh->vertices[b].x - mesh->vertices[a].x;
		float edgeAy = mesh->vertices[b].y - mesh->vertices[a].y;
		float edgeAz = mesh->vertices[b].z - mesh->vertices[a].z;

		//Edge AC
		float edgeBx = mesh->vertices[c].x - mesh->vertices[a].x;
		float edgeBy = mesh->vertices[c].y - mesh->vertices[a].y;
		float edgeBz = mesh->vertices[c].z - mesh->vertices[a].z;

		//Cross product
		float normX = edgeAy*edgeBz - edgeAz*edgeBy;
		float normY = edgeAz*edgeBx - edgeAx*edgeBz;
		float normZ = edgeAx*edgeBy - edgeAy*edgeBx;

		//Mag
		float mag = sqrtf(normX*normX + normY*normY + normZ*normZ);
		normX /= mag;
		normY /= mag;
		normZ /= mag;

		//Create vertices
		for (uint32_t j = i; j < i + 3; j++)
		{
			uint32_t idx = mesh->indices[j];

			vertexArray[j].x = mesh->vertices[idx].x;
			vertexArray[j].y = mesh->vertices[idx].y;
			vertexArray[j].z = mesh->vertices[idx].z;
			vertexArray[j].w = mesh->vertices[idx].w;

			vertexArray[j].texU = mesh->vertices[idx].texU;
			vertexArray[j].texV = mesh->vertices[idx].texV;
			vertexArray[j].texW = mesh->vertices[idx].texW;

			vertexArray[j].normX = normX;
			vertexArray[j].normY = normY;
			vertexArray[j].normZ = normZ;

			indexArray[j] = j;
		}

		vertexCount += mesh->vertexStride;
		indexCount += mesh->vertexStride;
	}

	//Free old data
	free(mesh->vertices);
	free(mesh->indices);

	//Set new data
	mesh->vertices = vertexArray;
	mesh->indices = indexArray;

	mesh->vertexCount = vertexCount;
	mesh->indexCount = indexCount;

	mesh->dataType &= ~STARDUST_SMOOTHSHADING;

	//Shrink vertices
	_post_RecomputeIndexArray(mesh);

	return STARDUST_ERROR_SUCCESS;
}


// ================= Triangulation ================= //

StardustErrorCode _post_TriangulateMeshEC(StardustMesh* mesh)
{
	//Check that mesh isn't already triangulated
	if (mesh->vertexStride == 3)
		return STARDUST_ERROR_SUCCESS;

	//Precalculate polygon count
	uint32_t polygonCount = mesh->indexCount / mesh->vertexStride; //IndexCount / number of indices per polygon

	//Allocate polygon array
	Polygon* polygons = malloc(sizeof(Polygon) * polygonCount);
	if (polygons == 0) { return STARDUST_ERROR_MEMORY_ERROR; }

	//Fill polygons
	for (uint32_t i = 0; i < polygonCount; i++)
	{
		uint32_t index = i * mesh->vertexStride;
		polygons[i].indices = malloc(sizeof(uint32_t) * mesh->vertexStride); //WHAT?
		if (polygons[i].indices == 0)
		{
			for (uint32_t j = 0; j < i; i++)
				free(polygons[j].indices);
			free(polygons);
			return STARDUST_ERROR_MEMORY_ERROR;
		}

		for (uint32_t j = 0; j < mesh->vertexStride; j++)
			polygons[i].indices[j] = mesh->indices[mesh->indices[index + j]]; //WHAT?
		polygons[i].vertexCount = mesh->vertexStride;

		_post_CalculatePolygonNormal(mesh, &polygons[i]);
	}


	//Allocate new index array
	uint32_t* newIndices = malloc(sizeof(uint32_t) * 3 * (mesh->indexCount - 2));
	if (newIndices == 0)
	{
		for (uint32_t i = 0; i < polygonCount; i++)
		{
			if (polygons[i].indices != 0)
				free(polygons[i].indices);
		}
		free(polygons);

		return STARDUST_ERROR_MEMORY_ERROR;
	}

	//Iterate over polygons and triangulate them
	StardustErrorCode res; //Return code
	uint32_t count = 0; //Number of polygons triangulated from fuctiion
	uint32_t newIndexCount = 0; //Position in newIndices
	for (uint32_t i = 0; i < polygonCount; i++)
	{
		res = _post_TriangulatePolygonEC(mesh, &polygons[i], newIndices + newIndexCount, &count);
		if (res != STARDUST_ERROR_SUCCESS) //Validate success
		{
			for (uint32_t i = 0; i < polygonCount; i++)
			{
				if (polygons[i].indices != 0)
					free(polygons[i].indices);
			}
			free(polygons);
			free(newIndices);
			return res;
		}

		newIndexCount += count;
	}

	//Set mesh to new indices
	free(mesh->indices);
	mesh->indices = newIndices;
	mesh->vertexStride = 3;
	mesh->indexCount = newIndexCount;

	for (uint32_t i = 0; i < polygonCount; i++)
	{
		if (polygons[i].indices != 0)
			free(polygons[i].indices);
	}
	free(polygons);

	return STARDUST_ERROR_SUCCESS;
}

StardustErrorCode _post_TriangulatePolygonEC(StardustMesh* mesh, Polygon* poly, uint32_t* indexArray, uint32_t* polygonCount)
{
	//Preallocate arrays
	uint32_t* convexIndices = malloc(sizeof(uint32_t) * mesh->vertexStride); //Allocate for maximum scenarios
	if (convexIndices == 0) { return STARDUST_ERROR_MEMORY_ERROR; }
	uint32_t* concaveIndices = malloc(sizeof(uint32_t) * mesh->vertexStride); 
	if (concaveIndices == 0) { free(convexIndices); return STARDUST_ERROR_MEMORY_ERROR; }


	//Get convex indices
	uint32_t convexCount = _post_FillConvexConcaveVertices(mesh, poly, convexIndices, concaveIndices);
	uint32_t concaveCount = mesh->vertexStride - convexCount;

	//Preallocate ears
	uint32_t* earIndices = malloc(sizeof(uint32_t) * mesh->vertexStride);
	if (earIndices == 0) 
	{ 
		free(convexIndices); free(concaveIndices);
		return STARDUST_ERROR_MEMORY_ERROR;
	}

	//Get ears
	uint32_t earCount = _post_FillEars(mesh, poly, convexIndices, convexCount, concaveIndices, concaveCount, earIndices);

	//Loop over ears
	uint32_t indexCount = 0;
	while (earCount > 0)
	{
		//	Get indices
		uint32_t earIdx = earIndices[earCount - 1];
		uint32_t prevIdx = (earIdx + poly->vertexCount - 1) % poly->vertexCount;
		uint32_t nextIdx = (earIdx + 1) % poly->vertexCount;

		//	Add to indexArray
		indexArray[indexCount] = poly->indices[prevIdx];
		indexArray[indexCount + 1] = poly->indices[earIdx];
		indexArray[indexCount + 2] = poly->indices[nextIdx];
		indexCount += 3;

		//Remove from polygon
		_post_RemoveFromPolygon(poly, earIdx);

		//	Recalculate convex indices
		convexCount = _post_FillConvexConcaveVertices(mesh, poly, convexIndices, concaveIndices);
		concaveCount = mesh->vertexStride - concaveCount;

		//	Recalculate ears
		earCount = _post_FillEars(mesh, poly, convexIndices, convexCount, concaveIndices, concaveCount, earIndices);
	}

	free(convexIndices);
	free(concaveIndices);
	free(earIndices);

	*polygonCount = indexCount;

	return STARDUST_ERROR_SUCCESS;
}

uint32_t _post_FillConvexConcaveVertices(StardustMesh* mesh, Polygon* poly, uint32_t* convexIndices, uint32_t* concaveIndices)
{
	//Counters
	uint32_t vexIdx = 0; //Convex idx
	uint32_t aveIdx = 0; //Concave idx

	//Iterate over indices
	for (uint32_t i = 0; i < poly->vertexCount; i++)
	{
		//Get previous and next vertices for triangle
		uint32_t prevIdx = (i + poly->vertexCount - 1) % poly->vertexCount; //I'm pretty sure C doesn't auto abs the result of a modulo op like python does.
		uint32_t nextIdx = (i + 1) % poly->vertexCount;

		//Check if vertex is convex. Indiex into vertex array by using mesh->indices
		if (_post_IsVertexConvex(&mesh->vertices[poly->indices[i]], &mesh->vertices[poly->indices[prevIdx]], &mesh->vertices[poly->indices[nextIdx]], &poly->normal) == 1)
		{
			convexIndices[vexIdx] = i; //Add vertex as convexVertex
			vexIdx++; //Increment Counter
		}
		else
		{
			concaveIndices[aveIdx] = i; //Add vertex as concaveVertex
			aveIdx++; //Increment Counter
		}
	}

	return vexIdx; //Only return a vexIdx because aveIdx can be calculated using mesh->vertexCount - vexIdx
}

uint32_t _post_FillEars(StardustMesh* mesh, Polygon* poly, uint32_t* convexIndices, uint32_t convexCount, uint32_t* concaveIndices, uint32_t concaveCount, uint32_t* earArray)
{
	uint32_t earIdx = 0;

	for (uint32_t i = 0; i < convexCount; i++)
	{
		//Get adjacent indices
		uint32_t currIdx = poly->indices[convexCount];
		uint32_t prevIdx = (currIdx + mesh->vertexStride - 1) % mesh->vertexStride;
		uint32_t nextIdx = (currIdx + 1) % mesh->vertexStride;

		int isEar = 1;
		for (uint32_t j = 0; j < concaveCount; j++)
		{
			if (j == prevIdx || j == nextIdx)
				continue;

			if (_post_TriangleContainsPoint(&mesh->vertices[prevIdx], &mesh->vertices[currIdx], &mesh->vertices[nextIdx], &mesh->vertices[j]))
				isEar = 0;
		}

		if (isEar)
		{
			earArray[earIdx] = i;
			earIdx++;
		}
	}

	return earIdx;
}

// ================= Utils ================= //


StardustErrorCode _post_RecomputeIndexArray(StardustMesh* mesh)
{
	uint32_t vertexCount = 0;

	Vertex* vertexArray = malloc(mesh->vertexCount * sizeof(Vertex)); //Assign max size arrays. reduce later
	if (vertexArray == 0) { return STARDUST_ERROR_MEMORY_ERROR; }
	uint32_t* indexArray = malloc(mesh->indexCount * sizeof(uint32_t));
	if (indexArray == 0) { free(vertexArray);  return STARDUST_ERROR_MEMORY_ERROR; }

	for (uint32_t i = 0; i < mesh->indexCount; i++)
	{
		//Get vertex pointer
		Vertex* meshVertex = &mesh->vertices[mesh->indices[i]];

		//Check if vertex exists within vertexArray
		int found = 0;
		for (uint32_t j = 0; j < vertexCount; j++)
		{
			if (sd_CompareVertex(meshVertex, &(vertexArray[j])))
			{
				//Add indice to indexArray
				indexArray[i] = j;

				found = 1;
				break;
			}
		}

		if (!found)
		{
			//Add vertex to array
			vertexArray[vertexCount] = *meshVertex; //Copy instruction

			//Add index to array
			indexArray[i] = vertexCount;

			//Increment arrays
			vertexCount++;
		}
	}

	free(mesh->vertices);
	free(mesh->indices);

	mesh->vertices = vertexArray;
	mesh->indices = indexArray;

	mesh->vertexCount = vertexCount;

	return STARDUST_ERROR_MEMORY_ERROR;
}

int _post_ComarePositions(NormalPosition* norm, Vertex* vertex)
{
	return norm->x == vertex->x &&
		norm->y == vertex->y &&
		norm->z == vertex->z &&
		norm->w == vertex->w;
}

int _post_IsVertexConvex(Vertex* vertex, Vertex* vPrev, Vertex* vNext, Vertex* normal)
{
	//Get edges
	float E1x = vPrev->x - vertex->x;
	float E1y = vPrev->y - vertex->y;
	float E1z = vPrev->z - vertex->z;
	
	float E2x = vNext->x - vertex->x;
	float E2y = vNext->y - vertex->y;
	float E2z = vNext->z - vertex->z;

	//Cross product
	float crossX = (E1y * E2z) - (E1z * E2y);
	float crossY = (E1z * E2x) - (E1x * E2z);
	float crossZ = (E1x * E2y) - (E1y * E2x);

	//Length squared
	float l2 = crossX*crossX + crossY*crossY + crossZ*crossZ;

	if (l2 == 0) //Early exit
		return 0;

	//Get signs
	int xSign = signbit(crossX);
	int ySign = signbit(crossY);
	int zSign = signbit(crossZ);

	int xSignNormal = signbit(normal->x);
	int ySignNormal = signbit(normal->y);
	int zSignNormal = signbit(normal->z);

	if (xSign != xSignNormal || ySign != ySignNormal || zSign != zSignNormal) //Return -1 if concave
		return 1;
	return -1; //Return 1 if convex

}

//Came from some stack overflow post. Can't remember which one. 
int _post_TriangleContainsPoint(Vertex* a, Vertex* b, Vertex* c, Vertex* p)
{
	float A = 0.5f * (b->y * c->x + a->y * (-b->x + c->x) + a->x * (b->y - c->y) + b->x * c->y);
	float sign = A > 0.0f ? 1.0f : -1.0f;
	float s = (a->y * c->x - a->x * c->y + (c->y - a->y) * p->x + (a->x - c->x) * p->y) * sign;
	float t = (a->x * b->y - a->y * b->x + (a->y - b->y) * p->x + (b->x - a->x) * p->y) * sign;

	if (s > 0 && t > 0 && (s + t) < 2 * A * sign)
		return 1;
	return 0;
}

void _post_RemoveFromPolygon(Polygon* poly, uint32_t idx)
{
	if (idx == poly->vertexCount - 1)
	{
		poly->vertexCount -= 1;
		return;
	}

	memcpy(poly + idx, poly + idx + 1, (poly->vertexCount - 1) - idx);

	poly->vertexCount -= 1;
}

void _post_CalculatePolygonNormal(StardustMesh* mesh, Polygon* poly)
{
	//Zero poly normal
	memset(&poly->normal, 0, sizeof(Vertex));

	for (uint32_t i = 0; i < poly->vertexCount; i++)
	{
		uint32_t j = (i + 1) % poly->vertexCount;
		Vertex* vertA = &mesh->vertices[poly->indices[i]];
		Vertex* vertB = &mesh->vertices[poly->indices[j]];

		poly->normal.x += (vertA->y - vertB->y) * (vertA->z + vertB->z);
		poly->normal.y += (vertA->z - vertB->z) * (vertA->x + vertB->x);
		poly->normal.z += (vertA->x - vertB->x) * (vertA->y + vertB->y);
	}
}
