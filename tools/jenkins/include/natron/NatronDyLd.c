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
#include <sys/sysctl.h> // sysctlbyname

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
   * executable, this allows users to control the architecture using
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

// from https://stackoverflow.com/a/11697362
// (tested on Snow Leopard and Catalina)

// Darwin version to OS X release:

// 22.x.x  macOS 13.x    Ventura
// 21.x.x  macOS 12.x    Monterey
// 20.x.x  macOS 11.x    Big Sur
// 19.x.x  macOS 10.15.x Catalina
// 18.x.x  macOS 10.14.x Mojave
// 17.x.x. macOS 10.13.x High Sierra
// 16.x.x  macOS 10.12.x Sierra
// 15.x.x  OS X  10.11.x El Capitan
// 14.x.x  OS X  10.10.x Yosemite
// 13.x.x  OS X  10.9.x  Mavericks
// 12.x.x  OS X  10.8.x  Mountain Lion
// 11.x.x  OS X  10.7.x  Lion
// 10.x.x  OS X  10.6.x  Snow Leopard
//  9.x.x  OS X  10.5.x  Leopard
//  8.x.x  OS X  10.4.x  Tiger
//  7.x.x  OS X  10.3.x  Panther
//  6.x.x  OS X  10.2.x  Jaguar
//  5.x    OS X  10.1.x  Puma

#ifdef LIBCXX
/* kernel version as major minor component*/
struct kern {
  short int version[3];
};

/* return the kernel version */
void GetKernelVersion(struct kern *k) {
  static short int version_[3] = {0};
  if (!version_[0]) {
    // just in case it fails someday
    version_[0] = version_[1] = version_[2] = -1;
    char str[256] = {0};
    size_t size = sizeof(str);
    int ret = sysctlbyname("kern.osrelease", str, &size, NULL, 0);
    if (ret == 0) sscanf(str, "%hd.%hd.%hd", &version_[0], &version_[1], &version_[2]);
  }
  memcpy(k->version, version_, sizeof(version_));
}

/* compare os version with a specific one
   0 is equal
   negative value if the installed version is less
   positive value if the installed version is more
*/
int CompareKernelVersion(short int major, short int minor, short int component) {
  struct kern k;
  GetKernelVersion(&k);
  if ( k.version[0] !=  major) return major - k.version[0];
  if ( k.version[1] !=  minor) return minor - k.version[1];
  if ( k.version[2] !=  component) return component - k.version[2];
  return 0;
}
#endif

int main(int argc, char *argv[])
{
  char exec_path[PATH_MAX];
  uint32_t buflen = PATH_MAX;
  _NSGetExecutablePath(exec_path, &buflen);

  // append "-driver" to the program name
  strncat(exec_path, "-driver", PATH_MAX - strlen(exec_path) - 1);
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
  // std::length_error::~length_error() is only defined in libc++abi.dylib on 10.9 (13.0.0) and later.
  // see https://github.com/NatronGitHub/Natron/issues/431
  // We this force linking with the provided libc++ on previous versions of OS X.
  if (CompareKernelVersion(13, 0, 0) > 0) {  // if 10.9.0 > OS X version
    /*
     * set the DYLD_LIBRARY_PATH environment variable to the directory containing only the libc++.1.dylib
     * and re-exec the binary. The re-exec is necessary as the DYLD_LIBRARY_PATH is only read at exec.
     */
    if (asprintf(&dyldLibraryPathDef, "DYLD_LIBRARY_PATH=%s/../Frameworks/libcxx", programDir) == -1) {
      fprintf(stderr, "Could not allocate space for defining DYLD_LIBRARY_PATH environment variable\n");
      exit(1);
    }
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
