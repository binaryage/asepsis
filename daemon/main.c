// based on Amith Singh's http://osxbook.com/software/fslogger
// read about Daemons and Agents here: http://developer.apple.com/library/mac/#technotes/tn2083

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <pthread.h>
#include <pwd.h>
#include <grp.h>
#include <ftw.h>

#include "fsevents.h"
#include "common.h"
#include "logging.h"

#include "shared.h" 

#define PROGNAME "asepsisd"
#define DEV_FSEVENTS "/dev/fsevents" // the fsevents pseudo-device
#define FSEVENT_BUFSIZ 131072          // buffer for reading from the device
#define EVENT_QUEUE_SIZE 2048            // limited by MAX_KFS_EVENTS
#define KFS_NUM_ARGS FSE_MAX_ARGS
#define VTYPE_MAX (sizeof(vtypeNames)/sizeof(char *))
#define VERBOSE(...) if (verbose) printf(__VA_ARGS__)
#define DAEMON_LOCK_PATH "/usr/local/.dscage/.asepsis.daemon.lock"

#pragma pack(1) // http://en.wikipedia.org/wiki/Data_structure_alignment

static char* format_binary(unsigned long x);

int g_asepsis_disabled = 0;
pthread_mutex_t g_asepsis_mutex;
pthread_mutexattr_t g_asepsis_mutex_attr;
aslclient g_asepsis_asl = NULL;
aslmsg g_asepsis_log_msg = NULL;

static int g_single_instance_lock = 0;

pthread_mutex_t g_commands_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_data_available = PTHREAD_COND_INITIALIZER;

#define COMMAND_OP_UNKNOWN -1
#define COMMAND_OP_RENAME 0
#define COMMAND_OP_DELETE 1

typedef struct command {
    char operation;
    char  path1[PATH_MAX];
    char  path2[PATH_MAX];
} command_t;

#define COMMANDS_BUFFER_SIZE 1024

command_t g_command_buffer[COMMANDS_BUFFER_SIZE];
int g_number_of_commands_in_buffer = 0;
int g_front_command =0;
int g_back_command=0;

// an event argument
typedef struct kfs_event_arg {
    u_int16_t  type;         // argument type
    u_int16_t  len;          // size of argument data that follows this field
    union {
        struct vnode *vp;
        char         *str;
        void         *ptr;
        int32_t       int32;
        int64_t       int64;
        dev_t         dev;
        ino_t         ino;
        int32_t       mode;
        uid_t         uid;
        gid_t         gid;
    } data;
} kfs_event_arg_t;

// an event
typedef struct kfs_event {
    int32_t         type; // event type
    pid_t           pid;  // pid of the process that performed the operation
    kfs_event_arg_t args[KFS_NUM_ARGS]; // event arguments
} kfs_event;

// event names
static const char *kfseNames[FSE_MAX_EVENTS] = {
    "FSE_CREATE_FILE",
    "FSE_DELETE",
    "FSE_STAT_CHANGED",
    "FSE_RENAME",
    "FSE_CONTENT_MODIFIED",
    "FSE_EXCHANGE",
    "FSE_FINDER_INFO_CHANGED",
    "FSE_CREATE_DIR",
    "FSE_CHOWN",
    "FSE_XATTR_MODIFIED",
    "FSE_XATTR_REMOVED",
};

// argument names
static const char *kfseArgNames[] = {
    "FSE_ARG_UNKNOWN", "FSE_ARG_VNODE", "FSE_ARG_STRING", "FSE_ARGPATH",
    "FSE_ARG_INT32",   "FSE_ARG_INT64", "FSE_ARG_RAW",    "FSE_ARG_INO",
    "FSE_ARG_UID",     "FSE_ARG_DEV",   "FSE_ARG_MODE",   "FSE_ARG_GID",
    "FSE_ARG_FINFO",
};

// for pretty-printing of vnode types
enum vtype {
    VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD, VSTR, VCPLX
};

enum vtype iftovt_tab[] = {
    VNON, VFIFO, VCHR, VNON, VDIR,  VNON, VBLK, VNON,
    VREG, VNON,  VLNK, VNON, VSOCK, VNON, VNON, VBAD,
};

static const char *vtypeNames[] = {
    "VNON",  "VREG",  "VDIR", "VBLK", "VCHR", "VLNK",
    "VSOCK", "VFIFO", "VBAD", "VSTR", "VCPLX",
};

static int acquireLock() {
    g_single_instance_lock = open(DAEMON_LOCK_PATH, O_CREAT|O_RDWR, S_IRWXU);
    if (flock(g_single_instance_lock, LOCK_EX|LOCK_NB) != 0) {
        close(g_single_instance_lock);
        return 0;
    }
    return 1;
}

static void releaseLock() {
    if (!g_single_instance_lock) return;
    flock(g_single_instance_lock, LOCK_UN);
    close(g_single_instance_lock);
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
        DLOG("Disabled by %s", DISABLED_TWEAK_PATH);
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

static void signal_handler_usr1(int sigraised) {
    DLOG("USR1 signal - %d", sigraised);
    _exit(0); // exit(0) should not be called from a signal handler.  Use _exit(0) instead
}

static char* get_proc_name(pid_t pid) {
    size_t        len = sizeof(struct kinfo_proc);
    static int    name[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, 0 };
    static struct kinfo_proc kp;
    
    name[3] = pid;
    
    kp.kp_proc.p_comm[0] = '\0';
    if (sysctl((int *)name, sizeof(name)/sizeof(*name), &kp, &len, NULL, 0))
        return "?";
    
    if (kp.kp_proc.p_comm[0] == '\0')
        return "exited?";
    
    return kp.kp_proc.p_comm;
}

char* format_binary(unsigned long x) {
    const int maxlen = 8;
    const int maxcnt = 4;
    static char fmtbuf[(maxlen+1)*maxcnt];
    static int count = 0;
    char *b;
    count++;
    count = count % maxcnt;
    b = &fmtbuf[(maxlen+1)*count];
    b[maxlen] = '\0';
    for (int z = 0; z < maxlen; z++) { b[maxlen-1-z] = ((x>>z) & 0x1) ? '1' : '0'; }
    return b;
}

// http://stackoverflow.com/questions/3184445/how-to-clear-directory-contents-in-c-on-linux-basically-i-want-to-do-rm-rf
static int rmdir_recursive_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    DLOG("remove %s\n", fpath);
    int res = remove(fpath);
    if (res) {
        ERROR("unable to remove %s (%s)\n", fpath, strerror(errno));
    }
    return res;
}

static int rmdir_recursive(const char *path) {
    struct stat sb;
    
    // check full path for presence
    if (stat(path, &sb) == 0) {
        if (S_ISDIR (sb.st_mode)) {
            DLOG("rmdir_recursive %s\n", path);
            return nftw(path, rmdir_recursive_cb, 64, FTW_DEPTH | FTW_PHYS);
        } else {
            DLOG("remove %s\n", path);
            int res = remove(path); // in case path is just a file path
            if (res) {
                ERROR("unable to remove %s (%s)\n", path, strerror(errno));
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
    
    if (!isDirectory(prefixPath1)) {
        // nothing to do, source dir is not present in our cache
        return 0;
    }
    
    // do mirror remove
    DLOG("handle delete %s\n", prefixPath1);
    res = rmdir_recursive(prefixPath1); // this is dangerous, but we know that our path has prefix at least
    if (res) {
        ERROR("unable to remove directory %s (%s)\n", prefixPath1, strerror(errno));
    }
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
    
    if (!isDirectory(prefixPath1)) {
        // nothing to do, source dir is not present in our cache
        return 0;
    }
    
    // do mirror rename
    DLOG("handle rename %s -> %s\n", prefixPath1, prefixPath2);
    
    // here we need to be especially careful
    // rename(2) may fail under some special circumstances
    // 1. something gets into the way of renaming
    // 2. parent folder is missing for new name
    rmdir_recursive(prefixPath2); // there may be some old content (pathological case)
    ensureDirectoryForPath(prefixPath2); // like mkdir -p $prefixPath2 (but excluding last directory)
    
    // ok, now it should be safe to call rename
    res = rename(prefixPath1, prefixPath2); // this is dangerous, but we know that our path has prefix at least
    if (res) {
        ERROR("unable to rename directories %s -> %s (%s)\n", prefixPath1, prefixPath2, strerror(errno));
    }
    return res;
}

static void enqueue_command(command_t cmd);
static command_t fetch_command();
static void* consumer();

void enqueue_command(command_t cmd) {
    g_command_buffer[g_back_command] = cmd;
    g_back_command = (g_back_command+1) % COMMANDS_BUFFER_SIZE;
    g_number_of_commands_in_buffer++;
}

command_t fetch_command() {
    command_t cmd;
    cmd = g_command_buffer[g_front_command];
    g_front_command = (g_front_command+1) % COMMANDS_BUFFER_SIZE;
    g_number_of_commands_in_buffer--;
    return cmd;
}

// this is a worker thread responsible for consuming commands produced by the main thread
void* consumer() {
    DLOG("Consumer: started\n");
    command_t cmd;
    while (1) {
        pthread_mutex_lock(&g_commands_mutex);
        if (g_number_of_commands_in_buffer == 0) {
            DLOG("Consumer: waiting for commands...\n");
            pthread_cond_wait(&g_data_available, &g_commands_mutex); 
        }
        cmd = fetch_command();
        pthread_mutex_unlock(&g_commands_mutex);

        if (cmd.operation == COMMAND_OP_RENAME) {
            DLOG("Consumer: received rename op %s -> %s\n", cmd.path1, cmd.path2);
            asepsis_handle_rename(cmd.path1, cmd.path2);
        } else if (cmd.operation == COMMAND_OP_DELETE) {
            DLOG("Consumer: received delete op %s\n", cmd.path1);
            asepsis_handle_delete(cmd.path1);
        }
    }
}

// the main thread is fs events listener and commands producer
int main(int argc, const char* argv[]) {
    int32_t arg_id;
    int     fd, clonefd = -1;
    int     i, j, eoff, off, ret;
    int verbose = 0;
    
    kfs_event_arg_t *kea;
    struct           fsevent_clone_args fca;
    char             buffer[FSEVENT_BUFSIZ];
    struct passwd   *p;
    struct group    *g;
    mode_t           va_mode;
    u_int32_t        va_type;
    u_int32_t        is_fse_arg_vnode = 0;
    char             fileModeString[11 + 1];
    int8_t           event_list[] = { // action to take for each event
        FSE_IGNORE,  // FSE_CREATE_FILE
        FSE_REPORT,  // FSE_DELETE
        FSE_IGNORE,  // FSE_STAT_CHANGED
        FSE_REPORT,  // FSE_RENAME
        FSE_IGNORE,  // FSE_CONTENT_MODIFIED
        FSE_IGNORE,  // FSE_EXCHANGE
        FSE_IGNORE,  // FSE_FINDER_INFO_CHANGED
        FSE_IGNORE,  // FSE_CREATE_DIR
        FSE_IGNORE,  // FSE_CHOWN
    };
    
    if (geteuid() != 0) {
        ERROR("You must be root to run %s. Try again using 'sudo'.\n", PROGNAME);
        exit(1);
    }
    
    if (argc==2 && strcmp(argv[1], "-v")==0) {
        verbose = 1;
    }
    
    INFO("starting asepsisd v##VERSION##\n");
    
    // prevent multiple instances
    if (!acquireLock()) {
        ERROR("Unable to acquire lock %s", DAEMON_LOCK_PATH);
        return EXIT_SUCCESS;
    }
    
    asepsis_setup();
    
    if (g_asepsis_disabled) {
        ERROR("disabled by %s", DISABLED_TWEAK_PATH);
        return EXIT_SUCCESS;
    }
    
    // this is for controlling echelon connection from outside, handy during install/uninstall
    signal(SIGUSR1, signal_handler_usr1);
    
    if ((fd = open(DEV_FSEVENTS, O_RDONLY)) < 0) {
        ERROR("Call to open failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    
    fca.event_list = (int8_t *)event_list;
    fca.num_events = sizeof(event_list)/sizeof(int8_t);
    fca.event_queue_depth = EVENT_QUEUE_SIZE;
    fca.fd = &clonefd; 
    if ((ret = ioctl(fd, FSEVENTS_CLONE, (char *)&fca)) < 0) {
        ERROR("Call to ioctl failed: %s", strerror(errno));
        close(fd);
        return EXIT_FAILURE;
    }
    
    close(fd);

    // run the consumer
    pthread_t consumer_thread; 
    pthread_create(&consumer_thread, NULL, consumer, NULL);
    
    DLOG("Listening to fsevents... (fd=%d)\n", clonefd);
    while (1) { // event-processing loop
      
        ssize_t bytes = read(clonefd, buffer, FSEVENT_BUFSIZ);
        if (bytes > 0) VERBOSE("=> received %ld bytes\n", bytes);
        
        off = 0;
        
        while (off < bytes) { // process one or more events received
            command_t cmd;
            cmd.operation = COMMAND_OP_UNKNOWN;
            cmd.path1[0] = 0;
            cmd.path2[0] = 0;
            
            struct kfs_event *kfse = (struct kfs_event *)((char *)buffer + off);
            
            off += sizeof(int32_t) + sizeof(pid_t); // type + pid
            
            if (kfse->type == FSE_EVENTS_DROPPED) { // special event
                VERBOSE("# Event\n");
                VERBOSE("  %-14s = %s\n", "type", "EVENTS DROPPED");
                VERBOSE("  %-14s = %d\n", "pid", kfse->pid);
                off += sizeof(u_int16_t); // FSE_ARG_DONE: sizeof(type)
                ERROR("some events dropped!");
                continue;
            }
            
            int32_t type = kfse->type & FSE_TYPE_MASK;
            uint16_t flags = FSE_GET_FLAGS(kfse->type);
            if (type < FSE_MAX_EVENTS) {
                VERBOSE("# Event\n");
                VERBOSE("  %-14s = %s (flags=%s)\n", "type", kfseNames[type], format_binary(flags));
            } else { // should never happen
                ERROR("This may be a program bug (type=%s, flags=%s).\n", format_binary(type), format_binary(flags));
                return EXIT_FAILURE;
            }
            
            VERBOSE("  %-14s = %d (%s)\n", "pid", kfse->pid, get_proc_name(kfse->pid));
            VERBOSE("  # Details\n    # %-14s%4s  %s\n", "type", "len", "data");
            
            kea = kfse->args; 
            i = 0;
            
            if (type==FSE_DELETE) {
                cmd.operation = COMMAND_OP_DELETE;
            }
            if (type==FSE_RENAME) {
                cmd.operation = COMMAND_OP_RENAME;
            }
            
            char dir = 0;
            
            while (1) { // process arguments
                i++;
                
                if (kea->type == FSE_ARG_DONE) { // no more arguments
                    VERBOSE("    %s (%#x)\n", "FSE_ARG_DONE", kea->type);
                    off += sizeof(u_int16_t);
                    break;
                }
                
                eoff = sizeof(kea->type) + sizeof(kea->len) + kea->len;
                off += eoff;
                
                arg_id = (kea->type > FSE_MAX_ARGS) ? 0 : kea->type;
                VERBOSE("    %-16s%4hd ", kfseArgNames[arg_id], kea->len);
                
                switch (kea->type) { // handle based on argument type
                        
                    case FSE_ARG_VNODE:  // a vnode (string) pointer
                        is_fse_arg_vnode = 1;
                        VERBOSE("%-6s = %s\n", "path", (char *)&(kea->data.vp));
                        break;
                        
                    case FSE_ARG_STRING: // a string pointer
                        VERBOSE("%-6s = %s\n", "string", (char *)&(kea->data.str));
                        if (cmd.path1[0]==0) {
                            strncpy(cmd.path1, (char *)&(kea->data.str), PATH_MAX);
                        } else {
                            strncpy(cmd.path2, (char *)&(kea->data.str), PATH_MAX);
                        }
                        break;
                        
                    case FSE_ARG_INT32:
                        VERBOSE("%-6s = %d\n", "int32", kea->data.int32);
                        break;
                        
                    case FSE_ARG_INT64:
                        VERBOSE("%-6s = %lld\n", "int64", kea->data.int64);
                        break;
                        
                    case FSE_ARG_RAW: // a void pointer
                        VERBOSE("%-6s = ", "ptr");
                        for (j = 0; j < kea->len; j++)
                            VERBOSE("%02x ", ((char *)kea->data.ptr)[j]);
                        VERBOSE("\n");
                        break;
                        
                    case FSE_ARG_INO: // an inode number
                        VERBOSE("%-6s = %u\n", "ino", (unsigned int)kea->data.ino);
                        break;
                        
                    case FSE_ARG_UID: // a user ID
                        p = getpwuid(kea->data.uid);
                        VERBOSE("%-6s = %d (%s)\n", "uid", kea->data.uid, (p) ? p->pw_name : "?");
                        break;
                        
                    case FSE_ARG_DEV: // a file system ID or a device number
                        if (is_fse_arg_vnode) {
                            VERBOSE("%-6s = %#08x\n", "fsid", kea->data.dev);
                            is_fse_arg_vnode = 0;
                        } else {
                            VERBOSE("%-6s = %#08x (major %u, minor %u)\n", "dev", kea->data.dev, major(kea->data.dev), minor(kea->data.dev));
                        }
                        break;
                        
                    case FSE_ARG_MODE: // a combination of file mode and file type
                        va_mode = (kea->data.mode & 0x0000ffff);
                        va_type = (kea->data.mode & 0xfffff000);
                        if (va_type&S_IFDIR) {
                            dir = 1;
                        }
                        strmode(va_mode, fileModeString);
                        va_type = iftovt_tab[(va_type & S_IFMT) >> 12];
                        VERBOSE("%-6s = %s (%#08x, vnode type %s)\n", "mode", fileModeString, kea->data.mode, (va_type < VTYPE_MAX) ?  vtypeNames[va_type] : "?");
                        break;
                        
                    case FSE_ARG_GID: // a group ID
                        g = getgrgid(kea->data.gid);
                        VERBOSE("%-6s = %d (%s)\n", "gid", kea->data.gid, (g) ? g->gr_name : "?");
                        break;
                        
                    default:
                        VERBOSE("%-6s = ?\n", "unknown");
                        break;
                }
                
                kea = (kfs_event_arg_t *)((char *)kea + eoff); // next
            } // for each argument
            
            // enqueue recognized command for our consumer
            if (dir) { // we are only interested in directory operations
                if (cmd.operation == COMMAND_OP_RENAME || cmd.operation == COMMAND_OP_DELETE) {
                    pthread_mutex_lock(&g_commands_mutex);
                    if (g_number_of_commands_in_buffer < COMMANDS_BUFFER_SIZE) {
                        enqueue_command(cmd);
                        pthread_cond_signal(&g_data_available);
                    } else {
                        // drop events in this case
                        ERROR("Dropped a command. COMMANDS_BUFFER_SIZE is too low. [%s]\n", cmd.path1);
                    }
                    pthread_mutex_unlock(&g_commands_mutex);
                } else {
                    ERROR("Received unknown command (type=%s, flags=%s).\n", format_binary(type), format_binary(flags));
                }
            }
        } // for each event
    } // forever
    
    close(clonefd);
    
    DLOG("releasing lock");
    releaseLock();
    
    return EXIT_SUCCESS;
}
