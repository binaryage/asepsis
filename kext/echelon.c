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
#include <sys/vnode.h>

#include "echelon.h"

#define DEBUG_SWITCH 0

#if DEBUG_SWITCH
    #define DLOG(s, ...) printf(s, __VA_ARGS__)
#else
    #define DLOG(s, ...)
#endif

#define kInvalidUnit 0xFFFFFFFF

int strprefix(const char *s1, const char *s2); // strprefix is in libkern's export list, but not in any header <rdar://problem/4116835>.

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

int gLostMessagesLogging = 0;

SYSCTL_NODE(_debug, OID_AUTO, asepsis, CTLFLAG_RW, 0, "see http://asepsis.binaryage.com");
SYSCTL_INT(_debug_asepsis, OID_AUTO, lost_messages_logging, CTLFLAG_RW, &gLostMessagesLogging, 0, "logging of lost messages");

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
            DLOG("asepsis.kext: detected rename in blessed dir (ignoring): %s -> %s\n", (const char*) arg0, (const char*) arg1);
            return KAUTH_RESULT_DEFER; 
        }

        DLOG("asepsis.kext: detected rename: %s -> %s\n", (const char*) arg0, (const char*) arg1);

        vfs_context_t vfsctx = vfs_context_current();
        if (vfsctx) {
            vnode_t vp;
            errno_t err = vnode_lookup((const char*)arg1, 0, &vp, vfsctx);
            if (err) {
                DLOG("vnode lookup failed vfsctx=%p path=%s\n", vfsctx, (const char*)arg1);
                return KAUTH_RESULT_DEFER;
            }
            if (err==0 && vp) {
                int isDir = vnode_isdir(vp);
                vnode_put(vp);
                if (!isDir) {
                    DLOG("  ... not a directory => %s\n", "bail out");
                    return KAUTH_RESULT_DEFER;
                }
            }
        }

        struct EchelonMessage info;
        info.version = ECHELON_VERSION;
        info.operation = ECHELON_OP_RENAME;
        strncpy(info.path1, (const char*)arg0, ECHELON_PATH_MAX);
        strncpy(info.path2, (const char*)arg1, ECHELON_PATH_MAX);
        send_message(&info);
    } else { // KAUTH_FILEOP_DELETE
        if (!arg1) return KAUTH_RESULT_DEFER; // skip kernel bugs
        
        if (strprefix((const char*) arg1, ECHELON_DSCAGE_PREFIX)) { // ignore activity in blessed dir
            DLOG("asepsis.kext: detected delete in blessed dir (ignoring): %s\n", (const char*) arg1);
            return KAUTH_RESULT_DEFER; 
        }
        
        DLOG("asepsis.kext: detected delete: %s\n", (const char*) arg1);

        vnode_t vp = (vnode_t)arg0;
        if (vp) {
            int isDir = vnode_isdir(vp);
            if (!isDir) {
                DLOG("  ... not a directory => %s\n", "bail out");
                return KAUTH_RESULT_DEFER;
            }
        }
        
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
        printf("asepsis.kext: failed to create KAUTH listener.\n");
        lck_mtx_unlock(gLock);
        return KERN_FAILURE;
    }

    lck_mtx_unlock(gLock);
    return KERN_SUCCESS;
}

static errno_t ctl_connect(kern_ctl_ref ctl_ref, struct sockaddr_ctl* sac, void** unitinfo) {
    #pragma unused(unitinfo)

    lck_mtx_lock(gLock);

    printf("asepsis.kext: client connected (%d)\n", sac->sc_unit);
    
    if (gConnection) {
        printf("asepsis.kext: some client already connected, overriding the old connection %p", gConnection);
    }
    
    gConnection = ctl_ref;
    gUnit = sac->sc_unit;

    lck_mtx_unlock(gLock);
    return 0;
}

static errno_t ctl_disconnect(kern_ctl_ref ctl_ref, u_int32_t unit, void* unitinfo) {
    #pragma unused(unitinfo)

    lck_mtx_lock(gLock);

    printf("asepsis.kext: client disconnected (%d)\n", unit);

    if (!gConnection) {
        DLOG("asepsis.kext: have no connection, ignoring disconnect %s", "");
        lck_mtx_unlock(gLock);
        return 0;
    }
    
    if (gUnit!=unit) {
        DLOG("asepsis.kext: got different unit, keeping old connection %p", gConnection);
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
        printf("asepsis.kext: lost connection during operation\n");
        lck_mtx_unlock(gLock);
        return;
    }

    if (gUnit == kInvalidUnit) {
        printf("asepsis.kext: invalid unit for ctl_ref %p\n", gConnection);
        lck_mtx_unlock(gLock);
        return;
    }
    
    int res = ctl_enqueuedata(gConnection, gUnit, info, sizeof(struct EchelonMessage), 0);
    if (res) {
        // most likely out of socket buffer space
        if (gLostMessagesLogging) {
            printf("asepsis.kext: unable to send message, ctl_enqueuedata failed %d, message lost: %s\n", res, info->path1);
        }
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

    printf("asepsis.kext v%s\n", ECHELON_RELEASE_VERSION);

    DLOG("asepsis.kext: %s!\n", "start");

    // allocate our global resources, needed in order to allocate memory 
    // and locks throughout the rest of the program.
    err = KERN_SUCCESS;
    gMallocTag = OSMalloc_Tagalloc(ECHELON_BUNDLE_ID, OSMT_DEFAULT);
    if (!gMallocTag) {
        printf("asepsis.kext: failed to alloc tag (%d)\n", err);
        err = KERN_FAILURE;
    }
    if (err == KERN_SUCCESS) {
        gLockGroup = lck_grp_alloc_init(ECHELON_BUNDLE_ID, LCK_GRP_ATTR_NULL);
        if (!gLockGroup) {
            printf("asepsis.kext: failed to alloc lock group (%d)\n", err);
            err = KERN_FAILURE;
        }
    }
    
    // allocate the lock that protects our configuration.
    if (err == KERN_SUCCESS) {
        gLock = lck_mtx_alloc_init(gLockGroup, LCK_ATTR_NULL);
        if (!gLock) {
            printf("asepsis.kext: failed to alloc config mutex (%d)\n", err);
            err = KERN_FAILURE;
        }
    }

    // register our sysctl handlers
    if (err == KERN_SUCCESS) {
        sysctl_register_oid(&sysctl__debug_asepsis);
        sysctl_register_oid(&sysctl__debug_asepsis_lost_messages_logging);
        gRegisteredOID = TRUE;
    }

    // register our control structure so that we can be found by a user level process
    if (err == KERN_SUCCESS) {
        err = ctl_register(&gCtlReg, &gCtlRef);
        if (err == KERN_SUCCESS) {
            DLOG("asepsis.kext: registered ctl with id 0x%x, ref %p\n", gCtlReg.ctl_id, gCtlRef);
        } else {
            printf("asepsis.kext: ctl_register returned error %d\n", err);
            err = KERN_FAILURE;
        }
    }
    
    if (err == KERN_SUCCESS) {
        err = install_listener();
    }
    
    // if we failed, shut everything down.
    if (err != KERN_SUCCESS) {
        printf("asepsis.kext: failed to initialize with error %d\n", err);
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
            printf("asepsis.kext: cannot unload kext, the client is still connected (%d)\n", res);
            return KERN_FAILURE; // prevent unloading when client is still connected
        }
        gCtlRef = NULL;
    }

    if (gRegisteredOID) {
        sysctl_unregister_oid(&sysctl__debug_asepsis_lost_messages_logging);
        sysctl_unregister_oid(&sysctl__debug_asepsis);
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
    DLOG("asepsis.kext: %s!\n", "finish");
    return KERN_SUCCESS;
}