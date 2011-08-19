//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Asepsis.kext from BinaryAge (Echelon)
//   a kernel-level file renaming watcher for Asepsis
//   more: http://blog.binaryage.com/totalfinder-alpha
//
// based on KAuthORama sample by Apple:
//   http://developer.apple.com/mac/library/samplecode/KauthORama/index.html
//   http://developer.apple.com/mac/library/technotes/tn2005/tn2127.html
//

#include <libkern/libkern.h>
#include <libkern/OSMalloc.h>
#include <sys/sysctl.h>
#include <sys/kauth.h>
#include <sys/kern_control.h>

#include "echelon.h"

#define DEBUG_SWITCH 0

#if DEBUG_SWITCH
    #define DLOG(s, ...) printf(s, __VA_ARGS__)
#else
    #define DLOG(s, ...)
#endif

#define kInvalidUnit 0xFFFFFFFF

int strprefix(const char *s1, const char *s2); // strprefix is in libkern's export list, but not in any header <rdar://problem/4116835>.

static int sys_ctl_handler(struct sysctl_oid * oidp, void* arg1, int arg2, struct sysctl_req * req);
static void send_message(struct EchelonMessage* info);
static errno_t ctl_connect(kern_ctl_ref ctl_ref, struct sockaddr_ctl *sac, void **unitinfo);
static errno_t ctl_disconnect(kern_ctl_ref ctl_ref, u_int32_t unit, void *unitinfo);

static OSMallocTag gMallocTag = NULL;        // our malloc tag
static lck_grp_t* gLockGroup = NULL;         // our lock group
static lck_mtx_t* gLock = NULL;              // concruency management for accessing global data
static kern_ctl_ref gConnection = 0;         // control reference to the connected process
static u_int32_t gUnit = 0;                  // unit number associated with the connected process
static kern_ctl_ref gCtlRef = NULL;
static boolean_t gRegisteredOID = FALSE;     // tracks whether we've registered our OID or not
static kauth_listener_t gFileOpListener = NULL;

// gConfiguration holds our current configuration string. It's modified by 
// sys_ctl_handler (well, by sysctl_handle_string which is called by sys_ctl_handler).
static char gConfiguration[1024];

// Declare our sysctl OID (that is, a variable that the user can 
// get and set using sysctl).  Once this OID is registered (which 
// is done in the start routine, echelon_start, below), the user 
// user can get and set our configuration variable (gConfiguration) 
// using the sysctl command line tool.
//
// We use OID using SYSCTL_OID rather than SYSCTL_STRING because 
// we want to override the hander function that's call (we want 
// sys_ctl_handler rather than sysctl_handle_string).
SYSCTL_OID(
    _kern,                                          // parent OID
    OID_AUTO,                                       // sysctl number, OID_AUTO means we're only accessible by name
    com_binaryage_echelon,                          // our name
    CTLTYPE_STRING | CTLFLAG_RW | CTLFLAG_KERN,     // we're a string, more or less
    gConfiguration,                                 // sysctl_handle_string gets/sets this string
    sizeof(gConfiguration),                         // and this is its maximum length
    sys_ctl_handler,                                // our handler 
    "A",                                            // because that's what SYSCTL_STRING does
    ""                                              // just a comment
);

// this is the new way to register a system control structure
// this is not a const structure since the ctl_id field will be set when the ctl_register call succeeds
static struct kern_ctl_reg gCtlReg = {
    ECHELON_BUNDLE_ID,             // use a reverse dns name which includes a name unique to your comany
    0,                      // set to 0 for dynamically assigned control ID - CTL_FLAG_REG_ID_UNIT not set
    0,                      // ctl_unit - ignored when CTL_FLAG_REG_ID_UNIT not set
    0,                      // privileged access required to access this filter
    0,                      // use default send size buffer
    0,                      // use default recv size buffer
    ctl_connect,            // Called when a connection request is accepted
    ctl_disconnect,         // called when a connection becomes disconnected
    NULL,                   // ctl_send_func - handles data sent from the client to kernel control
    NULL,                   // called when the user process makes the setsockopt call
    NULL                    // called when the user process makes the getsockopt call
};

static int fileop_listener(
    kauth_cred_t    credential,
    void *          idata,
    kauth_action_t  action,
    uintptr_t       arg0,
    uintptr_t       arg1,
    uintptr_t       arg2,
    uintptr_t       arg3
) {
    #pragma unused(idata)
    #pragma unused(action)
    #pragma unused(credential)
    #pragma unused(arg2)
    #pragma unused(arg3)

    if (action!=KAUTH_FILEOP_RENAME && action!=KAUTH_FILEOP_DELETE) return KAUTH_RESULT_DEFER; // interested only in renames and deletes

    lck_mtx_lock(gLock);
    boolean_t anyoneListening = !!gConnection;
    lck_mtx_unlock(gLock);
    
    if (!anyoneListening) return KAUTH_RESULT_DEFER; // nobody listening

    if (action==KAUTH_FILEOP_RENAME) {
        if (!arg0 || !arg1) return KAUTH_RESULT_DEFER; // skip kernel bugs

        if (strprefix((const char*) arg0, ECHELON_DSCAGE_PREFIX) || strprefix((const char*) arg1, ECHELON_DSCAGE_PREFIX)) { // ignore activity in blessed dir
            DLOG("echelon: detected rename in blessed dir (ignoring): %s -> %s\n", (const char*) arg0, (const char*) arg1);
            return KAUTH_RESULT_DEFER; 
        }

        DLOG("echelon: detected rename: %s -> %s\n", (const char*) arg0, (const char*) arg1);

        struct EchelonMessage info;
        info.version = ECHELON_VERSION;
        info.operation = ECHELON_OP_RENAME;
        strncpy(info.path1, (const char*)arg0, ECHELON_PATH_MAX);
        strncpy(info.path2, (const char*)arg1, ECHELON_PATH_MAX);
        send_message(&info);
    } else { // KAUTH_FILEOP_DELETE
        if (!arg1) return KAUTH_RESULT_DEFER; // skip kernel bugs
        
        if (strprefix((const char*) arg1, ECHELON_DSCAGE_PREFIX)) { // ignore activity in blessed dir
            DLOG("echelon: detected delete in blessed dir (ignoring): %s\n", (const char*) arg1);
            return KAUTH_RESULT_DEFER; 
        }
        
        DLOG("echelon: detected delete: %s\n", (const char*) arg1);
        
        struct EchelonMessage info;
        info.version = ECHELON_VERSION;
        info.operation = ECHELON_OP_DELETE;
        strncpy(info.path1, (const char*)arg1, ECHELON_PATH_MAX);
        send_message(&info);
    }

    return KAUTH_RESULT_DEFER;
}

static kern_return_t uninstall_listener(void) {
    lck_mtx_lock(gLock);

    if (gFileOpListener) {
        kauth_unlisten_scope(gFileOpListener);
        gFileOpListener = NULL;
    }

    lck_mtx_unlock(gLock);
    return KERN_SUCCESS;
}

static kern_return_t install_listener() {
    lck_mtx_lock(gLock);

    gFileOpListener = kauth_listen_scope(KAUTH_SCOPE_FILEOP, fileop_listener, NULL);
    if (!gFileOpListener) {
        printf("echelon: failed to create KAUTH listener.\n");
        lck_mtx_unlock(gLock);
        return KERN_FAILURE;
    }

    lck_mtx_unlock(gLock);
    return KERN_SUCCESS;
}

// This routine is called by the sysctl handler when it notices 
// that the configuration has changed.  It's responsible for 
// parsing the new configuration string and updating the listener.
//
// See sys_ctl_handler for a description of how I chose to handle the 
// failure case.
//
// This routine always runs under the gLock.
static void configure_echelon(const char *configuration) {
    printf("echelon: reconfiguring with %s\n", configuration);
    
    // parse configuration string and perform actions

    // nothing to do, maybe later ...
}

// this routine is called by the kernel when the user reads or writes our sysctl variable.  
// the arguments are standard for a sysctl handler.
static int sys_ctl_handler(
    struct sysctl_oid * oidp, 
    void *              arg1, 
    int                 arg2, 
    struct sysctl_req * req
) {
    int     result;
    
    // prevent two threads trying to change our configuration at the same time.
    lck_mtx_lock(gLock);
    
    // let sysctl_handle_string do all the heavy lifting of getting and setting the variable.
    result = sysctl_handle_string(oidp, arg1, arg2, req);
    
    // on the way out, if we got no error and a new value was set, do our magic.
    if ((result == 0) && req->newptr) {
        configure_echelon(gConfiguration);
    }
    
    lck_mtx_unlock(gLock);

    return result;
}

static errno_t ctl_connect(kern_ctl_ref ctl_ref, struct sockaddr_ctl* sac, void** unitinfo) {
    #pragma unused(unitinfo)

    lck_mtx_lock(gLock);

    printf("echelon: client connected (%d)\n", sac->sc_unit);
    
    if (gConnection) {
        printf("echelon: some client already connected, overriding the old connection %p", gConnection);
    }
    
    gConnection = ctl_ref;
    gUnit = sac->sc_unit;

    lck_mtx_unlock(gLock);
    return 0;
}

static errno_t ctl_disconnect(kern_ctl_ref ctl_ref, u_int32_t unit, void* unitinfo) {
    #pragma unused(unitinfo)

    lck_mtx_lock(gLock);

    printf("echelon: client disconnected (%d)\n", unit);

    if (!gConnection) {
        printf("echelon: have no connection, ignoring disconnect");
        lck_mtx_unlock(gLock);
        return 0;
    }
    
    if (gUnit!=unit) {
        printf("echelon: got different unit, keeping old connection %p", gConnection);
        lck_mtx_unlock(gLock);
        return 0;
    }
    
    gConnection = 0;
    gUnit = 0;

    lck_mtx_unlock(gLock);
    return 0;
}

// send message to listening client
static void send_message(struct EchelonMessage* info) {
    lck_mtx_lock(gLock);

    if (!gConnection) {
        printf("echelon: lost connection during operation\n");
        lck_mtx_unlock(gLock);
        return;
    }

    if (gUnit == kInvalidUnit) {
        printf("echelon: invalid unit for ctl_ref %p\n", gConnection);
        lck_mtx_unlock(gLock);
        return;
    }
    
    int res = ctl_enqueuedata(gConnection, gUnit, info, sizeof(struct EchelonMessage), 0);
    if (res) {
        // most likely out of socket buffer space
        printf("echelon: unable to send message, ctl_enqueuedata failed %d, disconnecting client\n", res);
        gConnection = 0;
        gUnit = 0;
    }

    lck_mtx_unlock(gLock);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// exported symbols visible by kernel
//

// prototypes for our entry points (because I've enabled Xcode's strict prototype checking)
extern kern_return_t echelon_start(kmod_info_t* ki, void* d);
extern kern_return_t echelon_stop(kmod_info_t* ki, void* d);

// ---------------------------------------------------------------------------------------------------------------------
// called by the system to start up the kext
extern kern_return_t echelon_start(kmod_info_t* ki, void* d) {
    #pragma unused(ki)
    #pragma unused(d)
    kern_return_t err;

    printf("echelon v%s\n", ECHELON_RELEASE_VERSION);

    printf("echelon: start!\n");

    // allocate our global resources, needed in order to allocate memory 
    // and locks throughout the rest of the program.
    err = KERN_SUCCESS;
    gMallocTag = OSMalloc_Tagalloc(ECHELON_BUNDLE_ID, OSMT_DEFAULT);
    if (!gMallocTag) {
        printf("echelon: failed to alloc tag (%d)\n", err);
        err = KERN_FAILURE;
    }
    if (err == KERN_SUCCESS) {
        gLockGroup = lck_grp_alloc_init(ECHELON_BUNDLE_ID, LCK_GRP_ATTR_NULL);
        if (!gLockGroup) {
            printf("echelon: failed to alloc lock group (%d)\n", err);
            err = KERN_FAILURE;
        }
    }
    
    // allocate the lock that protects our configuration.
    if (err == KERN_SUCCESS) {
        gLock = lck_mtx_alloc_init(gLockGroup, LCK_ATTR_NULL);
        if (!gLock) {
            printf("echelon: failed to alloc config mutex (%d)\n", err);
            err = KERN_FAILURE;
        }
    }

    // register our sysctl handler.
    if (err == KERN_SUCCESS) {
        sysctl_register_oid(&sysctl__kern_com_binaryage_echelon);
        gRegisteredOID = TRUE;
    }

    // register our control structure so that we can be found by a user level process
    if (err == KERN_SUCCESS) {
        err = ctl_register(&gCtlReg, &gCtlRef);
        if (err == KERN_SUCCESS) {
            DLOG("echelon: registered ctl with id 0x%x, ref %p\n", gCtlReg.ctl_id, gCtlRef);
        } else {
            printf("echelon: ctl_register returned error %d\n", err);
            err = KERN_FAILURE;
        }
    }
    
    if (err == KERN_SUCCESS) {
        err = install_listener();
    }
    
    // if we failed, shut everything down.
    if (err != KERN_SUCCESS) {
        printf("echelon: failed to initialize with error %d\n", err);
        (void) echelon_stop(ki, d);
    }

    return err;
}

// ---------------------------------------------------------------------------------------------------------------------
// called by the system to shut down the kext
extern kern_return_t echelon_stop(kmod_info_t* ki, void* d) {
    #pragma unused(ki)
    #pragma unused(d)

    if (gCtlRef) {
        errno_t res = ctl_deregister(gCtlRef);
        if (res) { // see http://lists.apple.com/archives/darwin-kernel/2005/Jul/msg00035.html
            printf("echelon: cannot unload kext, the client is still connected (%d)\n", res);
            return KERN_FAILURE; // prevent unloading when client is still connected
        }
        gCtlRef = NULL;
    }

    if (gRegisteredOID) {
        sysctl_unregister_oid(&sysctl__kern_com_binaryage_echelon);
        gRegisteredOID = FALSE;
    }

    uninstall_listener();
    
    if (gLock) {
        lck_mtx_free(gLock, gLockGroup);
        gLock = NULL;
    }
    
    if (gLockGroup) {
        lck_grp_free(gLockGroup);
        gLockGroup = NULL;
    }

    if (gMallocTag) {
        OSMalloc_Tagfree(gMallocTag);
        gMallocTag = NULL;
    }

    // and we're done.
    printf("echelon: finish!\n");
    return KERN_SUCCESS;
}