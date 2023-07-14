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



// ================= Harden Normals ================= //

void _post_HardenNormals(StardustMesh* mesh)
{
}


// ================= Utils ================= //

int _post_ComarePositions(NormalPosition* norm, Vertex* vertex)
{
	return norm->x == vertex->x &&
		norm->y == vertex->y &&
		norm->z == vertex->z &&
		norm->w == vertex->w;
}