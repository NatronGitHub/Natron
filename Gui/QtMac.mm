/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

//See https://trac.macports.org/ticket/43283
#if !defined(MAC_OS_X_VERSION_10_7) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7
#import <Cocoa/Cocoa.h> 
@interface NSScreen (LionAPI)
- (CGFloat)backingScaleFactor;
- (NSRect)convertRectToBacking:(NSRect)aRect;
@end

#endif // 10.7

namespace Natron {
bool 
isHighDPIInternal(const QWidget* w) {
    NSView* view = reinterpret_cast<NSView*>(w->winId());
    CGFloat scaleFactor = 1.0;
    if ([[view window] respondsToSelector: @selector(backingScaleFactor)])
        scaleFactor = [[view window] backingScaleFactor];
    
    return (scaleFactor > 1.0);
}
}
#endif
