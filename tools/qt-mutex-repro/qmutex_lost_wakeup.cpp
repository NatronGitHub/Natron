// qmutex_lost_wakeup: stress test for a lost wake-up in QMutex's non-futex implementation.
//
// Qt 5 on macOS, and Qt 6 where futexes are not used (Qt 6.0-6.6 on macOS, App Store-compliant
// builds up to 6.11, QNX, VxWorks, INTEGRITY, OpenBSD, NetBSD, Hurd, Solaris, LSB, wasm), lock
// a contended QMutex in QBasicMutex::lockInternal() and unlock it in unlockInternal(). Two
// relaxed atomic operations on QMutexPrivate::waiters there let a locking thread go to sleep on
// a QMutexPrivate that the unlocking thread has already detached from the mutex, on CPUs with
// a weak memory model (arm64). The mutex is then unlocked, but that thread never wakes up.
//
// The test runs pairs of threads that lock and unlock one QMutex per pair, so that the mutex
// often goes from "locked, no waiter" to "one waiter arriving" exactly while it is unlocked.
// A watchdog reports a thread that has been inside QMutex::lock() for --stall seconds while
// the mutex is free (QMutex::tryLock() from the watchdog succeeds): that is a lost wake-up.
//
// Build (one file, QtCore only):
//   c++ -std=c++17 -O2 qmutex_lost_wakeup.cpp $(pkg-config --cflags --libs Qt5Core) -o qmutex_lost_wakeup
//   c++ -std=c++17 -O2 qmutex_lost_wakeup.cpp $(pkg-config --cflags --libs Qt6Core) -o qmutex_lost_wakeup
// or with the CMakeLists.txt next to it.
// Usage: qmutex_lost_wakeup [--seconds N] [--pairs P] [--stall S] [--recursive]
// Exit status: 0 if no thread got stuck, 1 if a lost wake-up was detected, 2 on usage error.

#include <QtCore/QMutex>
#include <QtCore/QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#include <QtCore/QRecursiveMutex>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>
#include <unistd.h>

namespace {

struct alignas(128) Worker
{
    std::atomic<unsigned long long> iterations{0};
    std::atomic<bool> inLock{false};   // inside QMutex::lock()
    std::atomic<long long> lockEnteredNs{0};
};

long long nowNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// a short, variable pause, so that the two threads of a pair keep arriving at different moments
inline void pause(unsigned &seed, unsigned maxSpins)
{
    seed = seed * 1103515245u + 12345u;
    for (unsigned i = (seed >> 16) % maxSpins; i > 0; --i) {
        std::atomic_signal_fence(std::memory_order_seq_cst);
    }
}

template <typename Mutex>
int run(int seconds, int pairs, int stallSeconds, const char *kind)
{
    std::vector<std::unique_ptr<Mutex> > mutexes;
    for (int i = 0; i < pairs; ++i) {
        mutexes.emplace_back(new Mutex);
    }
    std::vector<Worker> workers(2 * pairs);
    std::vector<unsigned long long> shared(pairs * 16, 0);   // one counter per pair, padded
    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;

    std::printf("qmutex_lost_wakeup: Qt %s (built with %s), %s, %d pairs of threads, %d s, stall limit %d s\n",
                qVersion(), QT_VERSION_STR, kind, pairs, seconds, stallSeconds);
    std::fflush(stdout);
    for (int t = 0; t < 2 * pairs; ++t) {
        threads.emplace_back([&, t] {
            Mutex &m = *mutexes[t / 2];
            Worker &w = workers[t];
            unsigned seed = 0x9e3779b9u * (t + 1);
            while ( !stop.load(std::memory_order_relaxed) ) {
                w.lockEnteredNs.store(nowNs(), std::memory_order_relaxed);
                w.inLock.store(true, std::memory_order_release);
                m.lock();
                w.inLock.store(false, std::memory_order_release);
                ++shared[(t / 2) * 16];
                pause(seed, 64);
                m.unlock();
                w.iterations.fetch_add(1, std::memory_order_relaxed);
                pause(seed, 256);
            }
        });
    }

    const long long start = nowNs();
    const long long stallNs = stallSeconds * 1000000000LL;
    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        const long long now = nowNs();
        for (int t = 0; t < 2 * pairs; ++t) {
            Worker &w = workers[t];
            if ( !w.inLock.load(std::memory_order_acquire) ) {
                continue;
            }
            const long long waited = now - w.lockEnteredNs.load(std::memory_order_relaxed);
            if (waited < stallNs) {
                continue;
            }
            Mutex &m = *mutexes[t / 2];
            // The other thread of the pair may hold the mutex: then it is not a lost wake-up.
            if ( m.tryLock() ) {
                const bool stillStuck = w.inLock.load(std::memory_order_acquire) &&
                                        w.lockEnteredNs.load(std::memory_order_relaxed) == now - waited;
                m.unlock();
                if (stillStuck) {
                    unsigned long long total = 0;
                    for (const Worker &x : workers) {
                        total += x.iterations.load();
                    }
                    std::printf("FAIL: lost wake-up after %.1f s and %llu lock/unlock cycles: thread %d has been "
                                "blocked in QMutex::lock() for %.1f s, but the mutex is unlocked (tryLock() "
                                "from the watchdog succeeded).\n",
                                (now - start) / 1e9, total, t, waited / 1e9);
                    std::fflush(stdout);
                    _exit(1);   // the stuck thread can't be joined
                }
            }
        }
        if (now - start >= seconds * 1000000000LL) {
            break;
        }
    }
    stop = true;
    for (std::thread &th : threads) {
        th.join();
    }
    unsigned long long total = 0;
    for (const Worker &x : workers) {
        total += x.iterations.load();
    }
    std::printf("PASS: no lost wake-up in %d s, %llu lock/unlock cycles\n", seconds, total);

    return 0;
}

} // anonymous namespace

int main(int argc, char **argv)
{
    int seconds = 60, stall = 5;
    int pairs = std::max(1u, std::thread::hardware_concurrency() / 2);
    bool recursive = false;
    for (int i = 1; i < argc; ++i) {
        if ( !std::strcmp(argv[i], "--seconds") && (i + 1 < argc) ) {
            seconds = std::atoi(argv[++i]);
        } else if ( !std::strcmp(argv[i], "--pairs") && (i + 1 < argc) ) {
            pairs = std::atoi(argv[++i]);
        } else if ( !std::strcmp(argv[i], "--stall") && (i + 1 < argc) ) {
            stall = std::atoi(argv[++i]);
        } else if ( !std::strcmp(argv[i], "--recursive") ) {
            recursive = true;
        } else {
            std::fprintf(stderr, "usage: %s [--seconds N] [--pairs P] [--stall S] [--recursive]\n", argv[0]);

            return 2;
        }
    }
    if ( (seconds <= 0) || (pairs <= 0) || (stall <= 0) ) {
        std::fprintf(stderr, "%s: --seconds, --pairs and --stall must be positive\n", argv[0]);

        return 2;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    if (recursive) {
        return run<QRecursiveMutex>(seconds, pairs, stall, "QRecursiveMutex");
    }
#else
    if (recursive) {
        std::fprintf(stderr, "%s: --recursive needs Qt 5.14 or later\n", argv[0]);

        return 2;
    }
#endif

    return run<QMutex>(seconds, pairs, stall, "QMutex");
}
