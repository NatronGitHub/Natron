#if __APPLE__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/errno.h>
#include <mach-o/dyld.h> // we could also use _NSGetExecutablePath(char* buf, uint32_t* bufsize);
#include <limits.h> // PATH_MAX

int main(int argc, char *argv[])
{
  char programPath[PATH_MAX];
  uint32_t buflen = PATH_MAX;
  _NSGetExecutablePath(programPath, &buflen);

  // append "-driver" to the program name
  strncat(programPath, "-driver", PATH_MAX - strlen(programPath) - 1);
  printf("buf = %s\n", programPath);
  /*
   * set the DYLD_LIBRARY_PATH environment variable to the directory containing only a link to libstdc++.6.dylib
   * and re-exec the binart. The re-exec is necessary as the DYLD_LIBRARY_PATH is only read at exec.
   */
  char *dyldLibraryPathDef;
  char *programDir = dirname(programPath);
  if (asprintf(&dyldLibraryPathDef, "DYLD_LIBRARY_PATH=%s/../Frameworks/libstdc++", programDir) == -1) {
    fprintf(stderr, "Could not allocate space for defining DYLD_LIBRARY_PATH environment variable\n");
    exit(1);
  }
  putenv(dyldLibraryPathDef);
  execv(programPath, argv); // note that argv is always NULL-terminated

  fprintf(stderr, "execv(%s(%d), %s(%d), ...) failed: %s\n", programPath, (int)strlen(programPath), argv[0], (int)strlen(argv[0]), strerror(errno));
  exit(1);
}
#endif
