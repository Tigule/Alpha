#import <Cocoa/Cocoa.h>

#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

const char *WOW_ERROR_SERVER_ADDRESS = "12.41.72.213";

static NSString *NoteError = @"WoWErrorNoteError";
static NSString *NoteInfo = @"WoWErrorNoteInfo";

@interface WEPrintStream : NSObject {
  unsigned int  m_address;
  short         m_port;
  int           m_socket;
  int           m_event;
  int           m_errorCode;
  unsigned char m_open;
  id            m_delegate;
}
- (id)initWithName:(const char *)name port:(int)port;
- (void)timerFired:(NSTimer *)timer;
- (NSError *)streamError;
- (void)setDelegate:(id)delegate;
- (void)open;
- (void)close;
- (int)write:(const unsigned char *)buffer maxLength:(unsigned int)len;
@end

@interface WoWErrorController : NSObject <NSApplicationDelegate, NSWindowDelegate> {
  NSTextView   *logTextView;
  NSTextView   *userDescriptionTextView;
  NSWindow     *window;
  NSImageView  *iconImageView;
  NSString     *logFile;
  NSData       *dataToSend;
  unsigned char sendComplete;
  unsigned char sending;
}
- (void)cancelAction:(id)sender;
- (void)compat_stream:(WEPrintStream *)stream handleEvent:(int)event;
- (NSData *)getDataToSend;
- (void)sendAction:(id)sender;
- (void)noteError:(NSNotification *)note;
- (void)noteInfo:(NSNotification *)note;
- (void)sheetDidEnd:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo;
@end

@implementation WEPrintStream

- (id)initWithName:(const char *)name port:(int)port {
  self = [super init];
  if (self) {
    struct hostent *host = gethostbyname(name);
    if (host) {
      unsigned char *addr = (unsigned char *)host->h_addr_list[0];
      m_address = (addr[0] << 24) | (addr[1] << 16) | (addr[2] << 8) | addr[3];
      m_port = port;
    } else {
      m_address = 0;
      m_port = 0;
    }
  }
  return self;
}

- (void)timerFired:(NSTimer *)timer {
  if (!m_open) {
    [timer invalidate];
    return;
  }

  if (m_event != NSStreamEventErrorOccurred) {
    struct timeval tv = {0, 0};
    fd_set         efds;
    fd_set         wfds;

    FD_ZERO(&efds);
    FD_ZERO(&wfds);
    FD_SET(m_socket, &efds);
    FD_SET(m_socket, &wfds);

    int result = select(m_socket + 1, NULL, &wfds, &efds, &tv);
    if (result < 0) {
      m_event = NSStreamEventErrorOccurred;
      m_errorCode = errno;
    } else if (result > 0) {
      if (FD_ISSET(m_socket, &efds)) {
        m_event = NSStreamEventErrorOccurred;
      } else if (FD_ISSET(m_socket, &wfds)) {
        m_event = NSStreamEventHasSpaceAvailable;
      }
    }
  }

  if (m_event == NSStreamEventErrorOccurred) {
    [timer invalidate];
  }

  [m_delegate compat_stream:self handleEvent:m_event];
}

- (NSError *)streamError {
  return [NSError errorWithDomain:[NSString stringWithString:@"com.blizzard.PosixError"] code:m_errorCode userInfo:nil];
}

- (void)setDelegate:(id)delegate {
  m_delegate = delegate;
}

- (void)open {
  struct sockaddr_in addr;

  m_event = NSStreamEventNone;
  m_socket = -1;

  if (!m_port || !m_address) {
    m_event = NSStreamEventErrorOccurred;
    m_errorCode = EBADF;
  } else {
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
  }

  if (m_socket < 0) {
    m_event = NSStreamEventErrorOccurred;
    m_errorCode = errno;
  } else {
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = htonl(m_address);
    if (connect(m_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0 && errno != EINPROGRESS) {
      m_event = NSStreamEventErrorOccurred;
      m_errorCode = errno;
      close(m_socket);
      m_socket = -1;
    }
  }

  if (m_event == NSStreamEventErrorOccurred) {
    m_open = 0;
    [m_delegate compat_stream:self handleEvent:NSStreamEventErrorOccurred];
  } else {
    m_open = 1;
    [m_delegate compat_stream:self handleEvent:NSStreamEventOpenCompleted];
    [NSTimer scheduledTimerWithTimeInterval:0.05 target:self selector:@selector(timerFired:) userInfo:nil repeats:YES];
  }
}

- (void)close {
  m_open = 0;
  if (m_socket > 0) {
    close(m_socket);
    m_socket = -1;
  }
}

- (int)write:(const unsigned char *)buffer maxLength:(unsigned int)len {
  int sent = send(m_socket, buffer, len, 0);
  if (sent < 0) {
    m_event = NSStreamEventErrorOccurred;
    m_errorCode = errno;
  }
  return sent;
}

@end

int main(int argc, const char *argv[]) {
  WoWErrorController *controller = [[WoWErrorController alloc] init];
  [controller awakeFromNib];
  return NSApplicationMain(argc, argv);
}

@implementation WoWErrorController

- (id)init {
  self = [super init];
  if (!self) {
    return nil;
  }

  NSApplication *app = [NSApplication sharedApplication];
  NSMenu        *mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
  NSMenu        *menu;
  NSMenu        *submenu;
  NSMenuItem    *item;

  menu = [[NSMenu alloc] initWithTitle:@"WoWError"];
  [menu addItemWithTitle:@"About WoWError..." action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
  [menu addItem:[NSMenuItem separatorItem]];
  [menu addItemWithTitle:@"Hide WoWError" action:@selector(hide:) keyEquivalent:@"h"];
  item = [menu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
  [item setKeyEquivalentModifierMask:NSEventModifierFlagCommand | NSEventModifierFlagOption];
  [menu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
  [menu addItem:[NSMenuItem separatorItem]];
  [menu addItemWithTitle:@"Quit WoWError" action:@selector(terminate:) keyEquivalent:@"q"];
  [[mainMenu addItemWithTitle:@"WoWError" action:NULL keyEquivalent:@""] setSubmenu:menu];
  [menu release];

  menu = [[NSMenu alloc] initWithTitle:@"Edit"];
  [menu addItemWithTitle:@"Undo" action:@selector(undo:) keyEquivalent:@"z"];
  [menu addItemWithTitle:@"Redo" action:@selector(redo:) keyEquivalent:@"Z"];
  [menu addItem:[NSMenuItem separatorItem]];
  [menu addItemWithTitle:@"Cut" action:@selector(cut:) keyEquivalent:@"x"];
  [menu addItemWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];
  [menu addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];
  [menu addItemWithTitle:@"Delete" action:@selector(delete:) keyEquivalent:@""];
  [menu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
  [menu addItem:[NSMenuItem separatorItem]];

  submenu = [[NSMenu alloc] initWithTitle:@"Find"];
  [[submenu addItemWithTitle:@"Find…" action:@selector(performFindPanelAction:) keyEquivalent:@"f"] setTag:1];
  [[submenu addItemWithTitle:@"Find Next" action:@selector(performFindPanelAction:) keyEquivalent:@"g"] setTag:2];
  [[submenu addItemWithTitle:@"Find Previous" action:@selector(performFindPanelAction:) keyEquivalent:@"G"] setTag:3];
  [[submenu addItemWithTitle:@"Use Selection for Find" action:@selector(performFindPanelAction:) keyEquivalent:@"e"] setTag:7];
  [submenu addItemWithTitle:@"Jump to Selection" action:@selector(centerSelectionInVisibleArea:) keyEquivalent:@"j"];
  [[menu addItemWithTitle:@"Find" action:NULL keyEquivalent:@""] setSubmenu:submenu];
  [submenu release];

  submenu = [[NSMenu alloc] initWithTitle:@"Spelling"];
  [submenu addItemWithTitle:@"Spelling…" action:@selector(showGuessPanel:) keyEquivalent:@":"];
  [submenu addItemWithTitle:@"Check Spelling" action:@selector(checkSpelling:) keyEquivalent:@";"];
  [submenu addItemWithTitle:@"Check Spelling as You Type" action:@selector(toggleContinuousSpellChecking:) keyEquivalent:@""];
  [[menu addItemWithTitle:@"Spelling" action:NULL keyEquivalent:@""] setSubmenu:submenu];
  [submenu release];

  [[mainMenu addItemWithTitle:@"Edit" action:NULL keyEquivalent:@""] setSubmenu:menu];
  [menu release];

  menu = [[NSMenu alloc] initWithTitle:@"Window"];
  [menu addItemWithTitle:@"Close" action:@selector(performClose:) keyEquivalent:@"w"];
  [menu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
  [[mainMenu addItemWithTitle:@"Window" action:NULL keyEquivalent:@""] setSubmenu:menu];
  [app setWindowsMenu:menu];
  [menu release];

  menu = [[NSMenu alloc] initWithTitle:@"Help"];
  [menu addItemWithTitle:@"NewApplication Help" action:@selector(showHelp:) keyEquivalent:@"?"];
  [[mainMenu addItemWithTitle:@"Help" action:NULL keyEquivalent:@""] setSubmenu:menu];
  [app setHelpMenu:menu];
  [menu release];

  [app setMainMenu:mainMenu];
  [mainMenu release];

  window = [[NSWindow alloc] initWithContentRect:NSMakeRect(74, 385, 659, 539)
                                       styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
                                                 NSWindowStyleMaskResizable
                                         backing:NSBackingStoreBuffered
                                           defer:YES];
  [window setTitle:@"WoWError"];
  [window setMinSize:NSMakeSize(659, 561)];
  [window setFrameAutosaveName:@"WoWError log window"];

  NSView      *contentView = [window contentView];
  NSFont      *font = [NSFont fontWithName:@"LucidaGrande" size:13];
  NSTextField *field;
  NSButton    *button;
  NSScrollView *scrollView;
  NSSize        size;

  scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 60, 619, 185)];
  [scrollView setBorderType:NSBezelBorder];
  [scrollView setHasVerticalScroller:YES];
  [scrollView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  size = [scrollView contentSize];
  logTextView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, size.width, size.height)];
  [logTextView setMinSize:NSMakeSize(0, size.height)];
  [logTextView setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
  [logTextView setVerticallyResizable:YES];
  [logTextView setAutoresizingMask:NSViewWidthSizable];
  [[logTextView textContainer] setWidthTracksTextView:YES];
  [logTextView setRichText:NO];
  [logTextView setEditable:NO];
  [logTextView setBackgroundColor:[NSColor whiteColor]];
  [logTextView setTextColor:[NSColor blackColor]];
  [logTextView setInsertionPointColor:[NSColor blackColor]];
  [logTextView setFont:[NSFont fontWithName:@"Courier" size:12]];
  [logTextView setString:@"log"];
  [scrollView setDocumentView:logTextView];
  [logTextView release];
  [contentView addSubview:scrollView];
  [scrollView release];

  button = [[NSButton alloc] initWithFrame:NSMakeRect(502, 12, 143, 32)];
  [button setTitle:@"Send to Blizzard"];
  [button setBezelStyle:NSBezelStyleRounded];
  [button setFont:font];
  [button setAutoresizingMask:NSViewMinXMargin | NSViewMaxYMargin];
  [button setTarget:self];
  [button setAction:@selector(sendAction:)];
  [contentView addSubview:button];
  [button release];

  button = [[NSButton alloc] initWithFrame:NSMakeRect(420, 12, 82, 32)];
  [button setTitle:@"Cancel"];
  [button setBezelStyle:NSBezelStyleRounded];
  [button setFont:font];
  [button setAutoresizingMask:NSViewMinXMargin | NSViewMaxYMargin];
  [button setTarget:self];
  [button setAction:@selector(cancelAction:)];
  [contentView addSubview:button];
  [button release];

  scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 253, 619, 153)];
  [scrollView setBorderType:NSBezelBorder];
  [scrollView setHasVerticalScroller:YES];
  [scrollView setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
  size = [scrollView contentSize];
  userDescriptionTextView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, size.width, size.height)];
  [userDescriptionTextView setMinSize:NSMakeSize(0, size.height)];
  [userDescriptionTextView setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
  [userDescriptionTextView setVerticallyResizable:YES];
  [userDescriptionTextView setAutoresizingMask:NSViewWidthSizable];
  [[userDescriptionTextView textContainer] setWidthTracksTextView:YES];
  [userDescriptionTextView setRichText:NO];
  [userDescriptionTextView setBackgroundColor:[NSColor whiteColor]];
  [userDescriptionTextView setTextColor:[NSColor blackColor]];
  [userDescriptionTextView setInsertionPointColor:[NSColor blackColor]];
  [scrollView setDocumentView:userDescriptionTextView];
  [userDescriptionTextView release];
  [contentView addSubview:scrollView];
  [scrollView release];

  field = [[NSTextField alloc] initWithFrame:NSMakeRect(101, 466, 541, 28)];
  [field setStringValue:@"The following data will be sent to Blizzard when you click Send.\n"
                        @"Though you can opt not to send this information, doing so will help us improve the game."];
  [field setFont:[NSFont fontWithName:@"LucidaGrande" size:11]];
  [field setBezeled:NO];
  [field setDrawsBackground:NO];
  [field setEditable:NO];
  [field setSelectable:NO];
  [field setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
  [contentView addSubview:field];
  [field release];

  field = [[NSTextField alloc] initWithFrame:NSMakeRect(17, 414, 625, 17)];
  [field setStringValue:@"Describe what you were doing when the crash occurred."];
  [field setFont:font];
  [field setBezeled:NO];
  [field setDrawsBackground:NO];
  [field setEditable:NO];
  [field setSelectable:NO];
  [field setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
  [contentView addSubview:field];
  [field release];

  field = [[NSTextField alloc] initWithFrame:NSMakeRect(101, 502, 541, 17)];
  [field setStringValue:@"World of Warcraft has crashed."];
  [field setFont:font];
  [field setBezeled:NO];
  [field setDrawsBackground:NO];
  [field setEditable:NO];
  [field setSelectable:NO];
  [field setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
  [contentView addSubview:field];
  [field release];

  iconImageView = [[NSImageView alloc] initWithFrame:NSMakeRect(20, 455, 64, 64)];
  [iconImageView setImage:[NSImage imageNamed:NSImageNameApplicationIcon]];
  [iconImageView setEditable:YES];
  [iconImageView setAutoresizingMask:NSViewMaxXMargin | NSViewMinYMargin];
  [contentView addSubview:iconImageView];
  [iconImageView release];

  [window setInitialFirstResponder:userDescriptionTextView];

  return self;
}

- (void)cancelAction:(id)sender {
  [window close];
}

- (void)compat_stream:(WEPrintStream *)stream handleEvent:(int)event {
  BOOL done = NO;

  if (event & NSStreamEventHasSpaceAvailable) {
    const void *bytes = [dataToSend bytes];
    int         length = [dataToSend length];
    int         written = [stream write:(const unsigned char *)bytes maxLength:length < 1024 ? length : 1024];

    if (written <= 0) {
      [dataToSend release];
      done = YES;
    } else {
      length -= written;
      dataToSend = [[[dataToSend autorelease] subdataWithRange:NSMakeRange(written, length)] retain];
    }

    if (length <= 0) {
      sendComplete = 1;
      done = YES;
    }

    if (done) {
      [stream close];
      sending = 0;

      NSString *message = @"An error occurred sending log.";
      if (sendComplete) {
        message = @"Your error log was sent.";
      }

      NSNotification *note = [NSNotification notificationWithName:NoteInfo object:[[NSString alloc] initWithString:message]];
      [[NSNotificationQueue defaultQueue] enqueueNotification:note postingStyle:NSPostASAP];
    }
  } else if (event & NSStreamEventErrorOccurred) {
    NSNotification *note = [NSNotification notificationWithName:NoteError object:[[stream streamError] retain]];
    [[NSNotificationQueue defaultQueue] enqueueNotification:note postingStyle:NSPostASAP];
    [stream close];
    sending = 0;
  }
}

- (NSData *)getDataToSend {
  NSMutableString *data = [[[NSMutableString alloc] init] autorelease];
  NSString        *lineEnd = @"\r\n";
  NSString        *text = [[userDescriptionTextView string] stringByAppendingFormat:@"\n\n%@", [logTextView string]];
  NSUInteger       start = 0;
  NSUInteger       end = 0;
  NSUInteger       contentsEnd = 0;
  NSUInteger       length = [text length];

  while (end < length) {
    [text getLineStart:&start end:&end contentsEnd:&contentsEnd forRange:NSMakeRange(end, 1)];
    [data appendString:[text substringWithRange:NSMakeRange(start, contentsEnd - start)]];
    [data appendString:lineEnd];
  }

  const char *utf8 = [[NSString stringWithString:data] UTF8String];
  return [[NSData alloc] initWithBytes:utf8 length:strlen(utf8)];
}

- (void)sendAction:(id)sender {
  if (sending) {
    return;
  }

  WEPrintStream *stream = [[WEPrintStream alloc] initWithName:WOW_ERROR_SERVER_ADDRESS port:8086];
  if (stream) {
    dataToSend = [self getDataToSend];
    [stream setDelegate:self];
    [stream open];
    sendComplete = 0;
    sending = 1;
  }
}

- (void)awakeFromNib {
  sending = 0;
  sendComplete = 0;

  NSString *path = [[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent] stringByAppendingPathComponent:@"wow.icns"];
  NSImage  *image = [[NSImage alloc] initWithContentsOfFile:path];
  if (image) {
    [iconImageView setImage:image];
    [[NSApplication sharedApplication] setApplicationIconImage:image];
    [image release];
  }

  logFile = nil;
  [[NSApplication sharedApplication] setDelegate:self];
  [window setDelegate:self];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(noteError:) name:NoteError object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(noteInfo:) name:NoteInfo object:nil];
}

- (void)noteError:(NSNotification *)note {
  NSString *description = [[[note object] autorelease] description];
  NSBeginCriticalAlertSheet(@"An error has occurred", @"OK", nil, nil, window, nil, NULL, NULL, NULL, description);
}

- (void)noteInfo:(NSNotification *)note {
  NSString *message = [[note object] autorelease];
  NSBeginAlertSheet(message, @"OK", nil, nil, window, self, @selector(sheetDidEnd:returnCode:contextInfo:), NULL, NULL, @"");
}

- (BOOL)application:(NSApplication *)sender openFile:(NSString *)filename {
  logFile = [filename retain];
  return YES;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
  NSString *log = nil;

  if (logFile) {
    log = [NSString stringWithContentsOfFile:logFile];
  }

  if (log && [log length]) {
    [logTextView replaceCharactersInRange:NSMakeRange(0, [[logTextView string] length]) withString:log];
    [window makeKeyAndOrderFront:self];
  } else {
    [window close];
  }
}

- (void)windowWillClose:(NSNotification *)notification {
  [[NSApplication sharedApplication] terminate:self];
}

- (void)sheetDidEnd:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo {
  if (sendComplete) {
    [window close];
  }
}

@end
