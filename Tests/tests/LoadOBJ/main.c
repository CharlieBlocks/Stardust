#include "stardust.h"

const char* objectPath = ".\\tests\\resources\\cube.obj";

int main(int argc, char* argv[])
{

    //Load mesh
    StardustMesh* meshes = 0;
    size_t meshCount = 0;

    StardustErrorCode res = sd_LoadMesh(objectPath, 0, &meshes, &meshCount);
    if (res != STARDUST_ERROR_SUCCESS)
        return 1;

    //Delete mesh
    for (size_t i = 0; i < meshCount; i++)
        sd_FreeMesh(&meshes[i]);

    return 0;
}
