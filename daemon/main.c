// read about Daemons and Agents here: http://developer.apple.com/library/mac/#technotes/tn2083

#include <IOKit/kext/KextManager.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>

#include "common.h"
#include "logging.h"

extern int asepsis_run_monitor();
void asepsis_init(void);
void asepsis_setup_safe(void);
void asepsis_setup_logging(void);

int g_asepsis_disabled = 0;
pthread_mutex_t g_asepsis_mutex;
pthread_mutexattr_t g_asepsis_mutex_attr;
aslclient g_asepsis_asl = NULL;
aslmsg g_asepsis_log_msg = NULL;

// Either the calling process must have an effective user id of 0 (superuser),
// or the kext being loaded and all its dependencies must reside
// in /System and have an OSBundleAllowUserLoad property of true.
// ---
// we use OSBundleAllowUserLoad true case
static bool launchKext() {
    DLOG("launching KEXT from %s", KEXT_PATH);
    
    CFURLRef kextPath = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, CFSTR(KEXT_PATH), kCFURLPOSIXPathStyle, true);
    KextManagerLoadKextWithURL(kextPath, nil); // may fail because already loaded
    
    return true;
}

static int lock = 0;

static bool acquireLock() {
    lock = open(DAEMON_LOCK_PATH, O_CREAT|O_RDWR, S_IRWXU);
    if (flock(lock, LOCK_EX|LOCK_NB) != 0) {
        close(lock);
        return false;
    }
    return true;
}

static void releaseLock() {
    if (!lock) return;
    flock(lock, LOCK_UN);
    close(lock);
    unlink(DAEMON_LOCK_PATH);
}

// create a new ASL log
void asepsis_setup_logging(void) {
    static int asepsis_logging_initialized = 0;
    if (asepsis_logging_initialized) {
        return;
    }
    asepsis_logging_initialized = 1;
	g_asepsis_asl = asl_open("Asepsis", "daemon", 0);
	g_asepsis_log_msg = asl_new(ASL_TYPE_MSG);
	asl_set(g_asepsis_log_msg, ASL_KEY_SENDER, "Asepsis");    
}

// this is called first time Asepsis is going to do some action
static void asepsis_setup(void) {
    static int already_initialized = 0;
    if (already_initialized) {
        return; // no-op
    }
    already_initialized = 1;
    
    // existence of special file forces Asepsis disabling
    if (access(DISABLED_TWEAK_PATH, F_OK)==0) {
#if defined (DEBUG) // bail-out should be light-weight in non-debug mode
        asepsis_setup_logging();
        DLOG("disabled by %s", DISABLED_TWEAK_PATH);
#endif
        g_asepsis_disabled = 1;
        return; // minimize further interference
    }
}

// thread-safe version of asepsis_setup
// called first time when someone calls interposed functionality related to .DS_Store or when someone wants to log something
void asepsis_setup_safe(void) {
    pthread_mutex_lock(&g_asepsis_mutex);
    asepsis_setup();
    pthread_mutex_unlock(&g_asepsis_mutex);
}

int main(int argc, const char* argv[]) {
    // prevent multiple instances
    if (!acquireLock()) {
        ERROR("unable to acquire lock %s", DAEMON_LOCK_PATH);
        return EXIT_SUCCESS;
    }
    
    asepsis_setup();
    
    if (g_asepsis_disabled) {
        ERROR("disabled by %s", DISABLED_TWEAK_PATH);
        return EXIT_SUCCESS;
    }
    
    launchKext();

    DLOG("running monitor...");
    if (asepsis_run_monitor()==0) {
        DLOG("entering main loop...");
        CFRunLoopRun();
    }
    
    DLOG("releasing lock");
    releaseLock();
    
    return EXIT_SUCCESS;
}
