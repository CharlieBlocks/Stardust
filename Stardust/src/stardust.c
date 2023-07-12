#include "stardust.h"
#include "filetools.h"

#include <string.h>
#include <stdlib.h>

#include <stdio.h>

#include <assert.h>
#include <crtdbg.h>

#include "loaders/OBJLoader.h"

size_t sd_LoadMesh(const char* filename, StardustMeshFlags flags, StardustErrorCode* result, StardustMesh** meshes)
{
	//Check that file exists
	if (sd_FileExists(filename) == 0)
	{
		*result = STARDUST_ERROR_FILE_NOT_FOUND;
		return 0;
	}

	//Extract filename
	char* filenameBuffer = malloc(sizeof(char) * (strlen(filename)+1));
	if (filenameBuffer == 0)
		return 0;
	strcpy_s(filenameBuffer, strlen(filename) + 1, filename);

	size_t stringElem;
	char** splitString = sd_SplitString(filenameBuffer, '.', &stringElem);

	if (splitString == 0)
		return 0;


	char* ext = splitString[stringElem - 1];
	size_t meshCount;
	StardustErrorCode loaderRet;
	if (strcmp(ext, "obj") == 0)
	{
		loaderRet = _obj_LoadMesh(filename, flags, meshes, &meshCount);
	}
	else
	{
		*result = STARDUST_ERROR_FORMAT_NOT_SUPPORTED;
		return 0;
	}


	if (splitString != 0)
		sd_FreeStringArray(splitString, stringElem);
	free(filenameBuffer);

	*result = STARDUST_ERROR_SUCCESS;

	return meshCount;
}

STARDUST_FUNC void sd_FreeMesh(StardustMesh* mesh)
{
	free(mesh->vertices);
	free(mesh->indices);
	free(mesh);
}

STARDUST_FUNC int sd_isFormatSupported(const char* format)
{
	if (format == "obj")
		return 1; //Format supported

	return 0; //F
}

STARDUST_FUNC void sd_PrintVertex(Vertex* v)
{
	printf("Vertex Pos: (%f, %f, %f, %f), Vertex UVW: (%f, %f, %f), Vertex Norm (%f, %f, %f)\n", v->x, v->y, v->z, v->w, v->texU, v->texV, v->texW, v->normX, v->normY, v->normZ);
}
