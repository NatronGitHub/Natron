/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/ProcInfo.h"

#ifdef Q_OS_MAC

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <libproc.h>
#include <sys/sysctl.h>


/*
 Copied from Qt qcore_mac_p.h
 Helper class that automates refernce counting for CFtypes.
 After constructing the QCFType object, it can be copied like a
 value-based type.

 Note that you must own the object you are wrapping.
 This is typically the case if you get the object from a Core
 Foundation function with the word "Create" or "Copy" in it. If
 you got the object from a "Get" function, either retain it or use
 constructFromGet(). One exception to this rule is the
 HIThemeGet*Shape functions, which in reality are "Copy" functions.
 */
template <typename T>
class NatronCFType
{
public:
    inline NatronCFType(const T &t = 0)
    : type(t) {}

    inline NatronCFType(const NatronCFType &helper)
    : type(helper.type)
    {
        if (type) {CFRetain(type); }
    }

    inline ~NatronCFType()
    {
        if (type) {CFRelease(type); }
    }

    inline operator T() { return type; }

    inline NatronCFType operator =(const NatronCFType &helper)
    {
        if (helper.type) {
            CFRetain(helper.type);
        }
        CFTypeRef type2 = type;
        type = helper.type;
        if (type2) {
            CFRelease(type2);
        }

        return *this;
    }

    inline T *operator&() { return &type; }

    template <typename X>
    X as() const { return reinterpret_cast<X>(type); }

    static NatronCFType constructFromGet(const T &t)
    {
        CFRetain(t);

        return NatronCFType<T>(t);
    }

protected:
    T type;
};


class NatronCFString
: public NatronCFType<CFStringRef>
{
public:
    inline NatronCFString(const QString &str)
    : NatronCFType<CFStringRef>(0), string(str) {}

    inline NatronCFString(const CFStringRef cfstr = 0)
    : NatronCFType<CFStringRef>(cfstr) {}

    inline NatronCFString(const NatronCFType<CFStringRef> &other)
    : NatronCFType<CFStringRef>(other) {}

    operator QString() const;
    operator CFStringRef() const;
    static QString toQString(CFStringRef cfstr);
    static CFStringRef toCFStringRef(const QString &str);

private:
    QString string;
};

QString
NatronCFString::toQString(CFStringRef str)
{
    if (!str) {
        return QString();
    }

    CFIndex length = CFStringGetLength(str);
    if (length == 0) {
        return QString();
    }

    QString string(length, Qt::Uninitialized);
    CFStringGetCharacters( str, CFRangeMake(0, length), reinterpret_cast<UniChar *>( const_cast<QChar *>( string.unicode() ) ) );

    return string;
} // toQString

NatronCFString::operator QString() const
{
    if (string.isEmpty() && type) {
        const_cast<NatronCFString*>(this)->string = toQString(type);
    }

    return string;
}

CFStringRef
NatronCFString::toCFStringRef(const QString &string)
{
    return CFStringCreateWithCharacters( 0, reinterpret_cast<const UniChar *>( string.unicode() ),
                                        string.length() );
}

NatronCFString::operator CFStringRef() const
{
    if (!type) {
        const_cast<NatronCFString*>(this)->type =
        CFStringCreateWithCharactersNoCopy(0,
                                           reinterpret_cast<const UniChar *>( string.unicode() ),
                                           string.length(),
                                           kCFAllocatorNull);
    }
    
    return type;
}
NATRON_NAMESPACE_ENTER;

QString
ProcInfo::applicationFileName_mac()
{
    static QString appFileName;

    if ( appFileName.isEmpty() ) {
        NatronCFType<CFURLRef> bundleURL( CFBundleCopyExecutableURL( CFBundleGetMainBundle() ) );
        if (bundleURL) {
            NatronCFString cfPath( CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle) );
            if (cfPath) {
                appFileName = cfPath;
            }
        }
    }

    return appFileName;
}

NATRON_NAMESPACE_EXIT;


#endif
