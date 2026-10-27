#ifndef PKWARE_H
#define PKWARE_H

#ifdef __cplusplus
extern "C" {
#endif

#define CMP_BUFFER_SIZE 36312 /* work buffer size for implode() */
#define EXP_BUFFER_SIZE 12596 /* work buffer size for explode() */

#define CMP_BINARY 0
#define CMP_ASCII  1

#define CMP_NO_ERROR         0
#define CMP_INVALID_DICTSIZE 1
#define CMP_INVALID_MODE     2
#define CMP_BAD_DATA         3
#define CMP_ABORT            4

unsigned int __cdecl implode(
    unsigned int(__cdecl *read_buf)(char *buf, unsigned int *size, void *param),
    void(__cdecl *write_buf)(char *buf, unsigned int *size, void *param),
    char         *work_buf,
    void         *param,
    unsigned int *type,
    unsigned int *dsize
);
unsigned int __cdecl explode(
    unsigned int(__cdecl *read_buf)(char *buf, unsigned int *size, void *param),
    void(__cdecl *write_buf)(char *buf, unsigned int *size, void *param),
    char *work_buf,
    void *param
);

#ifdef __cplusplus
}
#endif

#endif
