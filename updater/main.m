#import "main.h"
#import <Sparkle/Sparkle.h>
#import "../sparkle/SUUpdateAlert.h"
#import "../sparkle/SUUIBasedUpdateDriver.h"

#ifdef DEBUG
# define DLOG(...) NSLog(__VA_ARGS__)
#else
# define DLOG(...)
#endif

void exitApp() {
    DLOG(@"exitApp");
    [NSApp terminate:nil];
}

@interface AUpdateDriver : SUUIBasedUpdateDriver {
@private
}

@end

@implementation AUpdateDriver

- (void)didFindValidUpdate {
    DLOG(@"didFindValidUpdate");
	updateAlert = [[SUUpdateAlert alloc] initWithAppcastItem:updateItem host:host];
	[updateAlert setDelegate:self];
	
	id<SUVersionDisplay>	versDisp = nil;
	if ([[updater delegate] respondsToSelector:@selector(versionDisplayerForUpdater:)])
		versDisp = [[updater delegate] versionDisplayerForUpdater: updater];
	[updateAlert setVersionDisplayer: versDisp];
	
	if ([[updater delegate] respondsToSelector:@selector(updater:didFindValidUpdate:)])
		[[updater delegate] updater:updater didFindValidUpdate:updateItem];
    
	// If the app is a menubar app or the like, we need to focus it first and alter the
	// update prompt to behave like a normal window. Otherwise if the window were hidden
	// there may be no way for the application to be activated to make it visible again.
    [[updateAlert window] setHidesOnDeactivate:NO];
    [NSApp activateIgnoringOtherApps:YES];
	[[updateAlert window] makeKeyAndOrderFront:self];
}

- (void)didNotFindUpdate {
    DLOG(@"didNotFindUpdate");
	[self abortUpdate]; // Don't tell the user that no update was found; this was a scheduled update.
}

- (void)abortUpdateWithError:(NSError *)error {
    DLOG(@"abortUpdateWithError %@", error);
    [super abortUpdateWithError:error];
}

- (void)abortUpdate {
    DLOG(@"abortUpdate");
    [super abortUpdate];
    exitApp();
}

@end

@interface AUpdater : SUUpdater { }

+(id)sharedUpdater;
-(id)init;

@end

@implementation AUpdater

+(id) sharedUpdater {
    return [self updaterForBundle:[NSBundle bundleForClass:[self class]]];
}

-(id) init {
    return [self initForBundle:[NSBundle bundleForClass:[self class]]];
}

- (void)checkForUpdatesInBackground {
	// Background update checks should only happen if we have a network connection.
	//	Wouldn't want to annoy users on dial-up by establishing a connection every
	//	hour or so:
	SUUpdateDriver*	theUpdateDriver = [[[AUpdateDriver alloc] initWithUpdater:self] autorelease];
	
	[NSThread detachNewThreadSelector: @selector(checkForUpdatesInBgReachabilityCheckWithDriver:) toTarget: self withObject: theUpdateDriver];
}

@end

@interface AppDelegate : NSObject
- (void) applicationWillFinishLaunching: (NSNotification *)notification;
@end

@implementation AppDelegate
- (void) applicationWillFinishLaunching: (NSNotification *)notification {
    DLOG(@"applicationWillFinishLaunching %@", notification);
    AUpdater* sparkle = [AUpdater sharedUpdater];
    [sparkle checkForUpdatesInBackground];
}
@end

static volatile BOOL caughtSIGINT = NO;
void handle_SIGINT(int signum) {
    caughtSIGINT = YES;
    CFRunLoopStop(CFRunLoopGetCurrent());
}

// =============================================================================
int main(int argc, const char* argv[]) {
    signal(SIGINT, handle_SIGINT);
    [NSApplication sharedApplication];
    [NSApp setDelegate: [AppDelegate new]];
    return NSApplicationMain(argc, argv);
//    
//    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
//    DLOG(@"AsepsisUpdater launched, argc=%d", argc);
//
//
//    NSApplicationLoad();
//    AUpdater* sparkle = [AUpdater sharedUpdater];
//    [sparkle checkForUpdatesInBackground];
//    
//    NSRunLoop* runLoop = [NSRunLoop mainRunLoop];
//    
//    [runLoop run];
//    
//    if (caughtSIGINT) {
//        NSLog(@"caught SIGINT - exiting...");
//    }
//    [pool drain];
//
//    return 0;
}
