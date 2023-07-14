#include "stardust.h"

int main(int argc, char* argv[])
{
	StardustErrorCode res;
	StardustMesh* meshes = 0;
	//StardustMesh* mesh = sd_LoadMesh("D:\\Programs\\C\\Stardust\\Tests\\cube.obj", 0, &res);
	size_t meshCount = sd_LoadMesh("D:\\Programs\\C\\Stardust\\Tests\\cube.obj", STARDUST_MESH_SMOOTH_NORMALS, &res, &meshes);

	for (uint32_t i = 0; i < meshes->vertexCount; i++)
	{
		sd_PrintVertex(&(meshes->vertices[i]));
	}

	for (size_t i = 0; i < meshCount; i++)
		sd_FreeMesh(meshes);

	return 0;
}