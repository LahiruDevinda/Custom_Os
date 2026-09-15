#ifndef FS_H
#define FS_H

#include "../include/types.h"

#define MAX_FILES       16
#define MAX_FILENAME    32
#define MAX_FILE_SIZE   512

typedef struct {
    char     name[MAX_FILENAME];
    uint32_t size;
    uint8_t  data[MAX_FILE_SIZE];
    bool     used;
} file_entry_t;

/* Core File System APIs */
void fs_init(void);
int  fs_create(const char *name, const char *content);
void cmd_ls(void);
void cmd_cat(const char *filename);

#endif /* FS_H */
