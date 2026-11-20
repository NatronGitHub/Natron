/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

// Command-line parsing tests (CLArgs).
// Regression test for https://github.com/NatronGitHub/Natron/issues/1263: a frame range with a step
// (e.g. 30-50:2) given right after "-w <writer>" was taken as the output file name of the writer.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <limits>
#include <list>
#include <utility>

#include <gtest/gtest.h>

#include <QtCore/QString>
#include <QtCore/QStringList>

#include "Engine/CLArgs.h"

NATRON_NAMESPACE_USING

namespace {
typedef std::list<std::pair<int, std::pair<int, int> > > FrameRanges;

QStringList
makeArgs(const char* const* args)
{
    QStringList ret;

    ret << QString::fromUtf8("NatronRenderer"); // argv[0]
    for (; *args; ++args) {
        ret << QString::fromUtf8(*args);
    }

    return ret;
}

// The frame ranges of "1-10,20,30-50:2": (step, (first, last)), with no step stored as INT_MIN.
void
checkSteppedRanges(const FrameRanges& ranges)
{
    const int noStep = std::numeric_limits<int>::min();

    ASSERT_EQ( 3u, ranges.size() );
    FrameRanges::const_iterator it = ranges.begin();
    EXPECT_EQ(noStep, it->first);
    EXPECT_EQ(1, it->second.first);
    EXPECT_EQ(10, it->second.second);
    ++it;
    EXPECT_EQ(noStep, it->first);
    EXPECT_EQ(20, it->second.first);
    EXPECT_EQ(20, it->second.second);
    ++it;
    EXPECT_EQ(2, it->first);
    EXPECT_EQ(30, it->second.first);
    EXPECT_EQ(50, it->second.second);
}
} // anonymous namespace

// A frame range with a step right after the writer name is a frame range, not a file name (#1263).
TEST(CLArgs, WriterFollowedBySteppedFrameRange)
{
    const char* const args[] = { "-w", "Write1", "1-10,20,30-50:2", "project.ntp", 0 };
    CLArgs cl(makeArgs(args), true);

    EXPECT_FALSE( bool( cl.getError() ) );
    const std::list<CLArgs::WriterArg>& writers = cl.getWriterArgs();
    ASSERT_EQ( 1u, writers.size() );
    EXPECT_EQ( QString::fromUtf8("Write1"), writers.front().name );
    EXPECT_TRUE( writers.front().filename.isEmpty() ) << "the frame range was taken as a file name: " << writers.front().filename.toStdString();
    EXPECT_TRUE( cl.getScriptFilename().endsWith( QString::fromUtf8("project.ntp") ) );
    EXPECT_TRUE( cl.hasFrameRange() );
    checkSteppedRanges( cl.getFrameRanges() );
}

// The same ranges after the project file (the documented workaround) give the same result.
TEST(CLArgs, SteppedFrameRangeAfterProject)
{
    const char* const args[] = { "-w", "Write1", "project.ntp", "1-10,20,30-50:2", 0 };
    CLArgs cl(makeArgs(args), true);

    EXPECT_FALSE( bool( cl.getError() ) );
    ASSERT_EQ( 1u, cl.getWriterArgs().size() );
    EXPECT_TRUE( cl.getWriterArgs().front().filename.isEmpty() );
    EXPECT_TRUE( cl.hasFrameRange() );
    checkSteppedRanges( cl.getFrameRanges() );
}

// An output file name after the writer name is still recognized.
TEST(CLArgs, WriterFollowedByFileName)
{
    const char* const args[] = { "-w", "Write1", "/tmp/out_####.exr", "project.ntp", "30-50:2", 0 };
    CLArgs cl(makeArgs(args), true);

    EXPECT_FALSE( bool( cl.getError() ) );
    const std::list<CLArgs::WriterArg>& writers = cl.getWriterArgs();
    ASSERT_EQ( 1u, writers.size() );
    EXPECT_EQ( QString::fromUtf8("/tmp/out_####.exr"), writers.front().filename );
    ASSERT_EQ( 1u, cl.getFrameRanges().size() );
    EXPECT_EQ( 2, cl.getFrameRanges().front().first );
    EXPECT_EQ( 30, cl.getFrameRanges().front().second.first );
    EXPECT_EQ( 50, cl.getFrameRanges().front().second.second );
}
