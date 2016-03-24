
/***************************************************************************/
/*           HERE DEFINE THE PROJECT SPECIFIC PUBLIC MACROS                */
/*    These are only in effect in a setting that doesn't use configure     */
/***************************************************************************/

/* Version number of project */
#define COINUTILS_VERSION "2.9.3"

/* Major Version number of project */
#define COINUTILS_VERSION_MAJOR 2

/* Minor Version number of project */
#define COINUTILS_VERSION_MINOR 9

/* Release Version number of project */
#define COINUTILS_VERSION_RELEASE 3

/*
  Define to 64bit integer types. Note that MS does not provide __uint64.

  Microsoft defines types in BaseTsd.h, part of the Windows SDK. Given
  that this file only gets used in the Visual Studio environment, it
  seems to me we'll be better off simply including it and using the
  types MS defines. But since I have no idea of history here, I'll leave
  all of this inside the guard for MSC_VER >= 1200. If you're reading this
  and have been developing in MSVS long enough to know, fix it.  -- lh, 100915 --
*/
#if _MSC_VER >= 1200
# include <BaseTsd.h>
# define COIN_INT64_T INT64
# define COIN_UINT64_T UINT64
  /* Define to integer type capturing pointer */
# define COIN_INTPTR_T ULONG_PTR
#else
# define COIN_INT64_T long long
# define COIN_UINT64_T unsigned long long
# define COIN_INTPTR_T int*
#endif
