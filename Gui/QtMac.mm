/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#include "Gui/Gui.h"

#ifdef Q_OS_MAC

#include <AppKit/NSView.h>

//See:
//- https://trac.macports.org/ticket/43283
//- https://code.google.com/p/chromiumembedded/source/browse/trunk/cef3/include/cef_application_mac.h
//- https://code.google.com/p/chromiumembedded/source/browse/trunk/cef3/tests/cefclient/cefclient_osr_widget_mac.mm?spec=svn1751&r=1751
// Note that in the following declarations, only backingScaleFactor is necessary,
// but we may need them in the future.

// Forward declarations for APIs that are part of the 10.7 SDK. This will allow
// using them when building with the 10.6 SDK.
#if !defined(MAC_OS_X_VERSION_10_7) || \
    MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7
#import <Cocoa/Cocoa.h>

enum {
  NSEventPhaseNone        = 0, // event not associated with a phase.
  NSEventPhaseBegan       = 0x1 << 0,
  NSEventPhaseStationary  = 0x1 << 1,
  NSEventPhaseChanged     = 0x1 << 2,
  NSEventPhaseEnded       = 0x1 << 3,
  NSEventPhaseCancelled   = 0x1 << 4,
};
typedef NSUInteger NSEventPhase;

@interface NSEvent (LionSDK)
+ (BOOL)isSwipeTrackingFromScrollEventsEnabled;

- (NSEventPhase)phase;
- (CGFloat)scrollingDeltaX;
- (CGFloat)scrollingDeltaY;
- (BOOL)isDirectionInvertedFromDevice;
@end

@interface NSScreen (LionSDK)
- (CGFloat)backingScaleFactor;
- (NSRect)convertRectToBacking:(NSRect)aRect;
@end

@interface NSWindow (LionSDK)
- (CGFloat)backingScaleFactor;
@end

@interface NSView (NSOpenGLSurfaceResolutionLionAPI)
- (void)setWantsBestResolutionOpenGLSurface:(BOOL)flag;
@end

@interface NSView (LionAPI)
- (NSSize)convertSizeToBacking:(NSSize)aSize;
- (NSRect)convertRectToBacking:(NSRect)aRect;
- (NSRect)convertRectFromBacking:(NSRect)aRect;
@end

#if 0
static NSString* const NSWindowDidChangeBackingPropertiesNotification =
    @"NSWindowDidChangeBackingPropertiesNotification";
static NSString* const NSBackingPropertyOldScaleFactorKey =
    @"NSBackingPropertyOldScaleFactorKey";
#endif

#endif  // MAC_OS_X_VERSION_10_7

NATRON_NAMESPACE_ENTER;

bool
QtMac::isHighDPIInternal(const QWidget* w) {
    NSView* view = reinterpret_cast<NSView*>(w->winId());
    CGFloat scaleFactor = 1.0;
    if ([[view window] respondsToSelector: @selector(backingScaleFactor)])
        scaleFactor = [[view window] backingScaleFactor];
    
    return (scaleFactor > 1.0);
}

NATRON_NAMESPACE_EXIT;

#endif
