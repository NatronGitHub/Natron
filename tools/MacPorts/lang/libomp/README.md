# libomp: unpinned (2026-11-15)

From 2021 to 2026-11-15, Natron pinned libomp to 11.1.0. Natron's OpenMP plug-ins
(DenoiseSharpen, the CImg plug-ins, the ImageMagick-based openfx-arena plug-ins) call
OpenMP from the host's render threads, and libomp 12 crashed in that situation
(Natron issue [#672](https://github.com/NatronGitHub/Natron/issues/672), LLVM issue
[#49923](https://github.com/llvm/llvm-project/issues/49923)).

Investigation and minimal reproducers: https://github.com/devernay/libomp-repro

The overlay no longer has a libomp `Portfile`, so MacPorts uses the upstream port
(libomp 21.1.7 at the time of writing). The pinned 11.1.0 Portfile is kept as
`Portfile~20261115` (MacPorts ignores it) together with its patchfiles
(`files/patch-libomp11-*.diff`, `files/reviews.llvm.org_D88252.diff`). To restore the pin,
rename it back to `Portfile`.

## Findings

Two separate bugs, both fixed upstream, neither specific to x86_64:

1. **Foreign threads using OpenMP (the bug that affected Natron).** Plain pthreads that
   are created and destroyed while each runs `#pragma omp parallel` make libomp 12.x
   abort with `OMP: Error #131: Thread identifier invalid`, or with
   `Assertion failure at kmp_runtime.cpp(3689)` in `__kmp_register_root`, or crash with
   SIGSEGV. This happens on both arm64 and x86_64. On non-Linux systems, 12.x disabled
   hidden helper threads but still reserved 8 gtids for them. The bug was fixed by
   llvm-project
   [f1b9ce2736d8](https://github.com/llvm/llvm-project/commit/f1b9ce2736d826df2ff4cbd67116864f0856334f)
   ("[OpenMP] Fix a few issues with hidden helper task", D105308), first released in
   **13.0.0**.
2. **Debug libomp crashes at exit.** This is what the conan llvm-openmp recipe's comment
   "All versions except for v11 crash with a segfault for the simple test_package.cpp test"
   refers to. It is why this README used to say the bug persisted until 17. Only
   **Debug** builds are affected: a `KMP_DEBUG`-only trace call in
   `__kmp_release_template` dereferences the freed `__kmp_threads[0]` in
   `__kmp_get_global_thread_id` during `exit()`. Release builds, including the MacPorts
   one, never had this bug. It was fixed by
   [35fdf8d70329](https://github.com/llvm/llvm-project/commit/35fdf8d70329b2b067cb9433a5891f2a57e2f4ed)
   ("[OpenMP] Fix a segment fault in __kmp_get_global_thread_id", D159324), first released
   in **18.1.0**.

Release builds tested on macOS 27, Apple M5 Pro (x86_64 under Rosetta 2). The two bugs
were each bisected to the commits above.

| libomp | repro (bug 1) | stress | test_package, Debug arm64 (bug 2) |
|---|---|---|---|
| 11.1.0 | ok | ok | ok |
| 12.0.1 | **fails** (arm64 and x86_64) | | **fails** |
| 13.0.x, 14.0.6, 17.0.6 | ok | | **fails** (13.0.0, 17.0.6) |
| 18.1.8, 19.1.7, 20.1.8 | ok (20.1.8) | | ok |
| 21.1.7 (MacPorts), 22.1.8, 23.1.5, main | ok | ok | ok |

Later reports of crashes or hangs with libomp 14 or 15 (xgboost, LightGBM) do not
reproduce with these tests. They are probably different issues, such as two libomp
copies loaded in the same process.

## Upgrading beyond 21.1.7

22.1.8 also passes every test. Starting with LLVM 22, the release no longer provides the
`openmp-*.src.tar.xz`/`cmake-*.src.tar.xz` tarballs, only `llvm-project-*.src.tar.xz`.
LLVM 23 also removes the standalone `openmp/` CMake build in favour of
`runtimes/ -DLLVM_ENABLE_RUNTIMES=openmp`. Follow upstream MacPorts instead of doing
this in the overlay.

## Testing

```
git clone https://github.com/devernay/libomp-repro && cd libomp-repro
LLVM_PROJECT=/path/to/llvm-project ./build-libomp.sh llvmorg-21.1.7 arm64
./runtest.sh 21.1.7 arm64 5
```

The tests can also run against the installed MacPorts libomp:

```
clang-mp-22 -fopenmp repro.c -o repro && ./repro 30 32 10
```
