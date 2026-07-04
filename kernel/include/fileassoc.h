#ifndef LOS_FILE_ASSOC_H
#define LOS_FILE_ASSOC_H

typedef int (*file_handler_t)(const char *path);

void fileassoc_initialize(void);

int fileassoc_register(
    const char *extension,
    file_handler_t handler
);

int fileassoc_open(const char *filename);

void fileassoc_list(void);

#endif
