#include <waylandgui/waylandgui.h>
#import <Cocoa/Cocoa.h>

NAMESPACE_BEGIN(waylandgui)

void *autorelease_init() {
    return [[NSAutoreleasePool alloc] init];
}

void autorelease_release(void *pool_) {
    [((NSAutoreleasePool *) pool_) release];
}

NAMESPACE_END(waylandgui)
