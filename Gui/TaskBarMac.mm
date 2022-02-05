/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "TaskBarMac.h"

#import <AppKit/NSDockTile.h>
#import <AppKit/NSApplication.h>
#import <AppKit/NSImageView.h>
#import <AppKit/NSCIImageRep.h>
#import <AppKit/NSBezierPath.h>
#import <AppKit/NSColor.h>
#import <AppKit/NSView.h>
#import <Foundation/NSString.h>

@interface TaskBarView : NSView {
    double _progress;
}
- (void)setProgress:(double)value;
@end

@implementation TaskBarView

- (void)setProgress:(double)value
{
    _progress = value;
    [[NSApp dockTile] display];
}
- (void)drawRect:(NSRect)rect
{
    Q_UNUSED(rect)

    NSRect boundary = [self bounds];
    [[NSApp applicationIconImage] drawInRect:boundary
                                    fromRect:NSZeroRect
                                   operation:NSCompositingOperationCopy
                                    fraction:1.0];

    if (_progress <= 0.0 || _progress >= 1.0) {
        return;
    }

    NSRect progressBoundary = boundary;
    progressBoundary.size.height *= 0.15;
    progressBoundary.size.width *= 0.75;
    progressBoundary.origin.x = ( NSWidth(boundary) - NSWidth(progressBoundary) ) / 2.0;
    progressBoundary.origin.y = (NSHeight(boundary) / 2.0) - (NSHeight(progressBoundary) / 2.0);

    NSRect currentProgress = progressBoundary;
    currentProgress.size.width *= _progress;

    [[NSColor blackColor] setFill];
    [NSBezierPath fillRect:progressBoundary];

    [[NSColor systemBlueColor] setFill];
    [NSBezierPath fillRect:currentProgress];

    [[NSColor blackColor] setStroke];
    [NSBezierPath strokeRect:progressBoundary];
}
@end

NATRON_NAMESPACE_ENTER

struct TaskBarMac::TB
{
    TaskBarView *view;
};

TaskBarMac::TaskBarMac(QObject *parent)
    : QObject(parent)
    , _tb(new TB)
{
    _tb->view = [[TaskBarView alloc] init];
    [[NSApp dockTile] setContentView:_tb->view];
}

TaskBarMac::~TaskBarMac()
{
    [[NSApp dockTile] setContentView:nil];
    [_tb->view release];
    delete _tb;
}

void
TaskBarMac::setProgress(double value)
{
    [_tb->view setProgress:value];

    if (value == 1.0) {
        [NSApp requestUserAttention: NSInformationalRequest];
    }
}

void
TaskBarMac::setProgressVisible(bool visible)
{
    if (!visible) {
        setProgress(0.0);
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_TaskBarMac.cpp"
