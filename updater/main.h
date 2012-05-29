#include <Foundation/Foundation.h>

@interface Reporter : NSObject {
    @public
    IBOutlet NSWindow* alertWindow_;      // The alert window

    IBOutlet NSTextField* dialogTitle_;
    IBOutlet NSTextField* dialogNote_;
    IBOutlet NSTextField* dialogExplanation_;
    IBOutlet NSTextField* commentMessage_;
    IBOutlet NSButton* sendButton_;
    IBOutlet NSButton* cancelButton_;
    IBOutlet NSTextField* countdownLabel_;
    IBOutlet NSProgressIndicator* progressIndicator_;

    // Text field bindings, for user input.
    NSString* countdownMessage_;           // Message indicating time left for input.
    NSString* targetApp_;                  // TotalTerminal or TotalFinder
    @private
    NSDictionary* parameters_;             // Key value pairs of data
    NSTimeInterval remainingDialogTime_;   // Keeps track of how long we have until we cancel the dialog
    NSTimer* messageTimer_;                // Timer we use to update the dialog
}

// Stops the modal panel with an NSAlertDefaultReturn value. This is the action
// invoked by the "Send Report" button.
-(IBAction)sendReport:(id)sender;
// Stops the modal panel with an NSAlertAlternateReturn value. This is the
// action invoked by the "Cancel" button.
-(IBAction)cancel:(id)sender;

-(NSString*)countdownMessage;
-(void)setCountdownMessage:(NSString*)value;

@end