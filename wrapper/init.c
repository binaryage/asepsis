#include <unistd.h>
#include <mach-o/dyld.h>
#include <string.h>
#include <pthread.h>

#include "common.h"
#include "logging.h"

int g_asepsis_disabled = 0;
pthread_mutex_t g_asepsis_mutex;
pthread_mutexattr_t g_asepsis_mutex_attr;
aslclient g_asepsis_asl = NULL;
aslmsg g_asepsis_log_msg = NULL;
uint32_t g_asepsis_executable_path_length = 1024;
char g_asepsis_executable_path[1024] = "!";

void asepsis_setup_safe(void);
void asepsis_setup_logging(void);

// here is a hardcoded list of white-listed processes - asepsis will disable itself for others
const char* g_asepsis_whitelist[] = {
    "/System/Library/CoreServices/Finder.app/Contents/MacOS/Finder",
    NULL
};

static int asepsis_process_is_whitelisted(const char* executable) {
    const char** path = g_asepsis_whitelist;
    while (*path) {
        // test if path is a prefix of executable
        if (strncmp(*path, executable, strlen(*path))==0) {
            return 1;
        }
        path++;
    }
    return 0;
}

// create a new ASL log
void asepsis_setup_logging(void) {
    static int asepsis_logging_initialized = 0;
    if (asepsis_logging_initialized) {
        return;
    }
    asepsis_logging_initialized = 1;
	g_asepsis_asl = asl_open("Asepsis", "dylib", 0);
	g_asepsis_log_msg = asl_new(ASL_TYPE_MSG);
	asl_set(g_asepsis_log_msg, ASL_KEY_SENDER, "Asepsis");
}

static void asepsis_setup_executable_path() {
    static int asepsis_executable_path_initialized = 0;
    if (asepsis_executable_path_initialized) {
        return;
    }
    asepsis_executable_path_initialized = 1;
    if (_NSGetExecutablePath(g_asepsis_executable_path, &g_asepsis_executable_path_length)) {
        // failed? ok, mockup some dummy value here
        strcpy(g_asepsis_executable_path, "?");
        g_asepsis_executable_path_length = 1;
#if defined (DEBUG)        
        asepsis_setup_logging();
        DLOG("unable to retrieve executable path %s", "");
#endif
    }
}

// this is called first time Asepsis is going to do some action
static void asepsis_setup(void) {
    // existence of special file forces Asepsis disabling
    if (access(DISABLED_TWEAK_PATH, F_OK)==0) {
#if defined (DEBUG) // bail-out should be light-weight in non-debug mode
        asepsis_setup_executable_path();
        asepsis_setup_logging();
        DLOG("disabled by %s", DISABLED_TWEAK_PATH);
#endif
        g_asepsis_disabled = 1;
        return; // minimize further interference
    }
    
    // retrieve host executable path
    asepsis_setup_executable_path();
    
    // disable if hosting executable is not white-listed
    if (!asepsis_process_is_whitelisted(g_asepsis_executable_path)) {
#if defined (DEBUG) // bail-out should be light-weight in non-debug mode      
        asepsis_setup_logging();
        DLOG("disabled because not on the white-list %s", "");
#endif
        g_asepsis_disabled = 1;
        return; // minimize further interference
    }
}

// thread-safe version of asepsis_setup
// called first time when someone calls interposed functionality related to .DS_Store or when someone wants to log something
void asepsis_setup_safe(void) {
    static int already_initialized = 0;
    if (already_initialized) {
        return; // no-op
    }
    already_initialized = 1;
    
    // init a recursive mutex
    pthread_mutexattr_init(&g_asepsis_mutex_attr);
    pthread_mutexattr_settype(&g_asepsis_mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_asepsis_mutex, &g_asepsis_mutex_attr);
    
    pthread_mutex_lock(&g_asepsis_mutex);
    asepsis_setup();
    pthread_mutex_unlock(&g_asepsis_mutex);
}