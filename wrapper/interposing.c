#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <dlfcn.h>

#include "echelon.h"
#include "common.h"
#include "logging.h"
#include "mach_override.h" 

#include "shared.c" 

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// these are low-level libc functions used by DesktopServicesPriv.framework to access .DS_Store files (in OS X 10.7-10.8)
// note: in previous OS versions DesktopServicesPriv.framework was using File Manager from Core Services

int asepsis_open(const char *, int, mode_t);
int asepsis_openx_np(const char *, int, filesec_t);
int asepsis_getattrlist(const char *path, void *alist, void *attributeBuffer, size_t bufferSize, unsigned int options);
int asepsis_setattrlist(const char *path, void *alist, void *attributeBuffer, size_t bufferSize, unsigned int options); 

static int (*open_reentry)(const char *, int, mode_t);
static int (*openx_np_reentry)(const char *, int, filesec_t);
static int (*getattrlist_reentry)(const char *path, void *alist, void *attributeBuffer, size_t bufferSize, unsigned int options);
static int (*setattrlist_reentry)(const char *path, void *alist, void *attributeBuffer, size_t bufferSize, unsigned int options);

#define OVERRIDE(fn) err = mach_override_ptr(dlsym(RTLD_DEFAULT, #fn), (void*)&asepsis_##fn, (void**)& fn ## _reentry); CHECK_OVERRIDE(err, fn)

void init_asepsis(void) {
#if defined(__i386__)
    DLOG("Asepsis init %s", "(i386)");
#elif defined(__x86_64__)
    DLOG("Asepsis init %s", "(x86_64)");
#endif
    
    asepsis_setup_safe();
    
    // do not apply overrides when asepsis is disabled
    if (g_asepsis_disabled) {
        return;
    }

    kern_return_t err;

    OVERRIDE(open);
    OVERRIDE(openx_np);
    OVERRIDE(getattrlist);
    OVERRIDE(setattrlist);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// interposed functions

#define REENTRY(path) open_reentry(path, flags, mode)
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

#define REENTRY(path) openx_np_reentry(path, flags, sec)
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

#define REENTRY(path) getattrlist_reentry(path, alist, attributeBuffer, bufferSize, options)
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

#define REENTRY(path) setattrlist_reentry(path, alist, attributeBuffer, bufferSize, options)
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