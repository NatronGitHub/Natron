/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/MemoryInfo.h"

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
#include <sstream> // stringstream

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

#include <QtCore/QString>
#include <QtCore/QLocale>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include "Global/GlobalDefines.h"

NATRON_NAMESPACE_ENTER

U64
getSystemTotalRAM()
{
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__)
    // see http://source.winehq.org/git/wine.git/blob/HEAD:/dlls/kernel32/heap.c
    uint64_t total;
#ifdef __APPLE__
    unsigned int val;
#else
    unsigned long val;
#endif
    int mib[2];
    size_t size_sys;
#ifdef HW_MEMSIZE
    uint64_t val64 = 0;
#endif
    total = 0;

    mib[0] = CTL_HW;
#ifdef HW_MEMSIZE
    mib[1] = HW_MEMSIZE;
    size_sys = sizeof(val64);
    if (!sysctl(mib, 2, &val64, &size_sys, NULL, 0) && size_sys == sizeof(val64) && val64) {
        total = val64;
    }
#endif

#if defined(__MACH__)
    if (!total) {
        host_name_port_t host = mach_host_self();
        mach_msg_type_number_t count;

        host_basic_info_data_t info;
        count = HOST_BASIC_INFO_COUNT;
        if (host_info(host, HOST_BASIC_INFO, (host_info_t)&info, &count) == KERN_SUCCESS) {
            total = info.max_mem;
        }

        mach_port_deallocate(mach_task_self(), host);
    }
#endif

    if (!total) {
        mib[1] = HW_PHYSMEM;
        size_sys = sizeof(val);
        if (!sysctl(mib, 2, &val, &size_sys, NULL, 0) && size_sys == sizeof(val) && val) {
            total = val;
        }
    }
    
    return total;
    
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

U64
getSystemTotalRAM_conditionnally()
{
    if ( isApplication32Bits() ) {
        return std::min( (U64)0x100000000ULL, getSystemTotalRAM() );
    } else {
        return getSystemTotalRAM();
    }
}

// prints RAM value as KB, MB or GB
QString
printAsRAM(U64 bytes)
{
    // According to the Si standard KB is 1000 bytes, KiB is 1024
    // but on windows sizes are calulated by dividing by 1024 so we do what they do.
    const U64 kb = 1024;
    const U64 mb = 1024 * kb;
    const U64 gb = 1024 * mb;
    const U64 tb = 1024 * gb;

    if (bytes >= tb) {
        return QCoreApplication::translate("MemoryInfo", "%1 TiB").arg( QLocale().toString(qreal(bytes) / tb, 'f', 3) );
    }
    if (bytes >= gb) {
        return QCoreApplication::translate("MemoryInfo", "%1 GiB").arg( QLocale().toString(qreal(bytes) / gb, 'f', 2) );
    }
    if (bytes >= mb) {
        return QCoreApplication::translate("MemoryInfo", "%1 MiB").arg( QLocale().toString(qreal(bytes) / mb, 'f', 1) );
    }
    if (bytes >= kb) {
        return QCoreApplication::translate("MemoryInfo", "%1 KiB").arg( QLocale().toString( (uint)(bytes / kb) ) );
    }

    return QCoreApplication::translate("MemoryInfo", "%1 byte(s)").arg( QLocale().toString( (uint)bytes ) );
}


#if 0 // not used for now
/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
std::size_t
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
#endif // 0

#if 0 // not used for now
/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
std::size_t
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
#endif // 0


std::size_t
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
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__)
    // and http://source.winehq.org/git/wine.git/blob/HEAD:/dlls/kernel32/heap.c
    unsigned long long ullAvailPhys;
    unsigned long long ullTotalPhys = 0;
    uint64_t total;
#ifdef __APPLE__
    unsigned int val;
#else
    unsigned long val;
#endif
    int mib[2];
    size_t size_sys;
#ifdef HW_MEMSIZE
    uint64_t val64 = 0;
#endif
#if 0
#ifdef VM_SWAPUSAGE
    struct xsw_usage swap;
#endif
#endif

    total = 0;
    ullAvailPhys = 0;

    mib[0] = CTL_HW;
#ifdef HW_MEMSIZE
    mib[1] = HW_MEMSIZE;
    size_sys = sizeof(val64);
    if (!sysctl(mib, 2, &val64, &size_sys, NULL, 0) && size_sys == sizeof(val64) && val64)
        total = val64;
#endif

#if defined(__MACH__)
    // see http://opensource.apple.com/source/system_cmds/system_cmds-498.2/vm_stat.tproj/vm_stat.c
    {
        host_name_port_t host = mach_host_self();
        mach_msg_type_number_t count;

#ifdef HOST_VM_INFO64_COUNT
        vm_size_t page_size;
        vm_statistics64_data_t vm_stat;

        count = HOST_VM_INFO64_COUNT;
        if (host_statistics64(host, HOST_VM_INFO64, (host_info64_t)&vm_stat, &count) == KERN_SUCCESS &&
            host_page_size(host, &page_size) == KERN_SUCCESS) {
            ullAvailPhys = (vm_stat.free_count + vm_stat.inactive_count) * (unsigned long long)page_size;
        }
#endif
        if (!total) {
            host_basic_info_data_t info;
            count = HOST_BASIC_INFO_COUNT;
            if (host_info(host, HOST_BASIC_INFO, (host_info_t)&info, &count) == KERN_SUCCESS) {
                total = info.max_mem;
            }
        }

        mach_port_deallocate(mach_task_self(), host);
    }
#endif

    if (!total) {
        mib[1] = HW_PHYSMEM;
        size_sys = sizeof(val);
        if (!sysctl(mib, 2, &val, &size_sys, NULL, 0) && size_sys == sizeof(val) && val)
            total = val;
    }

    if (total) {
        ullTotalPhys = total;
    }

    if (!ullAvailPhys) {
        mib[1] = HW_USERMEM;
        size_sys = sizeof(val);
        if (!sysctl(mib, 2, &val, &size_sys, NULL, 0) && size_sys == sizeof(val) && val)
            ullAvailPhys = val;
    }

    if (!ullAvailPhys) {
        ullAvailPhys = ullTotalPhys;
    }

#if 0
    ullTotalPageFile = ullAvailPhys;
    ullAvailPageFile = ullAvailPhys;

#ifdef VM_SWAPUSAGE
    mib[0] = CTL_VM;
    mib[1] = VM_SWAPUSAGE;
    size_sys = sizeof(swap);
    if (!sysctl(mib, 2, &swap, &size_sys, NULL, 0) && size_sys == sizeof(swap))
    {
        lpmemex->ullTotalPageFile = swap.xsu_total;
        lpmemex->ullAvailPageFile = swap.xsu_avail;
    }
#endif
#endif

    return ullAvailPhys;
#endif
}

NATRON_NAMESPACE_EXIT
