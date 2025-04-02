#ifndef XLIB_H
#define XLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct XLib_Handle XLib_Handle;

typedef enum {
    XLIB_OK = 0,
    XLIB_ERROR_OPEN = -1,
    XLIB_ERROR_SYMBOL = -2,
    XLIB_ERROR_CLOSE = -3,
    XLIB_ERROR_INVALID = -4,
    XLIB_ERROR_FORMAT = -5
} XLib_Error;

XLib_Handle* XLib_open(const char* path);
void* XLib_sym(XLib_Handle* handle, const char* symbol);
int XLib_close(XLib_Handle* handle);
const char* XLib_error(void);

#ifdef __cplusplus
}
#endif

#endif
