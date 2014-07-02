#ifndef STUBFILE_ALLOCATOR_H
#define STUBFILE_ALLOCATOR_H

int sf_write(void *sf, int global_bytepos, const int write_len, const void *data);

void *sf_read(void *sf, int global_bytepos, int bytes_to_read);

void *sf_new();

/**
 * Add a file to the allocator */
void sf_add_file(
    void* sf,
    const char *fname,
    const int fname_len,
    const int size
);

/**
 * @return number of files added to allocator */
int sf_get_nfiles(void* sf);

/**
 * @return the path of this file pointed to by this index */
const char *sf_file_get_path(void* sf, int idx);

/**
 * Set current working directory */
void sf_set_cwd(void * sf, const char *path);

/**
 * @return total file size in bytes */
unsigned int sf_get_total_size(void * sf);

#endif /* STUBFILE_ALLOCATOR_H */
