#include <syslog.h>
#include <asl.h>
#include <pthread.h>

extern aslclient g_asepsis_asl;
extern aslmsg g_asepsis_log_msg;
extern char g_asepsis_executable_path[1024];
extern pthread_mutex_t g_asepsis_mutex;

#if defined(DEBUG)
#  define DLOG(s, ...) asepsis_setup_safe(), asepsis_setup_logging(), pthread_mutex_lock(&g_asepsis_mutex), asl_log(g_asepsis_asl, g_asepsis_log_msg, ASL_LEVEL_NOTICE, "[%s] " s, g_asepsis_executable_path, __VA_ARGS__), pthread_mutex_unlock(&g_asepsis_mutex)
#else
#  define DLOG(s, ...)
#endif

#define ERROR(s, ...) asepsis_setup_safe(), asepsis_setup_logging(), pthread_mutex_lock(&g_asepsis_mutex), asl_log(g_asepsis_asl, g_asepsis_log_msg, ASL_LEVEL_ERR, "[%s] " s, g_asepsis_executable_path, __VA_ARGS__), pthread_mutex_unlock(&g_asepsis_mutex)
