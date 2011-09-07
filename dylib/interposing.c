// Interposing a library functions through dyld
// read: Mac OS X Internals: A Systems Approach, by Amit Singh
//       http://osxbook.com
//

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "echelon.h"
#include "common.h"
#include "logging.h"

#include "shared.c" 

////////////////////////////////////////////////////////////////////////////////////////////////////
// these are low-level libc functions used by DesktopServicesPriv.framework to access .DS_Store files (in Lion 10.7)
// note: in previous OS versions DesktopServicesPriv.framework was using File Manager from Core Services

int asepsis_open(const char *, int, mode_t);
int asepsis_openx_np(const char *, int, filesec_t);
int asepsis_getattrlist(const char *path, void *alist, void *attributeBuffer, size_t bufferSize, unsigned int options);
int asepsis_setattrlist(const char *path, void *alist, void *attributeBuffer, size_t bufferSize, unsigned int options); 

////////////////////////////////////////////////////////////////////////////////////////////////////
// interposing magic

typedef struct interpose_s {
  void *new_func;
  void *orig_func;
} interpose_t;

// taken from dyld/include/mach-o/dyld-interposing.h
#define DYLD_INTERPOSE(_replacement,_replacee) \
    __attribute__((used)) static struct{ const void* replacement; const void* replacee; } _interpose_##_replacee \
        __attribute__ ((section ("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacement, (const void*)(unsigned long)&_replacee };

#define INTERPOSE(name) DYLD_INTERPOSE(asepsis_##name, name)

INTERPOSE(open);
INTERPOSE(openx_np);
INTERPOSE(getattrlist);
INTERPOSE(setattrlist);

////////////////////////////////////////////////////////////////////////////////////////////////////
// interposed functions

#define REENTRY(path) open(path, flags, mode)
int asepsis_open(const char * path, int flags, mode_t mode) {
    if (!isDSStorePath(path)) {
        return REENTRY(path);
    }
    if (isDisabled()) {
        return REENTRY(path);
    }
    if (isPrefixPath(path)) {
        return REENTRY(path);
    }
    if (isDSStorePresentOnPath(path)) {
        return REENTRY(path);
    }
    char prefixPath[SAFE_PREFIX_PATH_BUFFER_SIZE];
    if (!makePrefixPath(path, prefixPath)) {
        return REENTRY(path);
    }
    SUSPEND_LOCK_CHECK();
    underscorePatch(prefixPath);
    SERIALIZATION_LOCK_CHECK();
    DLOG("asepsis_open %s (flags=%016x mode=%016x) -> %s", path, flags, mode, prefixPath);
    ensureDirectoryForPath(prefixPath);
    int res = REENTRY(prefixPath);
    SERIALIZATION_LOCK_RELEASE();
    SUSPEND_LOCK_RELEASE();
    return res;
}
#undef REENTRY

#define REENTRY(path) openx_np(path, flags, sec)
int asepsis_openx_np(const char * path, int flags, filesec_t sec) {
    if (!isDSStorePath(path)) {
        return REENTRY(path);
    }
    if (isDisabled()) {
        return REENTRY(path);
    }
    if (isPrefixPath(path)) {
        return REENTRY(path);
    }
    if (isDSStorePresentOnPath(path)) {
        return REENTRY(path);
    }
    char prefixPath[SAFE_PREFIX_PATH_BUFFER_SIZE];
    if (!makePrefixPath(path, prefixPath)) {
        return REENTRY(path);
    }
    SUSPEND_LOCK_CHECK();
    underscorePatch(prefixPath);
    SERIALIZATION_LOCK_CHECK();
    DLOG("openx_np %s (flags=%016x mode=%16p) -> %s", path, flags, sec, prefixPath);
    ensureDirectoryForPath(prefixPath);
    int res = REENTRY(prefixPath);
    SERIALIZATION_LOCK_RELEASE();
    SUSPEND_LOCK_RELEASE();
    return res;
}
#undef REENTRY

#define REENTRY(path) getattrlist(path, alist, attributeBuffer, bufferSize, options)
int asepsis_getattrlist(const char *path, void *alist, void *attributeBuffer, size_t bufferSize, unsigned int options) {
    if (!isDSStorePath(path)) {
        return REENTRY(path);
    }
    if (isDisabled()) {
        return REENTRY(path);
    }
    if (isPrefixPath(path)) {
        return REENTRY(path);
    }
    if (isDSStorePresentOnPath(path)) {
        return REENTRY(path);
    }
    char prefixPath[SAFE_PREFIX_PATH_BUFFER_SIZE];
    if (!makePrefixPath(path, prefixPath)) {
        return REENTRY(path);
    }
    SUSPEND_LOCK_CHECK();
    underscorePatch(prefixPath);
    SERIALIZATION_LOCK_CHECK();
    DLOG("getattrlist %s -> %s", path, prefixPath);
    int res = REENTRY(prefixPath);
    SERIALIZATION_LOCK_RELEASE();
    SUSPEND_LOCK_RELEASE();
    return res;
}
#undef REENTRY

#define REENTRY(path) setattrlist(path, alist, attributeBuffer, bufferSize, options)
int asepsis_setattrlist(const char *path, void *alist, void *attributeBuffer, size_t bufferSize, unsigned int options) {
    if (!isDSStorePath(path)) {
        return REENTRY(path);
    }
    if (isDisabled()) {
        return REENTRY(path);
    }
    if (isPrefixPath(path)) {
        return REENTRY(path);
    }
    if (isDSStorePresentOnPath(path)) {
        return REENTRY(path);
    }
    char prefixPath[SAFE_PREFIX_PATH_BUFFER_SIZE];
    if (!makePrefixPath(path, prefixPath)) {
        return REENTRY(path);
    }
    SUSPEND_LOCK_CHECK();
    underscorePatch(prefixPath);
    SERIALIZATION_LOCK_CHECK();
    DLOG("setattrlist %s -> %s", path, prefixPath);
    int res = REENTRY(prefixPath);
    SERIALIZATION_LOCK_RELEASE();
    SUSPEND_LOCK_RELEASE();
    return res;
}
#undef REENTRY