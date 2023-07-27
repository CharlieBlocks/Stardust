#include "stardust.h"

#include <stdio.h>

int main(int argc, char* argv[])
{
	StardustErrorCode res;
	StardustMesh* meshes = 0;
	//StardustMesh* mesh = sd_LoadMesh("D:\\Programs\\C\\Stardust\\Tests\\cube.obj", 0, &res);
	size_t meshCount = sd_LoadMesh("D:\\Programs\\C\\Stardust\\BlenderAssets\\nonTrigCube.obj", STARDUST_MESH_TRIANGULATE | STARDUST_MESH_SMOOTH_NORMALS, &res, &meshes);
	//size_t meshCount = sd_LoadMesh("D:\\Programs\\C\\Stardust\\BlenderAssets\\smoothCube.obj", 0, &res, &meshes);
	//size_t meshCount = sd_LoadMesh("D:\\Programs\\C\\Stardust\\BlenderAssets\\cube.obj", STARDUST_MESH_TRIANGULATE, &res, &meshes);

	printf("Vertex Stride: %i\n", meshes->vertexStride);
	printf("Vertex Count: %i\n", meshes->vertexCount);
	printf("Index Count: %i\n", meshes->indexCount);

	//Print all vertices
	/*for (uint32_t i = 0; i < meshes->vertexCount; i++)
	{
		sd_PrintVertex(&(meshes->vertices[i]));
	}*/

	//Print triangles
	for (uint32_t i = 0; i < meshes->indexCount; i += meshes->vertexStride)
	{
		printf("Triangle %i\n", (int)(i / meshes->vertexStride));
		printf("Indices -> %i, %i, %i\n", meshes->indices[i], meshes->indices[i + 1], meshes->indices[i + 2]);
		sd_PrintVertex(&meshes->vertices[meshes->indices[i]]);
		sd_PrintVertex(&meshes->vertices[meshes->indices[i+1]]);
		sd_PrintVertex(&meshes->vertices[meshes->indices[i+2]]);
		printf("%s\n", "");
	}

	for (size_t i = 0; i < meshCount; i++)
		sd_FreeMesh(meshes);

	return 0;
}