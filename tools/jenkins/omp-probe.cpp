// OpenMP probe for macOS: does a compiler build, link and run an OpenMP program with MacPorts'
// libomp? Used by compiler-common.sh (to choose Apple clang), tools/tart/provision.sh and the
// check in INSTALL_MACOS.md. The dynamic schedule matters: Apple clang's code for it needs a
// recent libomp (e.g. __kmpc_dispatch_deinit), so an old libomp fails at link time.
//
//   /usr/bin/clang++ -std=c++17 -DREQUIRED_CPLUSPLUS=201703L -Xpreprocessor -fopenmp \
//       -I/opt/local/include/libomp omp-probe.cpp -o t -L/opt/local/lib/libomp -lomp && ./t
#include <omp.h>

#if defined(REQUIRED_CPLUSPLUS) && __cplusplus < REQUIRED_CPLUSPLUS
#error "the compiler doesn't support the required C++ standard"
#endif

int
main()
{
    int n = 0;
#pragma omp parallel for schedule(dynamic) reduction(+:n)
    for (int i = 0; i < 4; ++i) {
        n += i;
    }

    return n == 6 && omp_get_max_threads() > 0 ? 0 : 1;
}
