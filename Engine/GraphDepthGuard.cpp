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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "GraphDepthGuard.h"

#include <algorithm> // min, max
#include <atomic>
#include <cassert>
#include <map>
#include <string>
#include <utility>
#include <vector>

#if defined(__NATRON_WIN32__)
#include <windows.h>
#elif defined(__NATRON_UNIX__)
#include <pthread.h>
#if defined(__FreeBSD__)
#include <pthread_np.h>
#endif
#if defined(__NATRON_LINUX__)
#include <sys/syscall.h>
#include <unistd.h>
#endif
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QString>

#include "Engine/EffectInstance.h"
#include "Engine/Node.h"

NATRON_NAMESPACE_ENTER

namespace {
// The state of the current thread. Trivially constructible, so that it needs no dynamic
// initialization of the thread-local storage.
struct GraphDepthThreadData
{
    int totalDepth; // number of nested guards, all traversals
    int depth[eGraphTraversalCount]; // number of nested guards of each traversal
    bool failed; // a guarded call was too deep since the outermost guard was created
    bool stackInitialized; // stackLimit was computed
    const char* stackLimit; // lowest address the stack may reach (safety margin included), or NULL if unknown
};

thread_local GraphDepthThreadData tData = {};

std::atomic<int> gMaxDepth(0);

// true once a "Node graph too deep" message was set, so that the successful renders only look for
// one to clear after that
std::atomic<bool> gMessageSet(false);

// The start of the message, to recognize it
QString
messagePrefix()
{
    return QCoreApplication::translate("GraphDepthGuard", "Node graph too deep:");
}

// The current position in the stack (which grows down on all supported platforms). With the address
// sanitizer, local variables may live in a "fake stack" on the heap, but the frame address doesn't.
inline const char*
currentStackPosition()
{
#if defined(__GNUC__) || defined(__clang__)
    return static_cast<const char*>( __builtin_frame_address(0) );
#else
    volatile char c = 0;

    return const_cast<const char*>(&c);
#endif
}

// The stack bounds of the current thread: lowest address and size, or false if unknown.
bool
getCurrentThreadStack(const char** low,
                      std::size_t* size)
{
#if defined(__NATRON_WIN32__)
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0602)
    // Windows 8 and later
    ULONG_PTR lowLimit = 0, highLimit = 0;
    GetCurrentThreadStackLimits(&lowLimit, &highLimit);
    if ( lowLimit && (highLimit > lowLimit) ) {
        *low = reinterpret_cast<const char*>(lowLimit);
        *size = highLimit - lowLimit;

        return true;
    }
#endif
    // The stack is one reservation: its allocation base is its lowest address. The size is only used
    // for the safety margin: the distance from the current position to the base is a lower bound.
    MEMORY_BASIC_INFORMATION mbi;
    const char* position = currentStackPosition();
    if ( VirtualQuery(position, &mbi, sizeof(mbi) ) == 0 ) {
        return false;
    }
    *low = static_cast<const char*>(mbi.AllocationBase);
    *size = position - *low;

    return true;
#elif defined(__NATRON_OSX__)
    pthread_t self = pthread_self();
    const char* high = static_cast<const char*>( pthread_get_stackaddr_np(self) ); // the stack grows down from here
    *size = pthread_get_stacksize_np(self);
    *low = high - *size;

    return true;
#elif defined(__NATRON_LINUX__) || defined(__FreeBSD__)
    pthread_attr_t attr;
#if defined(__FreeBSD__)
    if (pthread_attr_init(&attr) != 0) {
        return false;
    }
    if (pthread_attr_get_np(pthread_self(), &attr) != 0) {
        pthread_attr_destroy(&attr);

        return false;
    }
#else
    if (pthread_getattr_np(pthread_self(), &attr) != 0) {
        return false;
    }
#endif
    void* addr = 0;
    std::size_t stackSize = 0;
    const bool ok = (pthread_attr_getstack(&attr, &addr, &stackSize) == 0);
    pthread_attr_destroy(&attr);
    if (!ok || !addr || !stackSize) {
        return false;
    }
    *low = static_cast<const char*>(addr);
    *size = stackSize;
#if defined(__NATRON_LINUX__)
    if ( syscall(SYS_gettid) == getpid() ) {
        // The main thread's stack grows on demand, and the kernel keeps a gap (stack_guard_gap, 256
        // pages by default) between it and the mapping below it. With `ulimit -s unlimited`,
        // glibc reports the stack as reaching that mapping: don't count the gap.
        const std::size_t gap = 256 * (std::size_t)sysconf(_SC_PAGESIZE);
        if (*size > 2 * gap) {
            *low += gap;
            *size -= gap;
        }
    }
#endif

    return true;
#else
    (void)low;
    (void)size;

    return false;
#endif
} // getCurrentThreadStack

const char*
getStackLimit()
{
    if (!tData.stackInitialized) {
        tData.stackInitialized = true;
        tData.stackLimit = 0;
        const char* low = 0;
        std::size_t size = 0;
        if ( getCurrentThreadStack(&low, &size) && low && (size > 0) ) {
            // What runs after the deepest guarded call (a plug-in's render action, an expression,
            // the unwinding) must still fit: keep an eighth of the stack, between 256 KB and 1 MB.
            const std::size_t margin = std::min<std::size_t>( std::max<std::size_t>(size / 8, 256 * 1024), 1024 * 1024 );
            if (margin < size) {
                tData.stackLimit = low + margin;
            }
        }
    }

    return tData.stackLimit;
}
} // anon namespace

GraphDepthGuard::GraphDepthGuard(GraphTraversalEnum traversal,
                                 const EffectInstance* effect)
    : _tooDeep(false)
    , _traversal(traversal)
    , _effect(effect)
{
    assert(traversal >= 0 && traversal < eGraphTraversalCount);
    ++tData.totalDepth;
    const int depth = ++tData.depth[traversal];
    if (tData.failed) {
        // the traversal is failing: unwind it quickly
        _tooDeep = true;

        return;
    }
    const char* stackLimit = getStackLimit();
    int maxDepth = gMaxDepth.load(std::memory_order_relaxed);
    if ( !stackLimit && (maxDepth <= 0) ) {
        maxDepth = kGraphDepthGuardFallbackMaxDepth;
    }
    if ( ( (maxDepth > 0) && (depth > maxDepth) ) ||
         ( stackLimit && ( currentStackPosition() < stackLimit ) ) ) {
        _tooDeep = true;
        tData.failed = true;
    }
}

GraphDepthGuard::~GraphDepthGuard()
{
    --tData.depth[_traversal];
    if (--tData.totalDepth > 0) {
        return;
    }
    // This is the outermost guard of the thread: report the error and reset the state.
    assert(tData.totalDepth == 0);
    const bool failed = tData.failed;
    tData.failed = false;
    if ( !_effect || ( !failed && ( (_traversal != eGraphTraversalRender) || !gMessageSet.load() ) ) ) {
        return;
    }
    try {
        NodePtr node = _effect->getNode();
        if (!node) {
            return;
        }
        if (!failed) {
            // This render worked: clear the error of a previous render, if the graph was shortened
            QString message;
            int type;
            node->getPersistentMessage(&message, &type, false);
            if ( message.startsWith( messagePrefix() ) ) {
                node->clearPersistentMessage(false);
            }

            return;
        }
        const QString message = messagePrefix() + QLatin1Char(' ') +
                                QCoreApplication::translate("GraphDepthGuard",
                                                            "%1 has a chain of %2 nodes upstream, too many for the stack of this thread. "
                                                            "Shorten the chain, for example by pre-rendering a part of it with a Write or DiskCache node.")
                                .arg( QString::fromUtf8( node->getLabel_mt_safe().c_str() ) )
                                .arg( getLongestChainLength(_effect) );
        gMessageSet = true;
        node->setPersistentMessage( eMessageTypeError, message.toStdString() );
    } catch (...) {
        // a destructor must not throw: the render fails anyway
    }
}

int
GraphDepthGuard::getLongestChainLength(const EffectInstance* effect)
{
    // Iterative depth-first search: the graph is too deep for a recursion.
    std::map<const EffectInstance*, int> lengths; // -1 while being visited (protects against cycles)
    std::vector<std::pair<EffectInstancePtr, int> > stack; // node, next input to visit
    const EffectInstancePtr root = effect ? std::const_pointer_cast<EffectInstance>( effect->shared_from_this() ) : EffectInstancePtr();

    if (!root) {
        return 0;
    }
    stack.push_back( std::make_pair(root, 0) );
    lengths[root.get()] = -1;
    while ( !stack.empty() ) {
        const EffectInstancePtr node = stack.back().first;
        const int inputNb = stack.back().second;
        if ( inputNb < node->getNInputs() ) {
            ++stack.back().second;
            EffectInstancePtr input = node->getInput(inputNb);
            if ( input && ( lengths.find( input.get() ) == lengths.end() ) ) {
                lengths[input.get()] = -1;
                stack.push_back( std::make_pair(input, 0) );
            }
            continue;
        }
        // all the inputs are visited
        int length = 0;
        for (int i = 0; i < node->getNInputs(); ++i) {
            EffectInstancePtr input = node->getInput(i);
            if (input) {
                length = std::max( length, lengths[input.get()] );
            }
        }
        lengths[node.get()] = length + 1;
        stack.pop_back();
    }

    return lengths[root.get()];
}

bool
GraphDepthGuard::isTooDeep() const
{
    return _tooDeep || tData.failed;
}

bool
GraphDepthGuard::hasFailed()
{
    return tData.failed;
}

int
GraphDepthGuard::getMaxDepth()
{
    return gMaxDepth.load(std::memory_order_relaxed);
}

void
GraphDepthGuard::setMaxDepth(int maxDepth)
{
    gMaxDepth.store(maxDepth, std::memory_order_relaxed);
}

std::size_t
GraphDepthGuard::getStackLeft()
{
    const char* stackLimit = getStackLimit();
    const char* position = currentStackPosition();

    if ( !stackLimit || (position <= stackLimit) ) {
        return 0;
    }

    return position - stackLimit;
}

NATRON_NAMESPACE_EXIT
