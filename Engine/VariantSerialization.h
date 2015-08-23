//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef _Engine_VariantSerialization_h_
#define _Engine_VariantSerialization_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Variant.h"

#include <cassert>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
CLANG_DIAG_OFF(unused-local-typedef)
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/split_member.hpp>
CLANG_DIAG_ON(unused-local-typedef)
GCC_DIAG_ON(unused-parameter)
#endif

template<class Archive>
void
Variant::save(Archive & ar,
              const unsigned int version) const
{
    Q_UNUSED(version);
    QVariant::Type t = type();
    std::string typeStr;
    switch (t) {
        case QVariant::Bool: {
            bool v = toBool();
            typeStr = "bool";
            ar & boost::serialization::make_nvp("Type",typeStr);
            ar & boost::serialization::make_nvp("Value",v);
            break;
        }
        case QVariant::Int: {
            int v = toInt();
            typeStr = "int";
            ar & boost::serialization::make_nvp("Type",typeStr);
            ar & boost::serialization::make_nvp("Value",v);
            break;
        }
        case QVariant::UInt: {
            int v = toUInt();
            typeStr = "uint";
            ar & boost::serialization::make_nvp("Type",typeStr);
            ar & boost::serialization::make_nvp("Value",v);
            break;
        }
        case QVariant::Double: {
            double v = toDouble();
            typeStr = "double";
            ar & boost::serialization::make_nvp("Type",typeStr);
            ar & boost::serialization::make_nvp("Value",v);
            break;
        }
        case QVariant::String: {
            typeStr = "string";
            ar & boost::serialization::make_nvp("Type",typeStr);
            std::string str = toString().toStdString();
            ar & boost::serialization::make_nvp("Value",str);
            break;
        }
        case QVariant::StringList: {
            typeStr = "stringlist";
            ar & boost::serialization::make_nvp("Type",typeStr);
            std::list<std::string> list;
            QStringList strList = toStringList();
            for (int i = 0; i < strList.size(); ++i) {
                list.push_back( strList.at(i).toStdString() );
            }
            ar & boost::serialization::make_nvp("Value",list);
            break;
        }
        default:
            typeStr = "null";
            ar & boost::serialization::make_nvp("Type",typeStr);
            break;
    } // switch
} // save

template<class Archive>
void
Variant::load(Archive & ar,
              const unsigned int version)
{
    Q_UNUSED(version);
    std::string typeStr;
    ar & boost::serialization::make_nvp("Type",typeStr);
    if (typeStr == "bool") {
        bool v;
        ar & boost::serialization::make_nvp("Value",v);
        setValue<bool>(v);
    } else if (typeStr == "int") {
        int v;
        ar & boost::serialization::make_nvp("Value",v);
        setValue<int>(v);
    } else if (typeStr == "uint") {
        int v;
        ar & boost::serialization::make_nvp("Value",v);
        setValue<unsigned int>(v);
    } else if (typeStr == "double") {
        double v;
        ar & boost::serialization::make_nvp("Value",v);
        setValue<double>(v);
    } else if (typeStr == "string") {
        std::string str;
        ar & boost::serialization::make_nvp("Value",str);
        setValue<QString>( QString( str.c_str() ) );
    } else if (typeStr == "stringlist") {
        std::list<std::string> list;
        ar & boost::serialization::make_nvp("Value",list);
        QStringList strList;
        for (std::list<std::string>::iterator it = list.begin(); it != list.end(); ++it) {
            strList.push_back( (*it).c_str() );
        }
        setValue<QStringList>(strList);
    } else {
        assert(typeStr == "null");
        //nothing
    }
}

//BOOST_SERIALIZATION_SPLIT_MEMBER()
// split member function serialize funcition into save/load
template<class Archive>
void
Variant::serialize(Archive &ar,
                   const unsigned int file_version)
{
    boost::serialization::split_member(ar, *this, file_version);
}

#endif // _Engine_VariantSerialization_h_
