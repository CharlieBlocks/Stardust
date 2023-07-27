#include "stardust.h"
#include "filetools.h"
#include "postprocessing.h"

//Loaders
#include "loaders/OBJLoader.h"

//STD
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


StardustErrorCode sd_LoadMesh(const char* filename, StardustMeshFlags flags, StardustMesh** meshes, size_t* meshCount)
{
	StardustErrorCode ret;

	//Check that file exists
	if (sd_FileExists(filename) == 0)
		return STARDUST_ERROR_FILE_NOT_FOUND;

	//Extract filename
	char* filenameBuffer = malloc(sizeof(char) * (strlen(filename)+1));
	if (filenameBuffer == 0)
		return 0;
	strcpy_s(filenameBuffer, strlen(filename) + 1, filename);

	size_t stringElem;
	char** splitString = sd_SplitString(filenameBuffer, '.', &stringElem);

	if (splitString == 0)
		return STARDUST_ERROR_FILE_NOT_FOUND;


	char* ext = splitString[stringElem - 1];
	if (strcmp(ext, "obj") == 0)
	{
		ret = _obj_LoadMesh(filename, flags, meshes, meshCount);
	}
	else
		return STARDUST_ERROR_FORMAT_NOT_SUPPORTED;

	if (ret != STARDUST_ERROR_SUCCESS)
	{
		sd_FreeStringArray(splitString, stringElem);
		free(filenameBuffer);

		return ret;
	}

	sd_FreeStringArray(splitString, stringElem);
	free(filenameBuffer);
	

	//Perform post processing
	for (size_t i = 0; i < *meshCount; i++)
	{
		ret = _post_PerformPostProcessing(meshes[i], flags);
		if (ret != STARDUST_ERROR_SUCCESS)
		{
			for (size_t j = 0; j < *meshCount; j++)
				sd_FreeMesh(meshes[j]);
			return ret;
		}
	}


	return STARDUST_ERROR_SUCCESS;
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

STARDUST_FUNC int sd_CompareVertexPosition(Vertex* a, Vertex* b)
{
	return a->x == b->x &&
		a->y == b->y &&
		a->z == b->z &&
		a->w == a->w;
}

STARDUST_FUNC int sd_CompareVertex(Vertex* a, Vertex* b)
{
	return a->x == b->x &&
		a->y == b->y &&
		a->z == b->z &&
		a->w == b->w &&

		a->texU == b->texU &&
		a->texV == b->texV &&
		a->texW == b->texW &&

		a->normX == b->normX &&
		a->normY == b->normY &&
		a->normZ == b->normZ;

}

