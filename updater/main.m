#import "main.h"
#import <Sparkle/Sparkle.h>
#import "../sparkle/SUUIBasedUpdateDriver.h"
#import "../sparkle/SUUpdateAlert.h"

#ifdef DEBUG
# define DLOG(...) NSLog(__VA_ARGS__)
#else
# define DLOG(...)
#endif

#define ASEPSIS_ISSUES_SUPPORT_PAGE @"http://asepsis.binaryage.com/#diagnose"

// -----------------------------------------------------------------------------

void handle_SIGINT(int signum) {
    [NSApp terminate:nil];
}

void exitApp() {
    DLOG(@"exitApp");
    [NSApp terminate:nil];
}

bool diagnoseAsepsis() {
    // http://stackoverflow.com/questions/412562/execute-a-terminal-command-from-a-cocoa-app
    
    NSTask *task;
    task = [[NSTask alloc] init];
    [task setLaunchPath: @"/usr/local/bin/asepsisctl"];
    
    NSArray *arguments;
    arguments = @[@"diagnose"];
    [task setArguments: arguments];
    
    NSPipe *pipe;
    pipe = [NSPipe pipe];
    [task setStandardOutput: pipe];
    
    NSFileHandle *file;
    file = [pipe fileHandleForReading];
    
    [task launch];
    [task waitUntilExit];
    
    NSData *data;
    data = [file readDataToEndOfFile];
    
    NSString *string;
    string = [[NSString alloc] initWithData: data encoding: NSUTF8StringEncoding];
    DLOG(@"asepsisctl diagnose returned:\n%@", string);

    bool res = [task terminationStatus]==0;
    return res;
}

void reportFailedDiagnose() {
    DLOG(@"reportFailedDiagnose");
    NSString* supressKey = @"SuppressAsepsisVersionCheck";
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    if ([defaults boolForKey:supressKey]) {
        return;
    }
    NSAlert* alert = [NSAlert new];
    [alert setMessageText: @"Asepsis could be broken"];
    [alert setInformativeText: @"Have you installed a system update recently?\nThe \"asepsisctl diagnose\" command reported some issues with your Asepsis installation."];
    [alert setShowsSuppressionButton:YES];
    [alert addButtonWithTitle:@"Read more on the web..."];
    [alert addButtonWithTitle:@"OK"];
    NSInteger res = [alert runModal];
    if ([[alert suppressionButton] state] == NSOnState) {
        [defaults setBool:YES forKey:supressKey];
    }
    if (res==NSAlertFirstButtonReturn) {
        // read more
        [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:ASEPSIS_ISSUES_SUPPORT_PAGE]];
    }
}

// -----------------------------------------------------------------------------

@interface AUpdateDriver : SUUIBasedUpdateDriver {
}

@end

@implementation AUpdateDriver

- (void)didFindValidUpdate {
    DLOG(@"didFindValidUpdate");
    updateAlert = [[SUUpdateAlert alloc] initWithAppcastItem:updateItem host:host];
    [updateAlert setDelegate:self];
    
    id<SUVersionDisplay>    versDisp = nil;
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
    NSString* suppressUpdateErrorsFlag = [@"~/.asepsis-suppress-update-errors" stringByStandardizingPath];
    BOOL suppressUpdateError = [[NSFileManager defaultManager] fileExistsAtPath:suppressUpdateErrorsFlag];
    if (!suppressUpdateError) {
        NSUserNotification* notification = [[NSUserNotification alloc] init];
        notification.title = [NSString stringWithFormat:@"Asepsis update check failed"];
        notification.informativeText = [error localizedDescription];
        [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];
    } else {
        DLOG(@"  => update error notification suppressed");
    }
    if (download)
        [download cancel];
    [self abortUpdate];
}

- (void)abortUpdate {
    DLOG(@"abortUpdate");
    [super abortUpdate];
    exitApp();
}

@end

// -----------------------------------------------------------------------------

@interface AUpdater : SUUpdater { }

+(id)sharedUpdater;
-(id)init;
- (void)checkForUpdatesInBackground;

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
    //    Wouldn't want to annoy users on dial-up by establishing a connection every
    //    hour or so:
    SUUpdateDriver*    theUpdateDriver = [[AUpdateDriver alloc] initWithUpdater:self];
    
    [NSThread detachNewThreadSelector: @selector(checkForUpdatesInBgReachabilityCheckWithDriver:) toTarget: self withObject: theUpdateDriver];
}

@end

// -----------------------------------------------------------------------------

@interface AppDelegate : NSObject
- (void) applicationWillFinishLaunching: (NSNotification *)notification;
@end

@implementation AppDelegate

- (void) applicationWillFinishLaunching: (NSNotification *)notification {
    DLOG(@"applicationWillFinishLaunching %@", notification);
    if (!diagnoseAsepsis()) {
        reportFailedDiagnose();
    }

    AUpdater* sparkle = [AUpdater sharedUpdater];
    // with every new release to be published it is recommended to test if Sparkle really works
    // publishing a version with broken Sparkle updater would be a real pain
    //
    // the idea:
    // 1. prepare asepsis-test.xml and upload it to http://updates.binaryage.com
    // 2. install new release on a test machine
    // 3. touch ~/.use-test-appcast
    // 4. check for updates and go through updater/installer
    //
    // asepsis-test.xml should contain current binary but with bumped version number, so it triggers and emulates updating to future version
    //
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSString* useTestAppCastPath = [@"~/.use-test-appcast" stringByStandardizingPath];

    BOOL useTest = [fileManager fileExistsAtPath:useTestAppCastPath];
    if (useTest) {
        NSLog(@"Using http://updates.binaryage.com/asepsis-test.xml as appcast because %@ is present.", useTestAppCastPath);
        [sparkle setFeedURL:[NSURL URLWithString:@"http://updates.binaryage.com/asepsis-test.xml"]];
    }

    [sparkle checkForUpdatesInBackground];
}

@end

// =============================================================================
int main(int argc, const char* argv[]) {
    static id delegate;
    delegate = [AppDelegate new];
    signal(SIGINT, handle_SIGINT);
    [NSApplication sharedApplication];
    [NSApp setDelegate: delegate];
    return NSApplicationMain(argc, argv);
}