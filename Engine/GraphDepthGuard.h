/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

#ifndef Natron_Engine_GraphDepthGuard_h
#define Natron_Engine_GraphDepthGuard_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cstddef>

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER

/**
 * @brief The recursive traversals of the node graph that walk upstream, one call per node
 * (see GraphDepthGuard).
 **/
enum GraphTraversalEnum
{
    eGraphTraversalRender = 0, // EffectInstance::renderRoI
    eGraphTraversalRegionOfDefinition, // EffectInstance::getRegionOfDefinition_public
    eGraphTraversalRequest, // EffectInstance::getInputsRoIsFunctor (computeRequestPass)
    eGraphTraversalFrameVarying, // EffectInstance::isFrameVaryingOrAnimated_Recursive
    eGraphTraversalCount
};

/**
 * @brief Stops a recursive traversal of the node graph before it overflows the stack (#1440).
 *
 * The render, the region of definition and the other traversals that walk upstream recurse once
 * per node, using a few KB of stack per node, so a chain of a few thousand nodes overflowed the
 * stack of the thread and crashed, even with the 8 MB render-thread stacks of #1434.
 *
 * Create a GraphDepthGuard on the stack at the start of each recursive call. If isTooDeep() is true,
 * the call must not recurse further, and must return a failure (or a conservative result). A call is
 * too deep if less than a safety margin (an eighth of the stack, between 256 KB and 1 MB) is left on
 * the stack of the thread, or if more than getMaxDepth() calls of the same traversal are nested.
 *
 * Once a call is too deep, every guarded call of the thread is too deep until the outermost guard
 * of the thread is destroyed, so that the whole traversal (and any other traversal it is part of,
 * such as the render that asked for the region of definition) fails quickly. The outermost guard
 * then sets the persistent error message "Node graph too deep..." on its node, which is shown in the
 * GUI and printed by NatronRenderer, and resets the state of the thread.
 **/
class GraphDepthGuard
{
public:

    GraphDepthGuard(GraphTraversalEnum traversal,
                    const EffectInstance* effect);

    ~GraphDepthGuard();

    // true if this call, or a call of the current traversal of this thread, was too deep
    bool isTooDeep() const;

    /**
     * @brief true if a guarded call was too deep on this thread, since the outermost guard of the
     * thread was created. Code that would otherwise swallow the failure of a traversal (e.g. renderRoI
     * after getRegionOfDefinition failed) uses this to fail too.
     **/
    static bool hasFailed();

    /**
     * @brief The maximum number of nested calls of a traversal, in addition to the stack check, or 0
     * for no limit (the default, the stack check is enough). If the stack bounds of a thread are
     * unknown, kGraphDepthGuardFallbackMaxDepth is used.
     **/
    static int getMaxDepth();
    static void setMaxDepth(int maxDepth);

    /**
     * @brief The number of bytes left on the stack of the current thread before the safety margin, or
     * 0 if the stack bounds are unknown (for the tests).
     **/
    static std::size_t getStackLeft();

private:

    bool _tooDeep;
    GraphTraversalEnum _traversal;
    const EffectInstance* _effect;
};

// The maximum depth used when the stack bounds of a thread are unknown
#define kGraphDepthGuardFallbackMaxDepth 1000

NATRON_NAMESPACE_EXIT

#endif // Natron_Engine_GraphDepthGuard_h
