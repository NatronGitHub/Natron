 macports: keep the pin on libomp 11.1.0, although issue closed on LLVM side

llvm have moved to github issues, here's the corresponding github issue:
https://github.com/llvm/llvm-project/issues/49923
which was closed because they couldn't repro, but LightGBM users are reporting issues
similar to ours:
- https://github.com/microsoft/LightGBM/issues/4229#issuecomment-1056023292
- https://github.com/microsoft/LightGBM/issues/5764

According to the following, issues persist at least until libomp-17.0.rc4
https://github.com/conan-io/conan-center-index/pull/19857#issuecomment-1882905215
Apparently, conan's test_package.cpp would be a good unit test:
https://github.com/conan-io/conan-center-index/tree/master/recipes/llvm-openmp/all/test_package

let us keep the pin until someone confirms it's fixed
