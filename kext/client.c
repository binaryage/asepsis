//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Asepsis.kext test client from BinaryAge
//
// some bits of code pasted from tcplognke sample:
//   http://developer.apple.com/mac/library/samplecode/tcplognke/Introduction/Intro.html
//

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "echelon.h"

static int gSocket = -1;

// SignalHandler - implemented to handle an interrupt from the command line using Ctrl-C.
static void SignalHandler(int sigraised) {
    if (gSocket > 0) {
        close (gSocket);
    }
    
    // exit(0) should not be called from a signal handler.  Use _exit(0) instead
    _exit(0);
}

int main(int argc, char* const* argv) {
    struct sockaddr_ctl sc;
    struct EchelonMessage em;
    struct ctl_info ctl_info;
    sig_t oldHandler;

    // set up a signal handler so we can clean up when we're interrupted from the command line
    // otherwise we stay in our run loop forever
    oldHandler = signal(SIGINT, SignalHandler);
    if (oldHandler == SIG_ERR)
        printf("Could not establish new signal handler");
    
    gSocket = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (gSocket < 0) {
        perror("socket SYSPROTO_CONTROL");
        exit(0);
    }
    bzero(&ctl_info, sizeof(struct ctl_info));
    strcpy(ctl_info.ctl_name, ECHELON_BUNDLE_ID);
    if (ioctl(gSocket, CTLIOCGINFO, &ctl_info) == -1) {
        perror("ioctl CTLIOCGINFO");
        exit(0);
    } else {
        printf("ctl_id: 0x%x for ctl_name: %s\n", ctl_info.ctl_id, ctl_info.ctl_name);
    }

    bzero(&sc, sizeof(struct sockaddr_ctl));
    sc.sc_len = sizeof(struct sockaddr_ctl);
    sc.sc_family = AF_SYSTEM;
    sc.ss_sysaddr = SYSPROTO_CONTROL;
    sc.sc_id = ctl_info.ctl_id;
    sc.sc_unit = 0;

    if (connect(gSocket, (struct sockaddr *)&sc, sizeof(struct sockaddr_ctl))) {
        perror("connect");
        exit(0);
    }
    
    // now, just wait for the spam
    printf("entering the listening loop ...\n");
    while (1) {
        if (recv(gSocket, &em, sizeof(struct EchelonMessage), 0) != sizeof(struct EchelonMessage)) {
            printf("malformed recv packet, leaving!\n");
            break;
        }
        
        if (em.operation==ECHELON_OP_RENAME) {
            printf("! %s -> %s\n", em.path1, em.path2);
        }
        if (em.operation==ECHELON_OP_DELETE) {
            printf("x %s\n", em.path1);
        }
    }
    
    close(gSocket);
    gSocket = -1;
    
    return 0;
}