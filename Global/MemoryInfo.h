//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_GLOBAL_MEMORYINFO_H_
#define NATRON_GLOBAL_MEMORYINFO_H_

// Memory utility functions ( info about RAM etc...)

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm> // min, max
#include <stdexcept>

#if defined(_WIN32)
#  include <windows.h>
#  include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__ ) )
#  include <unistd.h>
#  include <sys/resource.h>
#  if defined(__APPLE__) && defined(__MACH__)
#    include <mach/mach.h>
#    include <mach/task.h>
#    include <mach/task_info.h>
#    include <sys/sysctl.h>
#    include <sys/statvfs.h>
#    include <stdexcept>
#    include <sys/errno.h>
#  elif (defined(_AIX) || defined(__TOS__AIX__ ) ) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__ )  ) )
#    include <fcntl.h>
#    include <procfs.h>
#  elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__) || defined(__FreeBSD__)
#    include <stdio.h>
#    include <unistd.h>
#    if defined(__FreeBSD__)
#      include <sys/sysctl.h>
#      include <sys/types.h>
#    else
#      include <sys/sysinfo.h>
#    endif
#  endif
#else
#  error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif

#include "Global/Macros.h"
#include <QtCore/QString>
#include <QtCore/QLocale>

inline uint64_t
getSystemTotalRAM()
{
#if defined(__APPLE__)
    int mib [] = {
        CTL_HW, HW_MEMSIZE
    };
    uint64_t value = 0;
    size_t length = sizeof(value);

    if ( -1 == sysctl(mib, 2, &value, &length, NULL, 0) ) {
        //error;
    }

    return value;

#elif defined(_WIN32)
    ///On Windows, but not Cygwin, the new GlobalMemoryStatusEx( ) function fills a 64-bit
    ///safe MEMORYSTATUSEX struct with information about physical and virtual memory. Structure fields include:
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);

    return status.ullTotalPhys;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__) || defined(__FreeBSD__)


    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);

    return pages * page_size;

#endif
}

inline bool
isApplication32Bits()
{
    return sizeof(void*) == 4;
}

inline uint64_t
getSystemTotalRAM_conditionnally()
{
    if ( isApplication32Bits() ) {
        return std::min( (uint64_t)0x100000000ULL, getSystemTotalRAM() );
    } else {
        return getSystemTotalRAM();
    }
}

// prints RAM value as KB, MB or GB
inline QString
printAsRAM(U64 bytes)
{
    // According to the Si standard KB is 1000 bytes, KiB is 1024
    // but on windows sizes are calulated by dividing by 1024 so we do what they do.
    const U64 kb = 1024;
    const U64 mb = 1024 * kb;
    const U64 gb = 1024 * mb;
    const U64 tb = 1024 * gb;

    if (bytes >= tb) {
        return QObject::tr("%1 TB").arg( QLocale().toString(qreal(bytes) / tb, 'f', 3) );
    }
    if (bytes >= gb) {
        return QObject::tr("%1 GB").arg( QLocale().toString(qreal(bytes) / gb, 'f', 2) );
    }
    if (bytes >= mb) {
        return QObject::tr("%1 MB").arg( QLocale().toString(qreal(bytes) / mb, 'f', 1) );
    }
    if (bytes >= kb) {
        return QObject::tr("%1 KB").arg( QLocale().toString( (uint)(bytes / kb) ) );
    }

    return QObject::tr("%1 byte(s)").arg( QLocale().toString( (uint)bytes ) );
}

/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
inline size_t
getPeakRSS( )
{
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );

    return (size_t)info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__ ) ) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__ )  ) )
    /* AIX and Solaris ------------------------------------------ */
    struct psinfo psinfo;
    int fd = -1;
    if ( ( fd = open( "/proc/self/psinfo", O_RDONLY ) ) == -1 ) {
        return (size_t)0L;      /* Can't open? */
    }
    if ( read( fd, &psinfo, sizeof(psinfo) ) != sizeof(psinfo) ) {
        close( fd );

        return (size_t)0L;      /* Can't read? */
    }
    close( fd );

    return (size_t)(psinfo.pr_rssize * 1024L);

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__ ) )
    /* BSD, Linux, and OSX -------------------------------------- */
    struct rusage rusage;
    getrusage( RUSAGE_SELF, &rusage );
#if defined(__APPLE__) && defined(__MACH__)

    return (size_t)rusage.ru_maxrss;
#else

    return (size_t)(rusage.ru_maxrss * 1024L);
#endif

#else

    /* Unknown OS ----------------------------------------------- */
    return (size_t)0L;          /* Unsupported. */
#endif
}

/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
inline size_t
getCurrentRSS( )
{
#if defined(_WIN32)
    /* Windows -------------------------------------------------- */
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );

    return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
    /* OSX ------------------------------------------------------ */
#  ifndef MACH_TASK_BASIC_INFO
    struct task_basic_info info;
    mach_msg_type_number_t infoCount = TASK_BASIC_INFO_COUNT;
    if (task_info( mach_task_self( ), TASK_BASIC_INFO,
                   (task_info_t)&info, &infoCount ) != KERN_SUCCESS) {
        return (size_t)0L;      /* Can't access? */
    }
#  else
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
                   (task_info_t)&info, &infoCount ) != KERN_SUCCESS) {
        return (size_t)0L;      /* Can't access? */
    }
#  endif

    return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
    /* Linux ---------------------------------------------------- */
    long rss = 0L;
    FILE* fp = NULL;
    if ( ( fp = fopen( "/proc/self/statm", "r" ) ) == NULL ) {
        return (size_t)0L;      /* Can't open? */
    }
    if (fscanf( fp, "%*s%ld", &rss ) != 1) {
        fclose( fp );

        return (size_t)0L;      /* Can't read? */
    }
    fclose( fp );

    return (size_t)rss * (size_t)sysconf( _SC_PAGESIZE);

#else

    /* AIX, BSD, Solaris, and Unknown OS ------------------------ */
    return (size_t)0L;          /* Unsupported. */
#endif
} // getCurrentRSS

inline size_t
getAmountFreePhysicalRAM()
{
#if defined(_WIN32)
    ///On Windows, but not Cygwin, the new GlobalMemoryStatusEx( ) function fills a 64-bit
    ///safe MEMORYSTATUSEX struct with information about physical and virtual memory. Structure fields include:
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);

    return statex.ullAvailPhys;
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
    struct sysinfo memInfo;
    sysinfo (&memInfo);
    long long totalAvailableRAM = memInfo.freeram;
    totalAvailableRAM *= memInfo.mem_unit;

    return totalAvailableRAM;
#elif defined(__FreeBSD__)
    unsigned long pagesize = getpagesize();
    u_int value;
    size_t value_size = sizeof(value);
    unsigned long long totalAvailableRAM;
    if (sysctlbyname("vm.stats.vm.v_free_count", &value, &value_size, NULL, 0) < 0) {
        throw std::runtime_error("Unable to get amount of free physical RAM");
    }
    totalAvailableRAM = value * (unsigned long long)pagesize;

    return totalAvailableRAM;
#elif defined(__APPLE__) && defined(__MACH__)
    mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
    vm_statistics_data_t vmstat;
    host_name_port_t hostName = mach_host_self();
    if ( KERN_SUCCESS != host_statistics(hostName, HOST_VM_INFO, (host_info_t)&vmstat, &count) ) {
        throw std::runtime_error("Unable to get amount of free physical RAM");
    }
    vm_size_t pageSize;
    if ( KERN_SUCCESS != host_page_size(hostName, &pageSize) ) {
        throw std::runtime_error("Unable to get amount of free physical RAM");
    }
    /**
     * Returning only the free_count is not useful because it seems munmap implementations tend to not increase the free_count
     * but the inactive_count instead
    **/
    return (vmstat.free_count + vmstat.inactive_count) * pageSize;
#endif
}

#endif // ifndef NATRON_GLOBAL_MEMORYINFO_H_
