#include "postprocessing.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include <stdio.h>
#include <crtdbg.h>

void _post_PerformPostProcessing(StardustMesh* mesh, StardustMeshFlags flags)
{
	//Switch on flags
	if ((flags & STARDUST_MESH_SMOOTH_NORMALS) == STARDUST_MESH_SMOOTH_NORMALS)
		_post_SmoothNormals(mesh);
	if ((flags & STARDUST_MESH_HARDEN_NORMALS) == STARDUST_MESH_HARDEN_NORMALS)
		_post_HardenNormals(mesh);
}


// ================= Smooth Normals ================= //

void _post_SmoothNormals(StardustMesh* mesh)
{
	// Create index list of simillar position vertices //
	
	uint32_t bucketCount = 0;
	
	//Set buckets for wost case scenario
	NormalPosition** normalBuckets = malloc(mesh->vertexCount * sizeof(NormalPosition*)); //Assume that every vertex is different
	uint32_t* bucketSizes = malloc(mesh->vertexCount * sizeof(uint32_t));

	assert(normalBuckets != 0);
	assert(bucketSizes != 0);

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
		assert(newBucket != 0); //Assert allocation

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

	free(bucketSizes);
	for (uint32_t i = 0; i < bucketCount; i++)
		free(normalBuckets[i]);
	free(normalBuckets);

	_post_RecomputeIndexArray(mesh);
}





// ================= Harden Normals ================= //

void _post_HardenNormals(StardustMesh* mesh)
{
	//Loop by face.
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;

	Vertex* vertexArray = malloc(mesh->indexCount * mesh->vertexStride * sizeof(Vertex));
	uint32_t* indexArray = malloc(mesh->indexCount * mesh->vertexStride * sizeof(uint32_t));

	assert(vertexArray != 0);
	assert(indexArray != 0);

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
			vertexArray[j].y = mesh->vertices[idx].x;
			vertexArray[j].z = mesh->vertices[idx].x;
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

	//Shrink vertices
	_post_RecomputeIndexArray(mesh);
}


// ================= Utils ================= //

void _post_RecomputeIndexArray(StardustMesh* mesh)
{
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;

	Vertex* vertexArray = malloc(mesh->vertexCount * sizeof(Vertex)); //Assign max size arrays. reduce later
	uint32_t* indexArray = malloc(mesh->vertexCount * sizeof(uint32_t));

	assert(vertexArray != 0);
	assert(indexArray != 0);

	for (uint32_t i = 0; i < mesh->vertexCount; i++)
	{
		//Get vertex pointer
		Vertex* meshVertex = &mesh->vertices[i];

		//Check if vertex exists within vertexArray
		int found = 0;
		for (uint32_t j = 0; j < vertexCount; j++)
		{
			if (sd_CompareVertex(meshVertex, &(vertexArray[j])))
			{
				//Add indice to indexArray
				indexArray[indexCount] = j;
				indexCount++;

				found = 1;
				break;
			}
		}

		if (!found)
		{
			//Add vertex to array
			vertexArray[vertexCount] = *meshVertex; //Copy instruction

			assert(_CrtCheckMemory());

			//Add index to array
			indexArray[indexCount] = vertexCount;

			assert(_CrtCheckMemory());

			//Increment arrays
			indexCount++;
			vertexCount++;
		}
	}

	free(mesh->vertices);
	free(mesh->indices);

	mesh->vertices = vertexArray;
	mesh->indices = indexArray;

	mesh->vertexCount = vertexCount;
	mesh->indexCount = indexCount;
}

int _post_ComarePositions(NormalPosition* norm, Vertex* vertex)
{
	return norm->x == vertex->x &&
		norm->y == vertex->y &&
		norm->z == vertex->z &&
		norm->w == vertex->w;
}