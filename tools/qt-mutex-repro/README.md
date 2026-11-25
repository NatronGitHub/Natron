# QMutex lost wake-up on arm64 (non-futex QMutex): repro, litmus tests, patches

NatronGitHub/Natron#1451, #1453. A thread can sleep forever in `QMutex::lock()` on a mutex
that nobody holds. This affects Qt builds whose QMutex doesn't use futexes, on CPUs with a weak
memory model (arm64). It is a follow-up to QTBUG-88247.

## The bug

`src/corelib/thread/qmutex.cpp`, non-futex path. When `unlockInternal()` finds no waiter, it
runs, on the unlocking thread U:

```
(2) d->waiters.fetchAndAddRelease(-BigNumber)   // returns 0: no waiter
(3) d_ptr.testAndSetRelease(d, nullptr)         // the mutex is unlocked
(4) d->derefWaiters(0)                          // waiters.testAndSetRelaxed(-BigNumber, 0)
```

Meanwhile `lockInternal()` on thread W, which already holds a reference on `d`, runs:

```
(1) old = d->waiters.loadAcquire()
(5) d->waiters.testAndSetRelaxed(old, old + 1)
(6) if (d != d_ptr.loadAcquire()) retry; else d->wait();
```

If W reads, at (1) or at (5), the 0 written by (4), then (6) must see the `d_ptr` reset done by
(3). If it doesn't, W waits on a `QMutexPrivate` that is no longer attached to the mutex, and no
unlock will ever signal it. The mutex is unlocked (`d_ptr == nullptr`), while W sleeps with
`d->waiters == 1`. There are two ways for W to miss the reset:

- **Race A: (1) reads (4).** QTBUG-88247 (qtbase d08e3b6de161, Qt 5.15.3) made (1) an acquire.
  But (4) is relaxed, so (1) only synchronizes with (2), which comes before (3).
- **Race B, ABA: (5) reads (4).** (1) reads the 0 from *before* (2). Then U runs (2), (3) and
  (4), so `waiters` goes 0 → -BigNumber → 0. W's relaxed CAS (5) then succeeds against the 0
  written by (4), without synchronizing with it.

**Fix:** make (4) a release and (5) an acquire. Then (3) happens before (6) in both cases. On
x86 both CASes are `lock cmpxchg`, a full barrier, so the change generates the same code there.
On arm64 it turns `cas` into `casl` and `casa`.

## Files

| File | What |
|---|---|
| `qmutex_lost_wakeup.cpp` | The repro: a command-line stress test, one file, QtCore only, Qt 5 or Qt 6 |
| `CMakeLists.txt` | Builds it with Qt 6 or Qt 5 |
| `litmus/*.litmus` | herd7 AArch64 litmus tests for races A and B, with and without the fix |
| `litmus/herd7-aarch64.txt` | herd7 7.58 output for those tests |
| `qtbase-dev-QMutex-fix-lost-wakeup.patch` | The fix for qtbase `dev`, `git am` format. It also applies to 6.11 and 6.8 |
| `../MacPorts/aqua/qt5/files/patch-qtbase-qmutex-arm64-lost-wakeup.diff` | The same fix for Qt 5.15.19 (MacPorts overlay, `-p0`) |

## The repro

```sh
c++ -std=c++17 -O2 qmutex_lost_wakeup.cpp $(pkg-config --cflags --libs Qt5Core) -o qmutex_lost_wakeup
c++ -std=c++17 -O2 qmutex_lost_wakeup.cpp $(pkg-config --cflags --libs Qt6Core) -o qmutex_lost_wakeup
# or
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/qt && cmake --build build

./qmutex_lost_wakeup [--seconds N] [--pairs P] [--stall S] [--recursive]
```

- **What it runs:** P pairs of threads. The default is half the number of cores. Each pair locks
  and unlocks its own `QMutex`, or a `QRecursiveMutex` with `--recursive`.
- **How it detects a hang:** a watchdog looks for a thread that has been inside `lock()` for
  `--stall` seconds (default 5). It then checks that it can take the mutex itself, with
  `tryLock()`. If it can, the mutex is free, so the thread is not simply waiting for its partner.
  The watchdog checks again after another `--stall` seconds. A thread that was only descheduled
  has moved on by then.
- **Exit status:** 0 (`PASS`) if no thread got stuck within N seconds (default 60). 1 (`FAIL: lost
  wake-up ...`) on a hang. 2 on a usage error.

Results on an Apple Silicon Mac (macOS 26, arm64), `--pairs 4`, run at `nice 19`. All rows except
the last two used this file's watchdog, which confirms a hang twice. The first row was run with
the committed file itself; the others ran before three cosmetic edits (licence header,
`std::_Exit`, a renamed helper). The two earlier runs at the bottom used a first version that
confirmed a hang once, after 5 s.

| QtCore | Mutex | Runs | Result |
|---|---|---|---|
| MacPorts qt5-qtbase 5.15.19 (stock) | QRecursiveMutex | 1 | **FAIL** after 24 s |
| 5.15.19 from source, unpatched | QRecursiveMutex | 3 | **FAIL** 3/3, after 18, 26 and 73 s |
| 5.15.19 from source, unpatched | QMutex | 2 | **FAIL** 2/2, after 30 and 61 s |
| 5.15.19 from source, patched | QRecursiveMutex | 4 × 300 s | PASS 4/4 |
| 5.15.19 from source, patched | QMutex | 2 × 300 s | PASS 2/2 |
| 6.11.3, App Store-compliant, unpatched | QMutex | 2 | **FAIL** 2/2, after 16 and 93 s |
| 6.11.3, App Store-compliant, patched | QMutex | 2 × 300 s | PASS 2/2 |
| *first version:* MacPorts qt5-qtbase 5.15.19 (stock) | QMutex / QRecursiveMutex | 1 + 1 | **FAIL** after 187 s / 6.7 s |
| *first version:* 6.11.3, App Store-compliant | QMutex | 1 + 1 | unpatched **FAIL** after 568 s; patched PASS 600 s |

The Qt 6.11.3 builds are QtCore only. They were configured with `-DFEATURE_appstore_compliant=ON`:
- That is the App Store configuration, which doesn't use the private `__ulock` API.
- So QMutex uses the non-futex path with Mach semaphores, as in Qt 6.0–6.6 on macOS.
- The build imports `semaphore_wait` and no `__ulock_*`/`os_sync_*`.

The Qt 5.15.19 builds are QtCore built from the official qtbase tarball, with and without the
patch. In the patched builds, the disassembly shows `casl` in `derefWaiters()` and `casa` for (5).

## Litmus tests (herd7, AArch64 model)

Run them with `herd7 -model aarch64.cat litmus/*.litmus` (herdtools7 7.58, `opam install herdtools7`,
no other options). `litmus/herd7-aarch64.txt` is that output; a rerun differs only in the `Time` lines.
The tests model the two threads directly:
- `x` is `d_ptr` (1 means "points to d"), and `y` is `d->waiters` (2 stands for -BigNumber).
- P0 is U: (2) `LDADDL`, (3) `CASL`, (4) `CAS` or `CASL`.
- P1 is W: (1) `LDAR`, (5) `CAS` or `CASA`, (6) `LDAR`.

The bad outcome has three parts: U saw no waiter, W's CAS (5) succeeded after reading the reset
count, and W still read the old `d_ptr`. In race A, (4) writes 3 instead of 0, only so that a
read from (4) can be told apart from the initial 0. Memory ordering doesn't depend on the values.

The tests use `LDAR` for the acquire loads. Clang compiles them to `LDAPR` on Apple Silicon, and
the same tests with `LDAPR` give the same results.

herd7 reports, for each test, the number of candidate executions where the condition holds ("Positive")
and where it doesn't ("Negative"):

| Test | (4) | (5) | Bad outcome | herd7 Positive / Negative |
|---|---|---|---|---|
| `QMutexA-qt5_15_19` | relaxed | relaxed | **Sometimes** | 8 / 104 |
| `QMutexA-release-only` | release | relaxed | Never | 0 / 104 |
| `QMutexA-patched` | release | acquire | Never | 0 / 80 |
| `QMutexB-qt5_15_19` | relaxed | relaxed | **Sometimes** | 16 / 88 |
| `QMutexB-release-only` | release | relaxed | **Sometimes** | 8 / 88 |
| `QMutexB-patched` | release | acquire | Never | 0 / 76 |

So both changes are needed. The release on (4) alone fixes race A but not race B.

## Which Qt builds use the non-futex QMutex

- **Qt 5.15**, every platform except Linux (non-LSB) builds, which use `QT_LINUX_FUTEX`.
  - Affected on arm64: macOS (Apple Silicon), Windows on ARM64 (for example MSYS2 CLANGARM64
    `mingw-w64-clang-aarch64-qt5-base`), FreeBSD/OpenBSD/NetBSD on arm64, and QNX.
  - x86 is not affected in practice.
- **Qt 6:**
  - Windows has used WaitOnAddress futexes since 6.2 (qtbase 91f6460aff0a). So Windows on ARM64
    is affected with 6.0 and 6.1.
  - FreeBSD uses futexes (umtx).
  - macOS:
    - 6.0–6.6 use Mach semaphores, so they are affected on Apple Silicon.
    - 6.7–6.11 use the private `__ulock` futexes when they are available at run time
      (d2368cde7006), except in App Store-compliant builds, which stay affected.
    - 6.12+ always use `os_sync_wait_on_address` (7f20caf5fbf3) and are not affected.
  - Still compiled in `dev` for QNX, VxWorks, INTEGRITY, OpenBSD, NetBSD, Hurd, Solaris, LSB
    builds and wasm. All of them are affected on weakly ordered CPUs: arm64 (QNX, VxWorks,
    INTEGRITY, the BSDs), POWER and RISC-V.
