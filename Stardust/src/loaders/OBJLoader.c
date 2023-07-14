#include "OBJLoader.h"

#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <math.h>

#include <assert.h>
#include <crtdbg.h>

#include "../filetools.h"


StardustErrorCode _obj_LoadMesh(const char* filename, StardustMeshFlags flags, StardustMesh** meshes, size_t* meshCount)
{
	StardustErrorCode result;

	// ---------------- Open File ---------------- //
	FILE* fileHandle;
	errno_t fileRet = fopen_s(&fileHandle, filename, "r");

	//Check that the file was opened
	if (fileRet != 0)
		return STARDUST_ERROR_IO_ERROR;

	
	// ---------------- Find OBJ tags and tag counts ---------------- //
	size_t objectCount;
	OBJObject* objects = _obj_GetObjects(fileHandle, &result, &objectCount);

	//Early exit if zero objects are returned

	if (result != STARDUST_ERROR_SUCCESS)
	{
		if (objects != 0)
			_obj_FreeObjects(objects, objectCount);
		fclose(fileHandle);
		return result;
	}

	_obj_RemoveTags(objects, objectCount, flags);

	// ---------------- Allocate arrays ---------------- //
	for (int i = 0; i < objectCount; i++)
		_obj_AllocateTagArrays(objects[i].tags);

	// ---------------- Fill arrays ---------------- //
	fseek(fileHandle, 0, 0); //Reset filePtr to the begining
	result = _obj_FillObjectData(fileHandle, objects, objectCount);

	if (result != STARDUST_ERROR_SUCCESS) //Error checking
	{
		_obj_FreeObjects(objects, objectCount);
		fclose(fileHandle);
		return result;
	}

	// ---------------- Compile Meshes ---------------- //
	*meshes = malloc(sizeof(StardustMesh) * objectCount);
	assert(*meshes != 0);
	memset(*meshes, 0, sizeof(StardustMesh) * objectCount);

	result = _obj_FillMeshes(*meshes, objects, objectCount);

	*meshCount = objectCount;

	// ---------------- Delete memory ---------------- //

	_obj_FreeObjects(objects, objectCount);

	fclose(fileHandle);
	return result;
}

OBJObject* _obj_GetObjects(FILE* file, StardustErrorCode* result, size_t* objectCount)
{
	// Predefine Variables //
	char buffer[MAX_LINE_BUFFER_SIZE]; //Buffer

	size_t currentObjectCount = 0; //Counters + array
	OBJObject* objects = 0;

	int currentObject = -1; //Current object (not really needed but it cleans things up)

	while (fgets(buffer, sizeof(buffer), file)) //Get until \n or character limit
	{
		//Check that we didn't hit the character limit
		if (buffer[strlen(buffer) - 1] != '\n')
		{
			//Throw overflow error
			*result = STARDUST_ERROR_LINE_EXCCEDS_BUFFER;
			
			//Check for cleanup
			//if (currentObjectCount > 0)
				//_obj_FreeObjects(objects, currentObjectCount);

			return 0;
		}

		//Split line by spaces
		size_t splitCount;
		char** splitLine = sd_SplitString(buffer, ' ', &splitCount);

		//Get prefix
		char* prefix = splitLine[0];

		//Compare prefix with tags
		if (strcmp(prefix, "o") == 0) //New Object
		{
			//Allocate larger array for object
			OBJObject* newObjects = malloc(sizeof(OBJObject) * (currentObjectCount + 1)); //Create larger array
			assert(newObjects != 0); //Assert that allocation was succesfull
			
			if (objects != 0) //Edge case on first iteration
				memcpy(newObjects, objects, sizeof(OBJObject) * currentObjectCount); //Copy in old objects
			
			// Increment counters
			currentObjectCount++;
			currentObject++;
			*objectCount = currentObjectCount;

			//Get object from new array for readability
			OBJObject* object = &newObjects[currentObject];

			//Initialise new OBJObject
			size_t nameLength = strlen(splitLine[1]) + 1; //Create name buffer
			object->name = malloc(nameLength);
			assert(object->name != 0); //Assert allocation success
			strcpy_s(object->name, nameLength, splitLine[1]); //Copy in string
			
			object->tags = malloc(sizeof(OBJTags)); //Create tags 
			assert(object->tags != 0); //Assert that allocation was succesfull

			//Clear object->tags
			memset(object->tags, 0, sizeof(OBJTags));
			
			//Swap pointers
			free(objects);
			objects = newObjects;
		}
		else if (strcmp(prefix, "v") == 0)
		{
			if (currentObject == -1) //Ensure that object was created
			{
				*result = STARDUST_ERROR_FILE_INVALID;
				return 0;
			}

			//Add tag to object
			objects[currentObject].tags->tags |= OBJTAG_VERTEX;

			//Increment counter
			objects[currentObject].tags->vertexTagCount++;


			if (splitCount < 4 || splitCount > 8) //Contains only prefix + x + y. OBJ spec forces a Z value || Contains no more than prefix + x + y + z + w + r + g + b
			{
				*result = STARDUST_ERROR_FILE_INVALID;
				return 0;
			}


			//Set elements per vertex
			if (objects[currentObject].tags->elementsPerVertex == 0) //Check that is has not been set
				objects[currentObject].tags->elementsPerVertex = (uint32_t)(splitCount - 1);
			else
			{
				//Validate that line contains correct amount of values
				if (splitCount - 1 != objects[currentObject].tags->elementsPerVertex)
				{
					*result = STARDUST_ERROR_FILE_INVALID; //Changed number of elements per vertex point mid object
					return 0;
				}
			}
		}
		else if (strcmp(prefix, "vt") == 0) //Texture coord
		{
			if (currentObject == -1) //Ensure that object was created
			{
				*result = STARDUST_ERROR_FILE_INVALID;
				return 0;
			}

			//Add tag to object
			objects[currentObject].tags->tags |= OBJTAG_TEXCOORD;

			//Increment counter
			objects[currentObject].tags->texCoordTagCount++;

			if (splitCount < 2) //No element after prefix
			{
				*result = STARDUST_ERROR_FILE_INVALID;
				return 0;
			} //Don't worry about extra elements as that won't cause an error


			//Set elements per texCoord
			if (objects[currentObject].tags->elementsPerTexCoord == 0) //Check that is has not been set
				objects[currentObject].tags->elementsPerTexCoord = (uint32_t)(splitCount - 1);
			else
			{
				//Validate that line contains correct amount of values
				if (splitCount - 1 != objects[currentObject].tags->elementsPerTexCoord)
				{
					*result = STARDUST_ERROR_FILE_INVALID; //Changed number of elements per vertex point mid object
					return 0;
				}
			}
		}
		else if (strcmp(prefix, "vn") == 0) //Normal
		{
			if (currentObject == -1) //Ensure that object was created
			{
				*result = STARDUST_ERROR_FILE_INVALID;
				return 0;
			}

			//Add tag to object
			objects[currentObject].tags->tags |= OBJTAG_NORMAL;

			//Increment counter
			objects[currentObject].tags->normalTagCount++;


			//Normal should only have 3 elements
			if (splitCount != 4)
			{
				*result = STARDUST_ERROR_FILE_INVALID;
				return 0;
			}
		}
		else if (strcmp(prefix, "f") == 0) //Face
		{
			if (currentObject == -1) //Ensure that object was created
			{
				*result = STARDUST_ERROR_FILE_INVALID;
				return 0;
			}

			//Add tag to object
			objects[currentObject].tags->tags |= OBJTAG_FACE;

			//Increment counter
			objects[currentObject].tags->faceTagCount++;
			

			//Set elements per face
			if (objects[currentObject].tags->elementsPerFace == 0) //Check that is has not been set
				objects[currentObject].tags->elementsPerFace = (uint32_t)(splitCount - 1);
			else
			{
				//Validate that line contains correct amount of values
				if (splitCount - 1 != objects[currentObject].tags->elementsPerFace)
				{
					*result = STARDUST_ERROR_FILE_INVALID; //Changed number of elements per vertex point mid object
					return 0;
				}
			}

			for (uint32_t i = 0; i < objects[currentObject].tags->elementsPerFace; i++)
			{
				size_t indiceCount;
				char** indiceSplit = sd_SplitString(splitLine[i + 1], '/', &indiceCount);

				//Check that it is within bounds
				if (indiceCount > 3)
				{
					*result = STARDUST_ERROR_FILE_INVALID;
					return 0;
				}

				if (objects[currentObject].tags->indicesPerVertex == 0) //Not set
					objects[currentObject].tags->indicesPerVertex = (uint32_t)indiceCount;
				else
				{
					if (indiceCount != objects[currentObject].tags->indicesPerVertex)
					{
						*result = STARDUST_ERROR_FILE_INVALID;
						return 0;
					}
				}
			}

		}
		else if (strcmp(prefix, "s") == 0)
		{
			if (currentObject == -1) //Ensure that object was created
			{
				*result = STARDUST_ERROR_FILE_INVALID;
				return 0;
			}

			//Label objects as smooth shaded
			objects[currentObject].tags->tags |= OBJTAG_SMOOTH;
		}
		else if (strcmp(prefix, "vp") == 0)
		{
			//NOT SUPPORTED YET
		}
		else if (strcmp(prefix, "l") == 0)
		{
			//NOT SUPPORTED YET
		}
		sd_FreeStringArray(splitLine, splitCount);
	} //while (fgets(buffer, sizeof(buffer), file))

	*result = STARDUST_ERROR_SUCCESS;

	return objects;
}

void _obj_AllocateTagArrays(OBJTags* tags)
{
	if ((tags->tags & OBJTAG_VERTEX) != 0)
	{
		// Vertices //
		tags->vertices = malloc(sizeof(float) * tags->vertexTagCount * tags->elementsPerVertex);
		assert(tags->vertices != 0);
	}

	if ((tags->tags & OBJTAG_TEXCOORD) != 0)
	{
		// Texture Coords //
		tags->texCoords = malloc(sizeof(float) * tags->texCoordTagCount * tags->elementsPerTexCoord);
		assert(tags->texCoords != 0);
	}

	if ((tags->tags & OBJTAG_NORMAL) != 0)
	{
		// Normals //
		tags->normals = malloc(sizeof(float) * tags->normalTagCount * 3); //Always have 3 components to normals
		assert(tags->normals != 0);
	}

	if ((tags->tags & OBJTAG_FACE) != 0)
	{
		// Faces //
		tags->vertexHashes = malloc(sizeof(uint32_t) * tags->faceTagCount * tags->elementsPerFace);
		assert(tags->vertexHashes != 0);

		tags->indices = malloc(sizeof(uint32_t) * tags->faceTagCount * tags->elementsPerFace);
		assert(tags->indices != 0);
	}
}

void _obj_RemoveTags(OBJObject* objects, size_t objectCount, StardustMeshFlags flags)
{
	unsigned int ignoreOBJTags = 0;

	if ((flags & STARDUST_MESH_IGNORE_UVS) != 0)
		ignoreOBJTags |= OBJTAG_TEXCOORD;

	if ((flags & STARDUST_MESH_IGNORE_NORMALS) != 0)
		ignoreOBJTags |= OBJTAG_NORMAL;

	for (size_t i = 0; i < objectCount; i++)
		objects[i].tags->tags &= ~ignoreOBJTags;
}

StardustErrorCode _obj_FillObjectData(FILE* file, OBJObject* objects, size_t objectCount)
{
	// Define variables //
	char buffer[MAX_LINE_BUFFER_SIZE]; //Ignmoring EOL checks since they were done by _obj_GetObjects

	OBJObject* currentObject = 0;

	int ignore_texCoords = 0;
	int ignore_normals = 0;

	while (fgets(buffer, sizeof(buffer), file))
	{

		//Split line by spaces
		size_t splitCount;
		char** splitLine = sd_SplitString(buffer, ' ', &splitCount);

		//Get prefix
		char* prefix = splitLine[0];

		//Compare prefix with tags
		if (strcmp(prefix, "o") == 0) //Object
		{
			//Find current object in objects
			for (int i = 0; i < objectCount; i++)
			{
				if (strcmp(objects[i].name, splitLine[1]) == 0)
				{
					currentObject = &objects[i];
					break;
				}
			}

			ignore_texCoords = (currentObject->tags->tags & OBJTAG_TEXCOORD) == 0;
			ignore_normals = (currentObject->tags->tags & OBJTAG_NORMAL) == 0;
		}

		else if (strcmp(prefix, "v") == 0) //Vertex
		{
			_obj_ParseVector(splitLine, currentObject->tags->vertices + currentObject->tags->vertexPosition, currentObject->tags->elementsPerVertex);
			currentObject->tags->vertexPosition += currentObject->tags->elementsPerVertex;
		}

		else if (!ignore_texCoords && strcmp(prefix, "vt") == 0) //Texture coordinate
		{
			_obj_ParseVector(splitLine, currentObject->tags->texCoords + currentObject->tags->texCoordPosition, currentObject->tags->elementsPerTexCoord);
			currentObject->tags->texCoordPosition += currentObject->tags->elementsPerTexCoord;
		}
	
		else if (!ignore_normals && strcmp(prefix, "vn") == 0) //Normal
		{
			_obj_ParseVector(splitLine, currentObject->tags->normals + currentObject->tags->normalPosition, 3);
			currentObject->tags->normalPosition += 3;
		}

		else if (strcmp(prefix, "f") == 0) //Face
		{
			// Get Hash //
			for (uint32_t i = 0; i < currentObject->tags->elementsPerFace; i++)
			{
				uint32_t hash = _obj_GetHash(splitLine[i + 1], currentObject->tags); //The warning is techinally checked in the _obj_GetObjects func but the compiler doens't know that

				// Check if hash exists in array //
				int hashPos = _obj_HashInArray(currentObject->tags->vertexHashes, currentObject->tags->faceTagCount * currentObject->tags->elementsPerFace, hash);

				if (hashPos != -1) //If hash exists. Add hash index
					currentObject->tags->indices[currentObject->tags->indexPosition] = hashPos;
				else //Otherwise add hash to hashes and then add hash index.
				{
					currentObject->tags->vertexHashes[currentObject->tags->hashPosition] = hash;
					currentObject->tags->indices[currentObject->tags->indexPosition] = currentObject->tags->hashPosition;

					currentObject->tags->hashPosition++; //Increment counter
				}
				currentObject->tags->indexPosition++; //Increment counter
			}
		}


		sd_FreeStringArray(splitLine, splitCount);

	} // while (fgets(buffer, sizeof(buffer), file))

	return STARDUST_ERROR_SUCCESS;
}

StardustErrorCode _obj_FillMeshes(StardustMesh* meshes, OBJObject* objects, size_t objectCount)
{
	for (int i = 0; i < objectCount; i++)
	{
		// Create Vertex Array //
		Vertex* vertices = malloc(sizeof(Vertex) * objects[i].tags->hashPosition);
		assert(vertices != 0);
		memset(vertices, 0, sizeof(Vertex) * objects[i].tags->hashPosition);

		int ignore_texCoords = (objects[i].tags->tags & OBJTAG_TEXCOORD) == 0;
		int ignore_normals = (objects[i].tags->tags & OBJTAG_NORMAL) == 0;

		int include_W = objects[i].tags->elementsPerVertex == 4 || objects[i].tags->elementsPerVertex == 7;
		int include_rgb = objects[i].tags->elementsPerVertex > 4;

		//Iterate over hashes
		for (uint32_t j = 0; j < objects[i].tags->hashPosition; j++)
		{
			uint32_t vI, tI, nI;
			_obj_DecomposeHash(objects[i].tags->vertexHashes[j], objects[i].tags, &vI, &tI, &nI);

			vI *= objects[i].tags->elementsPerVertex;
			tI *= objects[i].tags->elementsPerTexCoord;
			nI *= 3;

			vertices[j].x = objects[i].tags->vertices[vI];
			vertices[j].y = objects[i].tags->vertices[vI + 1];
			vertices[j].z = objects[i].tags->vertices[vI + 2];
			if (include_W)
				vertices[j].w = objects[i].tags->vertices[vI + 3];
			else
				vertices[j].w = 1.0f;

			if (include_rgb)
			{
				vertices[j].r = objects[i].tags->vertices[vI + 3 + include_W];
				vertices[j].g = objects[i].tags->vertices[vI + 4 + include_W];
				vertices[j].b = objects[i].tags->vertices[vI + 5 + include_W];
			}

			if (!ignore_texCoords)
			{
				vertices[j].texU = objects[i].tags->texCoords[tI];
				if (objects[i].tags->elementsPerTexCoord > 1)
				{
					vertices[j].texV = objects[i].tags->texCoords[tI + 1];
					if (objects[i].tags->elementsPerTexCoord > 2)
						vertices[j].texW = objects[i].tags->texCoords[tI + 2];
				}
			}

			if (!ignore_normals)
			{
				vertices[j].normX = objects[i].tags->normals[nI];
				vertices[j].normY = objects[i].tags->normals[nI + 1];
				vertices[j].normZ = objects[i].tags->normals[nI + 2];
			}
		}

		meshes[i].vertices = vertices;
		meshes[i].vertexCount = objects[i].tags->hashPosition;

		// Indices //
		meshes[i].indices = malloc(sizeof(uint32_t) * objects[i].tags->indexPosition);
		assert(meshes[i].indices != 0);
		memcpy(meshes[i].indices, objects[i].tags->indices, sizeof(uint32_t) * objects[i].tags->indexPosition);

		meshes[i].indexCount = objects[i].tags->indexPosition;

		meshes[i].dataType = STARDUST_VERTEX_DATA | STARDUST_INDEX_DATA;
		if (include_rgb)
			meshes[i].dataType |= STARDUST_COLOR_DATA;
		if (!ignore_texCoords)
			meshes[i].dataType |= STARDUST_TEXTURE_DATA;
		if (!ignore_normals)
			meshes[i].dataType |= STARDUST_NORMAL_DATA;
		if ((objects[i].tags->tags & OBJTAG_SMOOTH) == OBJTAG_SMOOTH)
			meshes[i].dataType |= STARDUST_SMOOTHSHADING;
	}

	return STARDUST_ERROR_SUCCESS;
}

void _obj_ParseVector(char** string, float* arr, uint32_t elemCount)
{
	for (uint32_t i = 0; i < elemCount; i++)
		arr[i] = (float)strtod(string[i + 1], NULL);
}

uint32_t _obj_GetHash(char* string, OBJTags* tags)
{
	size_t indiceCount = 0;
	char** splitLine = sd_SplitString(string, '/', &indiceCount);

	if (tags->indicesPerVertex == 1)
	{
		uint32_t vI = (uint32_t)atoi(splitLine[0]) - 1;
		return vI;
	}
	else if (tags->indicesPerVertex == 2)
	{
		uint32_t vI = (uint32_t)atoi(splitLine[0]) - 1;
		uint32_t tI = (uint32_t)atoi(splitLine[1]) - 1;
		return (tI * tags->vertexTagCount) + vI;
	}
	else if (tags->indicesPerVertex == 3)
	{
		uint32_t vI = (uint32_t)atoi(splitLine[0]) - 1;
		uint32_t tI = (uint32_t)atoi(splitLine[1]) - 1;
		uint32_t nI = (uint32_t)atoi(splitLine[2]) - 1;
		return (nI * tags->texCoordTagCount * tags->vertexTagCount) + (tI * tags->vertexTagCount) + vI;
	}

	sd_FreeStringArray(splitLine, indiceCount);

	return 0;
}

int _obj_HashInArray(uint32_t* arr, size_t arrSize, uint32_t hash)
{
	for (int i = 0; i < arrSize; i++)
	{
		if (arr[i] == hash)
			return i;
	}

	return -1;
}

void _obj_DecomposeHash(uint32_t hash, OBJTags* tags, uint32_t* vI, uint32_t* tI, uint32_t* nI)
{
	if (tags->indicesPerVertex == 1)
	{
		*nI = 0;
		*tI = 0;
		*vI = hash;
	}
	else if (tags->indicesPerVertex == 2)
	{
		*nI = 0;
		*tI = hash / tags->vertexTagCount;
		*vI = hash % tags->vertexTagCount;
	}
	else if (tags->indicesPerVertex == 3)
	{
		*nI = hash / (tags->vertexTagCount * tags->texCoordTagCount);
		uint32_t z = hash % (tags->vertexTagCount * tags->texCoordTagCount);
		*tI = z / tags->vertexTagCount;
		*vI = z % tags->vertexTagCount;
	}
}

void _obj_FreeObject(OBJObject* obj)
{
	//Free name
	free(obj->name);
	
	//Check tags
	if (obj->tags != 0)
		_obj_FreeTags(obj->tags);

	free(obj);
}

void _obj_FreeObjects(OBJObject* objs, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		_obj_FreeObject(objs);
	}
}

void _obj_FreeTags(OBJTags* tags)
{
	if (tags->vertices != 0) //Vertices
		free(tags->vertices);
	if (tags->texCoords != 0) //Tex coords
		free(tags->texCoords);
	if (tags->normals != 0) //Normals
		free(tags->normals);
	if (tags->vertexHashes != 0) //Hashes
		free(tags->vertexHashes);
	if (tags->indices != 0) //Indices
		free(tags->indices);
}

/*StardustErrorCode sd_LoadOBJFromFile(const char* filename, StardustMeshFlags flags, StardustMesh** mesh)
{
	//Assume that error file checking has been done by sd_LoadMesh

	FILE* fileHandle;
	errno_t fileRet = fopen_s(&fileHandle, filename, "r");
	if (fileRet != 0)
	{
		return 0;
	}
	
	//Define line buffer
	char buffer[MAX_LINE_BUFFER_SIZE];

	//Counters
	uint32_t vertexCount		= 0;
	uint32_t normalCount		= 0;
	uint32_t uvCount			= 0;
	uint32_t indexCount		= 0;
	uint32_t vertexPerFace	= 0;

	while (fgets(buffer, sizeof(buffer), fileHandle))
	{
		//Check line \n
		if (strchr(buffer, (int)'\n') == NULL) //No end of line. Line exceeds buffer
		{
			fclose(fileHandle);
			return STARDUST_ERROR_LINE_EXCCEDS_BUFFER;
		}

		//Check first character
		size_t lineComponentCount;
		char** lineComponents = sd_SplitString(buffer, ' ', &lineComponentCount);

		if (lineComponents == 0)
			continue;

		char* prefix = lineComponents[0];

		//Sort prefix
		if (strcmp(prefix, "v") == 0)
			vertexCount++;
		else if (strcmp(prefix, "vn") == 0)
			normalCount++;
		else if (strcmp(prefix, "vt") == 0)
			uvCount++;
		else if (strcmp(prefix, "f") == 0)
		{
			indexCount++; 
		
			if (vertexPerFace == 0) //Have we check yet?
				vertexPerFace = (uint32_t)lineComponentCount - 1;
		}

		sd_FreeStringArray(lineComponents, lineComponentCount);
	}

	//Check that we found enough vertices and faces
	if (vertexCount < 1 || indexCount < 1)
	{
		fclose(fileHandle);
		return STARDUST_ERROR_FILE_INVALID;
	}

	uint32_t maxIndexCount = indexCount * 3;

	//Allocate arrays
	float* vertices = malloc(sizeof(float) * vertexCount * 3);
	float* normals = malloc(sizeof(float) * normalCount * 3);
	float* uvs = malloc(sizeof(float) * uvCount * 2);
	uint32_t* indices = malloc(sizeof(uint32_t) * maxIndexCount);
	uint64_t* hashes = malloc(sizeof(uint64_t) * maxIndexCount);

	//Allocate array position counters
	uint32_t vertexPos		= 0;
	uint32_t normalPos		= 0;
	uint32_t uvPos			= 0;
	uint32_t indexPos		= 0;
	uint32_t hashPos		= 0;

	//Reset file pointer
	fseek(fileHandle, 0, 0);

	//Set locale before calls to strtod
	char* previousLocal = setlocale(LC_NUMERIC, NULL);
	setlocale(LC_NUMERIC, "");
	
	//Iterate over file
	int error = 0;
	while (fgets(buffer, sizeof(buffer), fileHandle))
	{
		//Ignore \n check as it was done earlier

		//Get prefix
		size_t lineComponentCount;
		char** lineComponents = sd_SplitString(buffer, ' ', &lineComponentCount);

		if (lineComponents == 0)
			continue;

		char* prefix = lineComponents[0];

		//Sort
		if (strcmp(prefix, "v") == 0) //Vertex
		{
			int ret = _obj_Parse3ComponentLine(lineComponents, lineComponentCount, vertices + vertexPos);
			if (ret == 0)
				error = 1;

			vertexPos += 3;
		}

		else if (strcmp(prefix, "vn") == 0)
		{
			int ret = _obj_Parse3ComponentLine(lineComponents, lineComponentCount, normals + normalPos);
			if (ret == 0)
				error = 1;

			normalPos += 3;
		}

		else if (strcmp(prefix, "vt") == 0)
		{
			int ret = _obj_Parse2ComponentLine(lineComponents, lineComponentCount, uvs + uvPos);
			if (ret == 0)
				error = 1;

			uvPos += 2;
		}

		else if (strcmp(prefix, "f") == 0)
		{
			for (uint32_t i = 0; i < vertexPerFace; i++)
			{
				uint64_t hash = _obj_GetVertexHash(lineComponents[i + 1], vertexCount, uvCount, normalCount);
				//Check if hash exists
				int idx = _obj_isInArray(hashes, maxIndexCount, hash);
				if (idx != -1)
				{
					//Get obj index
					indices[indexPos] = idx;
				}
				else
				{
					hashes[hashPos] = hash;
					indices[indexPos] = hashPos;
					hashPos++;
				}
				indexPos++;
			}
		}

		sd_FreeStringArray(lineComponents, lineComponentCount);
		
		if (error)
			break;
	}

	if (error)
	{
		free(vertices);
		free(normals);
		free(uvs);
		free(indices);
		free(hashes);

		setlocale(LC_NUMERIC, previousLocal);
		fclose(fileHandle);

		return STARDUST_ERROR_FILE_INVALID;
	}
	
	assert(_CrtCheckMemory());

	//Compile everything into vertices
	Vertex* vertexArray = malloc(sizeof(Vertex) * hashPos);

	assert(_CrtCheckMemory());

	for (uint32_t i = 0; i < hashPos; i++)
	{
		//Extract indices
		uint32_t vertexIndex, uvIndex, normalIndex;
		_obj_DecomposeVertexHash(hashes[i], vertexCount, uvCount, &vertexIndex, &uvIndex, &normalIndex); //Get indices

		vertexIndex *= 3; //Multiply to match arrays
		uvIndex *= 2;
		normalIndex *= 3;

		//Construct Vertex
		vertexArray[i].x = vertices[vertexIndex];
		vertexArray[i].y = vertices[vertexIndex + 1];
		vertexArray[i].z = vertices[vertexIndex + 2];

		vertexArray[i].u = uvs[uvIndex];
		vertexArray[i].v = uvs[uvIndex + 1];

		vertexArray[i].normX = normals[normalIndex];
		vertexArray[i].normY = normals[normalIndex + 1];
		vertexArray[i].normZ = normals[normalIndex + 2];
	}

	assert(_CrtCheckMemory());

	//Create mesh
	*mesh = malloc(sizeof(StardustMesh));

	(*mesh)->vertices = vertexArray;
	(*mesh)->indices = indices;
	(*mesh)->vertexCount = hashPos;
	(*mesh)->indexCount = indexPos;

	assert(_CrtCheckMemory());

	free(vertices);
	free(normals);
	free(uvs);
	free(hashes);

	setlocale(LC_NUMERIC, previousLocal);
	fclose(fileHandle);

	return STARDUST_ERROR_SUCCESS;
}

int _obj_Parse3ComponentLine(char** line, size_t elemCount, float* arr)
{
	//Validate line count
	if (elemCount != 4)
	{
		return 0;
	}

	//Extract numbers as strings
	arr[0] = (float)strtod(line[1], NULL);
	arr[1] = (float)strtod(line[2], NULL);
	arr[2] = (float)strtod(line[3], NULL);

	return 1;
}

int _obj_Parse2ComponentLine(char** line, size_t elemCount, float* arr)
{
	//Validate line count
	if (elemCount != 3)
	{
		return 0;
	}

	//Extract numbers as strings
	arr[0] = (float)strtod(line[1], NULL);
	arr[1] = (float)strtod(line[2], NULL);

	return 1;
}

int _obj_ParseFaceLine(char** line, size_t elemCount, size_t verticesPerFace, uint16_t* indices)
{
	//Validate line count
	if (elemCount != 4)
	{
		return 0;
	}


	return 1;
}

uint64_t _obj_GetVertexHash(char* vertexStr, size_t totalVertices, size_t totalUVS, size_t totalNormals)
{
	size_t count;
	char** split = sd_SplitString(vertexStr, '/', &count);

	int vIdx = atoi(split[0]) - 1;
	int tIdx = atoi(split[1]) - 1;
	int nIdx = atoi(split[2]) - 1;

	sd_FreeStringArray(split, count);

	return (nIdx * totalVertices * totalUVS) + (tIdx * totalVertices) + vIdx;
}

void _obj_DecomposeVertexHash(uint64_t hash, uint32_t vertexCount, uint32_t uvCount, uint32_t* vertexIndex, uint32_t* uvIndex, uint32_t* normalIndex)
{
	*normalIndex = (uint32_t)(hash / ((uint64_t)vertexCount * (uint64_t)uvCount));

	uint32_t z = (uint32_t)(hash % ((uint64_t)vertexCount * (uint64_t)uvCount));

	*uvIndex = z / vertexCount;
	*vertexIndex = z % vertexCount;
}

int _obj_isInArray(uint64_t* arr, uint32_t arrSize, uint64_t val)
{
	for (uint32_t i = 0; i < arrSize; i++)
	{
		if (arr[i] == val)
			return (int)i;
	}

	return -1;
}*/


