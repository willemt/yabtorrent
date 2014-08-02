
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *file2strl(
    const char *path,
    unsigned int *file_len_out
)
{
    FILE *file;

    if (!(file = fopen(path, "rb")))
    {
        fprintf(stderr, "Unable to open file %s\n", path);
        return NULL;
    }

    if (-1 == fseek(file, 0, SEEK_END))
    {
        fprintf(stderr, "Unable to seek file %s\n", path);
        return NULL;
    }

    unsigned long file_len;
    if (-1 == (file_len = ftell(file)))
    {
        fprintf(stderr, "Unable to ftell() file %s\n", path);
        return NULL;
    }

    if (-1 == fseek(file, 0, SEEK_SET))
    {
        fprintf(stderr, "Unable to seek file %s\n", path);
        return NULL;
    }

    char *contents;
    if (!(contents = malloc(file_len + 1)))
    {
        fprintf(stderr, "Memory error!\n");
        fclose(file);
        return NULL;
    }

    fread(contents, file_len, 1, file);
    fclose(file);

    contents[file_len] = '\0';

    if (file_len_out)
        *file_len_out = file_len + 1;

    return contents;
}

char *file2str(
    const char *path,
    unsigned long *file_len_out
)
{
    return file2strl(path,NULL);
}

