#ifdef __cplusplus
extern "C" {
#endif
    
    extern int initFSMonitor();
    extern bool isFSMonitorRunning();
    extern int isOK();
    extern int getFSMode();
    extern int setFSMode(int mode);
    
#ifdef __cplusplus
}
#endif
