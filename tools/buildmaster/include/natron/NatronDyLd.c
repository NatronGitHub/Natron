#if __APPLE__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/errno.h>
#include <mach-o/dyld.h> // we could also use _NSGetExecutablePath(char* buf, uint32_t* bufsize);
#include <limits.h> // PATH_MAX
#include <spawn.h> // posix_spawn
#include <err.h> // err

extern char** environ;

/* from http://svn.python.org/projects/python/trunk/Mac/Tools/pythonw.c */
static void
setup_spawnattr(posix_spawnattr_t* spawnattr)
{
  size_t ocount;
  size_t count;
  cpu_type_t cpu_types[1];
  short flags = 0;
#ifdef __LP64__
  int   ch;
#endif

  if ((errno = posix_spawnattr_init(spawnattr)) != 0) {
    err(2, "posix_spawnattr_int");
    /* NOTREACHTED */
  }

  count = 1;

  /* Run the real Natron executable using the same architure as this
   * executable, this allows users to controle the architecture using
   * "arch -ppc Natron"
   */

#if defined(__ppc64__)
  cpu_types[0] = CPU_TYPE_POWERPC64;

#elif defined(__x86_64__)
  cpu_types[0] = CPU_TYPE_X86_64;

#elif defined(__ppc__)
  cpu_types[0] = CPU_TYPE_POWERPC;
#elif defined(__i386__)
  cpu_types[0] = CPU_TYPE_X86;
#else
#       error "Unknown CPU"
#endif

  if (posix_spawnattr_setbinpref_np(spawnattr, count,
                                    cpu_types, &ocount) == -1) {
    err(1, "posix_spawnattr_setbinpref");
    /* NOTREACHTED */
  }
  if (count != ocount) {
    fprintf(stderr, "posix_spawnattr_setbinpref failed to copy\n");
    exit(1);
    /* NOTREACHTED */
  }


  /*
   * Set flag that causes posix_spawn to behave like execv
   */
  flags |= POSIX_SPAWN_SETEXEC;
  if ((errno = posix_spawnattr_setflags(spawnattr, flags)) != 0) {
    err(1, "posix_spawnattr_setflags");
    /* NOTREACHTED */
  }
}

int main(int argc, char *argv[])
{
  char exec_path[PATH_MAX];
  uint32_t buflen = PATH_MAX;
  _NSGetExecutablePath(exec_path, &buflen);

  // append "-driver" to the program name
  strncat(exec_path, "-driver", PATH_MAX - strlen(exec_path) - 1);
  printf("buf = %s\n", exec_path);
  char *dyldLibraryPathDef = 0;
  char *programDir = dirname(exec_path);
#if defined(LIBGCC)
  /*
   * set the DYLD_LIBRARY_PATH environment variable to the directory containing only a link to libstdc++.6.dylib
   * and re-exec the binary. The re-exec is necessary as the DYLD_LIBRARY_PATH is only read at exec.
   */
  if (asprintf(&dyldLibraryPathDef, "DYLD_LIBRARY_PATH=%s/../Frameworks/libgcc", programDir) == -1) {
    fprintf(stderr, "Could not allocate space for defining DYLD_LIBRARY_PATH environment variable\n");
    exit(1);
  }
#elif defined(LIBCXX)
  /*
   * set the DYLD_FALLBACK_LIBRARY_PATH environment variable to the directory containing only the libc++.1.dylib
   * and re-exec the binary. The re-exec is necessary as the DYLD_FALLBACK_LIBRARY_PATH is only read at exec.
   */
  if (asprintf(&dyldLibraryPathDef, "DYLD_FALLBACK_LIBRARY_PATH=%s/../Frameworks/libcxx", programDir) == -1) {
    fprintf(stderr, "Could not allocate space for defining DYLD_FALLBACK_LIBRARY_PATH environment variable\n");
    exit(1);
  }
#else
#error "define LIBGCC or LIBCXX"
#endif
  putenv(dyldLibraryPathDef);

  /* We're weak-linking to posix-spawnv to ensure that
   * an executable build on 10.5 can work on 10.4.
   */
  if (&posix_spawn != NULL) {
    posix_spawnattr_t spawnattr = NULL;


    setup_spawnattr(&spawnattr);
    posix_spawn(NULL, exec_path, NULL,
                &spawnattr, argv, environ);
    err(1, "posix_spawn: %s", exec_path);
  }

  execve(exec_path, argv, environ); // note that argv is always NULL-terminated

  err(1, "execv(%s(%d), %s(%d), ...) failed", exec_path, (int)strlen(exec_path), argv[0], (int)strlen(argv[0]));
}
#endif
