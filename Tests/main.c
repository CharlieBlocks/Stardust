#include "stardust.h"

#include <stdio.h>

int main(int argc, char* argv[])
{
	StardustMesh* meshes = 0;
	size_t meshCount = 0;
	StardustErrorCode res = sd_LoadMesh("D:\\Programs\\C\\Stardust\\Tests\\tests\\resources\\quadCube.obj", STARDUST_MESH_TRIANGULATE, &meshes, &meshCount);
	//StardustErrorCode res = sd_LoadMesh("D:\\Programs\\C\\Stardust\\BlenderAssets\\smoothCube.obj", STARDUST_MESH_HARDEN_NORMALS, &meshes, &meshCount);
	//StardustErrorCode res = sd_LoadMesh("D:\\Programs\\C\\Stardust\\BlenderAssets\\cube.obj", STARDUST_MESH_SMOOTH_NORMALS, &meshes, &meshCount);

	printf("Vertex Stride: %i\n", meshes->vertexStride);
	printf("Vertex Count: %i\n", meshes->vertexCount);
	printf("Index Count: %i\n", meshes->indexCount);

	//Print polygons
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
		sd_FreeMesh(&meshes[i]);

	return 0;
}