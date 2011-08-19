#import <CommonCrypto/CommonDigest.h>

#include <assert.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/param.h>
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

#include "echelon.h"
#include "Agent.h"

#ifdef _DEBUG
#define DLOG(...) NSLog(__VA_ARGS__)
#else
#define DLOG(...)
#endif

#define ERROR(s, ...) NSBeep(),NSLog(s, __VA_ARGS__)

void makePrefixPath(const char* path, char* res);

// /some/dir/.DS_Store -> /usr/local/.dscage/some/dir/_DS_Store
// or -> /usr/local/.dscage/..longpaths/<md5(path)>/_DS_Store in case of very long path
// res must have at least size of PATH_MAX+PATH_MAX+1
void makePrefixPath(const char* path, char* res) {
    size_t len1 = strlcpy(res, ECHELON_DSCAGE_PREFIX, PATH_MAX);
    size_t len2 = strlcpy(res + len1, path, PATH_MAX);
    size_t total = len1 + len2;
    if (total >= PATH_MAX) {
        // our prefix + original path length has exceeded PATH_MAX, use path hash instead to fit into given space
        unsigned char result[CC_MD5_DIGEST_LENGTH];
        CC_MD5(path, (CC_LONG)strlen(path), result);
        snprintf(res + len1, PATH_MAX, "/..longpaths/%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x/_DS_Store", 
                 result[0], result[1], result[2], result[3], 
                 result[4], result[5], result[6], result[7],
                 result[8], result[9], result[10], result[11],
                 result[12], result[13], result[14], result[15]
                 );
        return;
    }
    *(res + total - DSSTORE_LEN) = '_';
}

static pthread_attr_t attr;
static pthread_t posixThreadID = 0;
static bool threadStartFailed = false;

static int fssocket = -1;
static int fsmode = 0;
static pthread_mutex_t fslock;

static int wait1;
static int wait2;

static NSFileManager* fm = NULL;

static bool isDirectory(NSString* path) {
    BOOL dir = FALSE;
    
    if ([fm fileExistsAtPath:path isDirectory:&dir]) {
        if (dir) return true;
    }
    return false;
}

static int handleDelete(const char* path1) {
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    int res = 0;
    
    char p1[SAFE_PREFIX_PATH_BUFFER_SIZE];
    makePrefixPath(path1, p1);
    
    DLOG(@"handleDelete %s", p1);
    
    NSString* np1 = [NSString stringWithUTF8String:p1];
    if (!isDirectory(np1)) {
        // nothing to do, source dir is not present in cache
        DLOG(@"  => dir does not exist, ignoring ...");
    } else {
        NSError* error;
        if (![fm removeItemAtPath:np1 error:&error]) {
            ERROR(@".dscache worker: failed to removeItemAtPath: %@ (%@)", np1, error);
            res = 1;
        }
    }
    
    [pool release];
    return res;
}

static int handleRename(const char* path1, const char* path2) {
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    int res = 0;
    
    char p1[SAFE_PREFIX_PATH_BUFFER_SIZE];
    makePrefixPath(path1, p1);
    
    char p2[SAFE_PREFIX_PATH_BUFFER_SIZE];
    makePrefixPath(path2, p2);
    
    DLOG(@"handleRename %s -> %s", p1, p2);
    
    NSString* np1 = [NSString stringWithUTF8String:p1];
    
    if (!isDirectory(np1)) {
        // nothing to do, source dir is not present in cache
        DLOG(@"  => p1 does not exist, ignoring ...");
    } else {
        NSError* error;
        NSString* np2 = [NSString stringWithUTF8String:p2];
        if (![fm copyItemAtPath:np1 toPath:np2 error:&error]) {
            ERROR(@".dscache worker: failed to copyItemAtPath:%@ toPath:%@ (%@)", np1, np2, error);
            res = 1;
        } else {
            if (![fm removeItemAtPath:np1 error:&error]) {
                ERROR(@".dscache worker: failed to removeItemAtPath: %@ (%@)", np1, error);
                res = 1;
            }
        }
    }
    
    [pool release];
    return res;
}

static void* FSMonitorMain(void* data) {
    DLOG(@"FSMonitor thread started!");
    wait1 = 1;
    
    struct sockaddr_ctl sc;
    struct EchelonMessage em;
    struct ctl_info ctl_info;
    
    fssocket = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fssocket < 0) {
        DLOG(@"unable to allocate SYSPROTO_CONTROL socket");
        posixThreadID = 0;
        threadStartFailed = true;
        DLOG(@"FSMonitor thread finished!");
        return NULL;
    }
    bzero(&ctl_info, sizeof(struct ctl_info));
    strcpy(ctl_info.ctl_name, ECHELON_BUNDLE_ID);
    if (ioctl(fssocket, CTLIOCGINFO, &ctl_info) == -1) {
        DLOG(@"unable to ioctl CTLIOCGINFO: %s", strerror(errno));
        fssocket = -1;
        posixThreadID = 0;
        threadStartFailed = true;
        DLOG(@"FSMonitor thread finished!");
        DLOG(@"unable to connect Echelon, do you have kernel extension running?");
        return NULL;
    } else {
        DLOG(@"ctl_id: 0x%x for ctl_name: %s", ctl_info.ctl_id, ctl_info.ctl_name);
    }
    
    bzero(&sc, sizeof(struct sockaddr_ctl));
    sc.sc_len = sizeof(struct sockaddr_ctl);
    sc.sc_family = AF_SYSTEM;
    sc.ss_sysaddr = SYSPROTO_CONTROL;
    sc.sc_id = ctl_info.ctl_id;
    sc.sc_unit = 0;
    
    if (connect(fssocket, (struct sockaddr*)&sc, sizeof(struct sockaddr_ctl))) {
        DLOG(@"unable to connect the socket: %s", strerror(errno));
        fssocket = -1;
        posixThreadID = 0;
        threadStartFailed = true;
        DLOG(@"FSMonitor thread finished!");
        return NULL;
    }
    
    // now, just read the stuff!
    wait2 = 2;
    while (1) {
        if (recv(fssocket, &em, sizeof(struct EchelonMessage), 0) != sizeof(struct EchelonMessage)) {
            DLOG(@"FSMonitor thread received signal or socket was broken, closing ...");
            break;
        }
        if (em.operation == ECHELON_OP_RENAME) {
            handleRename(em.path1, em.path2);
        }
        if (em.operation == ECHELON_OP_DELETE) {
            handleDelete(em.path1);
        }
    }
    
    close(fssocket);
    
    fssocket = -1;
    posixThreadID = 0;
    DLOG(@"FSMonitor thread finished!");
    return NULL;
}

static int startFSMonitor() {
    if (posixThreadID) {
        return 1; // already running
    }
    
    // create the thread using POSIX routines
    int res = pthread_attr_init(&attr);
    assert(!res);
    res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    assert(!res);
    
    wait1 = 0;
    wait2 = 0;
    threadStartFailed = false;
    int error = pthread_create(&posixThreadID, &attr, &FSMonitorMain, NULL);
    if (threadStartFailed) {
        posixThreadID = 0;
    }
    
    res = pthread_attr_destroy(&attr);
    assert(!res);
    
    if (error) {
        DLOG(@"unable to spawn FSMonitor thread (%d)", error);
        return 1;
    }
    
    // wait for thread to fully start
    while (!wait1) {
        usleep(10000);
        if (!posixThreadID || threadStartFailed) return 2;
    }
    
    while (!wait2) {
        usleep(10000);
        if (!posixThreadID || threadStartFailed) return 3;
    }
    
    // echelon connection is up and running :-)
    return 0;
}

static int stopFSMonitor() {
    if (!posixThreadID) {
        return 1; // not running
    }
    
    if (fssocket == -1) {
        return 2; // no socket
    }
    
    // signal thread to exit
    int res = close(fssocket);
    if (res) return res;
    
    while (posixThreadID) {
        usleep(10000);
    }
    
    return 0;
}

bool isFSMonitorRunning() {
    return posixThreadID != NULL;
}

int isOK() {
    int mode = getFSMode();
    
    return mode != 1 || isFSMonitorRunning();
}

static void SignalHandlerUsr1(int sigraised) {
    DLOG(@"Got USR1 Signal - %d", sigraised);
    setFSMode(DSTORE_MODE_CLASSIC);
}

int initFSMonitor() {
    pthread_mutex_init(&fslock, NULL);
    fm = [[NSFileManager alloc] init];
    
    // this is for controlling echelon connection from outside, handy during install
    signal(SIGUSR1, SignalHandlerUsr1);
    return 0;
}

int getFSMode() {
    int mode;
    
    pthread_mutex_lock(&fslock);
    mode = fsmode;
    pthread_mutex_unlock(&fslock);
    return mode;
}

int setFSMode(int mode) {
    int res;
    
    pthread_mutex_lock(&fslock);
    fsmode = mode;
    if (fsmode == DSTORE_MODE_REDIRECT) {
        res = startFSMonitor();
    } else {
        res = stopFSMonitor();
    }
    pthread_mutex_unlock(&fslock);
    return res;
}

