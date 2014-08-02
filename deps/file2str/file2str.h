#ifndef FILE2STR_H
#define FILE2STR_H

/**
 * @param path File's path.
 * @param len Pointer to the length of the string returned
 * @return string contents of file; otherwise NULL on error */
char *file2strl(const char *path, unsigned int *len);

char *file2str(const char *path);

#endif /* FILE2STR_H */
