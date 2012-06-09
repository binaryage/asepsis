#include <syslog.h>
#include <asl.h>
#include <pthread.h>

extern aslclient g_asepsis_asl;
extern aslmsg g_asepsis_log_msg;
extern pthread_mutex_t g_asepsis_mutex;

#if defined(DEBUG)
#  define DLOG(...) asepsis_setup_safe(), asepsis_setup_logging(), pthread_mutex_lock(&g_asepsis_mutex), asl_log(g_asepsis_asl, g_asepsis_log_msg, ASL_LEVEL_NOTICE, __VA_ARGS__), printf(__VA_ARGS__), pthread_mutex_unlock(&g_asepsis_mutex)
#else
#  define DLOG(s, ...)
#endif

#define ERROR(...) asepsis_setup_safe(), asepsis_setup_logging(), pthread_mutex_lock(&g_asepsis_mutex), asl_log(g_asepsis_asl, g_asepsis_log_msg, ASL_LEVEL_ERR, __VA_ARGS__), printf(__VA_ARGS__), pthread_mutex_unlock(&g_asepsis_mutex)
#define INFO(...) asepsis_setup_safe(), asepsis_setup_logging(), pthread_mutex_lock(&g_asepsis_mutex), asl_log(g_asepsis_asl, g_asepsis_log_msg, ASL_LEVEL_NOTICE, __VA_ARGS__), printf(__VA_ARGS__), pthread_mutex_unlock(&g_asepsis_mutex)
