#if defined(DEBUG)
#  define DLOG(...) printf(__VA_ARGS__)
#else
#  define DLOG(s, ...)
#endif

#define ERROR(...) printf(__VA_ARGS__)