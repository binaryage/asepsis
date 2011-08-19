#import "AgentScripting.h"

#define	scriptLoggingMasterSwitch	( 1 )

#if scriptLoggingMasterSwitch
#define SLOG(format,...) NSLog( @"SLOG: File=%s line=%d proc=%s " format, strrchr("/" __FILE__,'/')+1, __LINE__, __PRETTY_FUNCTION__, ## __VA_ARGS__ )
#else
#define SLOG(format,...)
#endif

@implementation NSApplication (AgentScripting)


/* kvc method for the 'ready' AppleScript property.
 
 note, since we have defined 'ready' as a read-only property in our
 scripting definition file, we only implement the getter method
 and have not implemented the setter method 'setReady:'. */
- (NSNumber*) ready {
    
    /* output to the log */
	SLOG(@"returning application's ready property");
	
    /* return always ready */
	return [NSNumber numberWithBool:YES];
}

@end
