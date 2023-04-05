 macports: keep the pin on libomp 11.1.0, although issue closed on LLVM side

llvm have moved to github issues, here's the corresponding github issue:
https://github.com/llvm/llvm-project/issues/49923
which was closed because they couldn't repro, but LightGBM users are reporting issues
similar to ours:
- https://github.com/microsoft/LightGBM/issues/4229#issuecomment-1056023292
- https://github.com/microsoft/LightGBM/issues/5764

let us keep the pin until someone confirms it's fixed
