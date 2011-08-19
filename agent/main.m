// read about Daemons and Agents here: http://developer.apple.com/library/mac/#technotes/tn2083

#include <IOKit/kext/KextManager.h>

#ifdef _DEBUG
#define DLOG(...) NSLog(__VA_ARGS__)
#else
#define DLOG(...)
#endif

void handle_SIGINT(int signum);

static bool launchKext() {
    // Either the calling process must have an effective user id of 0 (superuser),
    // or the kext being loaded and all its dependencies must reside
    // in /System and have an OSBundleAllowUserLoad property of true.
    // ---
    // we use OSBundleAllowUserLoad true case
    NSURL* kextPath = [NSURL fileURLWithPath:@"/System/Library/Extensions/Asepsis.kext"];
    
    KextManagerLoadKextWithURL((CFURLRef)kextPath, nil); // may fail because already loaded
    
    return true;
}

static int lock = 0;

static NSString* lockPath() {
    NSString* cachedLockPath = nil;
    
    if (!cachedLockPath) {
        cachedLockPath = [@"/tmp/.Asepsis.lock" stringByStandardizingPath];
        [cachedLockPath retain];
    }
    return cachedLockPath;
}

static bool acquireLock() {
    const char* path = [lockPath() fileSystemRepresentation];
    lock = open(path, O_CREAT|O_RDWR, S_IRWXU);
    if (flock(lock, LOCK_EX|LOCK_NB) != 0) {
        NSLog(@"Unable to obtain lock '%s' - exiting to prevent multiple CrashWatcher instances", path);
        close(lock);
        return false;
    }
    return true;
}

static void releaseLock() {
    if (!lock) return;
    flock(lock, LOCK_UN|LOCK_NB);
    close(lock);
    unlink([lockPath() fileSystemRepresentation]);
}

// =============================================================================
int main(int argc, const char* argv[]) {
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    // prevent multiple instances
    if (!acquireLock()) {
        [pool release];
        exit(1);
    }
    
    launchKext();
    
    int res = NSApplicationMain(argc,  (const char **) argv);
    
    releaseLock();
    [pool release];
    
    return res;
}
