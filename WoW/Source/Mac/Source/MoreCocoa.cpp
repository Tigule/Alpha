#define BOOL OBJC_BOOL
#import <AppKit/AppKit.h>
#undef BOOL

#include <storm.h>
#include <Event/EvtApi.h>

#include <unistd.h>

@interface MultiThreadingMode : NSObject {
  unsigned char m_finished;
}
- (id)init;
- (void)enter;
- (void)NoOpThread;
@end

@implementation MultiThreadingMode

- (void)enter {
  [NSThread detachNewThreadSelector:@selector(NoOpThread) toTarget:self withObject:self];

  while (!__atomic_load_n(&m_finished, __ATOMIC_ACQUIRE)) {
    usleep(10);
  }
}

- (void)NoOpThread {
  __atomic_store_n(&m_finished, 1, __ATOMIC_RELEASE);
}

- (void)dealloc {
  [super dealloc];
}

- (id)init {
  m_finished = 0;
  return [super init];
}

@end

namespace MoreCocoa {

NSAutoreleasePool *s_pool;

BOOL AutoReleaseFlush(LPCVOID, LPVOID) {
  [s_pool release];
  s_pool = [[NSAutoreleasePool alloc] init];
  EventSetTimer(15000u, AutoReleaseFlush, 0);
  return 1;
}

void Initialize() {
  s_pool = [[NSAutoreleasePool alloc] init];

  if (![NSThread isMultiThreaded]) {
    MultiThreadingMode *mode = [[MultiThreadingMode alloc] init];

    [mode enter];
    [mode release];

    ASSERT([NSThread isMultiThreaded]);
  }
}

}
