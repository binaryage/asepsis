#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <limits.h>

#include "echelon.h"
#include "common.h"
#include "logging.h"

#include "shared.h" 

static pthread_t g_monitor_thread_id = 0;
static int g_monitor_entered_loop = 0;
static int g_monitor_socket = -1;
static int g_received_signal = 0;

extern int asepsis_run_monitor(void);
extern int asepsis_stop_monitor(void);

// http://stackoverflow.com/questions/3184445/how-to-clear-directory-contents-in-c-on-linux-basically-i-want-to-do-rm-rf
static int rmdir_recursive_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    DLOG("remove %s", fpath);
    int res = remove(fpath);
    if (res) {
        ERROR("unable to remove %s (%s)", fpath, strerror(errno));
    }
    return res;
}
 
static int rmdir_recursive(const char *path) {
    struct stat sb;
    
    // check full path for presence
    if (stat(path, &sb) == 0) {
        if (S_ISDIR (sb.st_mode)) {
            DLOG("rmdir_recursive %s", path);
            return nftw(path, rmdir_recursive_cb, 64, FTW_DEPTH | FTW_PHYS);
        } else {
            DLOG("remove %s", path);
            int res = remove(path); // in case path is just a file path
            if (res) {
                ERROR("unable to remove %s (%s)", path, strerror(errno));
            }
            return 0;
        }
    }
    
    return 0;
}

#define REENTRY(x) 1
#define path "asepsis_handle_delete"
static int asepsis_handle_delete(const char* path1) {
    int res = 0;
    
    if (isPrefixPath(path1)) {
        return 0;
    }
    
    char prefixPath1[SAFE_PREFIX_PATH_BUFFER_SIZE];
    if (!makePrefixPath(path1, prefixPath1)) {
        return 0; // the prefixed path would be too long
    }

    SUSPEND_LOCK_CHECK();
    
    if (!isDirectory(prefixPath1)) {
        // nothing to do, source dir is not present in our cache
        SUSPEND_LOCK_RELEASE();
        return 0;
    }

    // do mirror remove
    DLOG("handle delete %s", prefixPath1);
    SERIALIZATION_LOCK_CHECK();
    res = rmdir_recursive(prefixPath1); // this is dangerous, but we know that our path has prefix at least
    if (res) {
        ERROR("unable to remove directory %s (%s)", prefixPath1, strerror(errno));
    }
    SERIALIZATION_LOCK_RELEASE();
    SUSPEND_LOCK_RELEASE();
    return res;
}

#undef path
#define path "asepsis_handle_rename"
static int asepsis_handle_rename(const char* path1, const char* path2) {
    int res = 0;
    
    if (isPrefixPath(path1) || isPrefixPath(path2)) {
        return 0;
    }
    
    char prefixPath1[SAFE_PREFIX_PATH_BUFFER_SIZE];
    if (!makePrefixPath(path1, prefixPath1)) {
        return 0; // the prefixed path would be too long
    }
    
    char prefixPath2[SAFE_PREFIX_PATH_BUFFER_SIZE];
    if (!makePrefixPath(path2, prefixPath2)) {
        return 0; // the prefixed path would be too long
    }

    SUSPEND_LOCK_CHECK();

    if (!isDirectory(prefixPath1)) {
        // nothing to do, source dir is not present in our cache
        SUSPEND_LOCK_RELEASE();
        return 0;
    }
    
    // do mirror rename
    DLOG("handle rename %s -> %s", prefixPath1, prefixPath2);
    
    SERIALIZATION_LOCK_CHECK();
    
    // here we need to be especially careful
    // rename(2) may fail under some special circumstances
    // 1. something gets into the way of renaming
    // 2. parent folder is missing for new name
    rmdir_recursive(prefixPath2); // there may be some old content (pathological case)
    ensureDirectoryForPath(prefixPath2); // like mkdir -p $prefixPath2 (but excluding last directory)
    
    // ok, now it should be safe to call rename
    res = rename(prefixPath1, prefixPath2); // this is dangerous, but we know that our path has prefix at least
    if (res) {
        ERROR("unable to rename directories %s -> %s (%s)", prefixPath1, prefixPath2, strerror(errno));
    }

    SERIALIZATION_LOCK_RELEASE();
    SUSPEND_LOCK_RELEASE();
    return res;
}

static void* asepsis_monitor_thread(void* data) {
    DLOG("asepsis_monitor_thread starting");
    
    struct sockaddr_ctl sc;
    struct EchelonMessage em;
    struct ctl_info ctl_info;
    
    g_monitor_socket = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (g_monitor_socket < 0) {
        ERROR("unable to allocate SYSPROTO_CONTROL socket");
        g_monitor_thread_id = 0;
        return NULL;
    }
    bzero(&ctl_info, sizeof(struct ctl_info));
    strcpy(ctl_info.ctl_name, ECHELON_BUNDLE_ID);
    if (ioctl(g_monitor_socket, CTLIOCGINFO, &ctl_info) == -1) {
        ERROR("unable to ioctl CTLIOCGINFO: %s", strerror(errno));
        g_monitor_socket = -1;
        g_monitor_thread_id = 0;
        ERROR("unable to connect asepsis.kext, do you have the kernel extension running?");
        return NULL;
    } else {
        DLOG("asepsis.kext info: ctl_id=0x%x for ctl_name=%s", ctl_info.ctl_id, ctl_info.ctl_name);
    }
    
    bzero(&sc, sizeof(struct sockaddr_ctl));
    sc.sc_len = sizeof(struct sockaddr_ctl);
    sc.sc_family = AF_SYSTEM;
    sc.ss_sysaddr = SYSPROTO_CONTROL;
    sc.sc_id = ctl_info.ctl_id;
    sc.sc_unit = 0;
    
    if (connect(g_monitor_socket, (struct sockaddr*)&sc, sizeof(struct sockaddr_ctl))) {
        ERROR("unable to connect the socket: %s", strerror(errno));
        g_monitor_socket = -1;
        g_monitor_thread_id = 0;
        return NULL;
    }
    
    // now, just read the stuff!
    DLOG("asepsis_monitor_thread entering loop...");
    g_monitor_entered_loop = 1;
    while (1) {
        if (recv(g_monitor_socket, &em, sizeof(struct EchelonMessage), MSG_WAITALL) != sizeof(struct EchelonMessage)) {
            if (g_received_signal) {
                INFO("asepsis_monitor_thread received signal, closing ...");
            } else {
                ERROR("asepsis_monitor_thread socket was broken, closing ...");
            }
            break;
        }
        if (em.operation == ECHELON_OP_RENAME) {
            // DLOG("asepsis_monitor_thread received rename op %s -> %s", em.path1, em.path2);
            asepsis_handle_rename(em.path1, em.path2);
        }
        if (em.operation == ECHELON_OP_DELETE) {
            // DLOG("asepsis_monitor_thread received delete op %s", em.path1);
            asepsis_handle_delete(em.path1);
        }
    }
    
    close(g_monitor_socket);
    
    g_monitor_socket = -1;
    g_monitor_thread_id = 0;
    DLOG("asepsis_monitor_thread finished!");
    return NULL;
}

int asepsis_run_monitor(void) {
    pthread_attr_t attr;

    if (g_monitor_thread_id) {
        ERROR("already running!");
        return 1; // already running
    }
    
    // create the thread using POSIX routines
    if (pthread_attr_init(&attr)) {
        ERROR("pthread_attr_init failed: %m");
        return 1;
    }
    
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
        ERROR("pthread_attr_setdetachstate failed: %m");
        return 1;
    }

    int error = pthread_create(&g_monitor_thread_id, &attr, &asepsis_monitor_thread, NULL);

    if (pthread_attr_destroy(&attr)) {
        ERROR("pthread_attr_destroy failed: %m");
        return 1;
    }
    
    if (error) {
        ERROR("unable to spawn asepsis_monitor_thread (%d)", error);
        return 1;
    }
    
    // wait up-to 5s for main thread to enter the loop
    int counter = 0;
    while (!g_monitor_entered_loop) {
        counter++;
        if (counter>500) {
            return 3;
        }
        if (!g_monitor_thread_id) {
            return 2;
        }
        usleep(10000); // 10ms
    }

    // echelon connection is up and running :-)
    return 0;
}

int asepsis_stop_monitor() {
    if (!g_monitor_thread_id) {
        return 1; // not running
    }

    if (g_monitor_socket == -1) {
        return 2; // no socket
    }

    // signal thread to exit
    g_received_signal = 1;
    int res = close(g_monitor_socket);
    if (res) {
        return res;
    }

    while (g_monitor_thread_id) {
        usleep(10000);
    }

    return 0;
}
