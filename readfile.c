
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

char *read_file(
    const char *name,
    int *len
)
{
    FILE *file;

    char *buffer;

    unsigned long fileLen;

    //Open file
    file = fopen(name, "rb");
    if (!file)
    {
        fprintf(stderr, "Unable to open file %s\n", name);
        return NULL;
    }

    //Get file length
    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);

    //Allocate memory
    if (!(buffer = (char *) malloc(fileLen + 1)))
    {
        fprintf(stderr, "Memory error!\n");
        fclose(file);
        return NULL;
    }

    //Read file contents into buffer
    fread(buffer, fileLen, 1, file);
    fclose(file);

    *len = fileLen + 1;

    return buffer;
    //Do what ever with buffer
//      free(buffer);
}
