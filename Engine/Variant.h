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

#ifndef NATRON_ENGINE_VARIANT_H_
#define NATRON_ENGINE_VARIANT_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QVariant>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QMetaType>
#include <QtCore/QDataStream>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "Global/Macros.h"
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)

#pragma message WARN("move serialization to a separate header")
CLANG_DIAG_OFF(unused-local-typedef)
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
CLANG_DIAG_ON(unused-local-typedef)
GCC_DIAG_ON(unused-parameter)
#include <boost/serialization/list.hpp>
#include <boost/serialization/split_member.hpp>
#endif
class Variant
    : public QVariant
{
    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar,
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
    void load(Archive & ar,
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

    BOOST_SERIALIZATION_SPLIT_MEMBER()

public:

    Variant()
        : QVariant()
    {
    }

    explicit Variant(const QVariant & qtVariant)
        : QVariant(qtVariant)
    {
    }

    template<typename T>
    Variant(T variant[],
            int count)
    {
        QList<QVariant> list;
        for (int i = 0; i < count; ++i) {
            list << QVariant(variant[i]);
        }
        QVariant::setValue(list);
    }

    virtual ~Variant()
    {
    }

    template<typename T>
    T value() const
    {
        return QVariant::value<T>();
    }

    template<typename T>
    void setValue(const T &value)
    {
        QVariant::setValue(value);
    }

    template<typename T>
    void setValue(T variant[],
                  int count)
    {
        QList<QVariant> list;
        for (int i = 0; i < count; ++i) {
            list << QVariant(variant[i]);
        }
        QVariant::setValue(list);
    }

    int toInt(bool *ok = 0) const
    {
        return QVariant::toInt(ok);
    }

    uint toUInt(bool *ok = 0) const
    {
        return QVariant::toUInt(ok);
    }

    qlonglong toLongLong(bool *ok = 0) const
    {
        return QVariant::toLongLong(ok);
    }

    qulonglong toULongLong(bool *ok = 0) const
    {
        return QVariant::toULongLong(ok);
    }

    bool toBool() const
    {
        return QVariant::toBool();
    }

    double toDouble(bool *ok = 0) const
    {
        return QVariant::toDouble(ok);
    }

    float toFloat(bool *ok = 0) const
    {
        return QVariant::toFloat(ok);
    }

    qreal toReal(bool *ok = 0) const
    {
        return QVariant::toReal(ok);
    }

    QByteArray toByteArray() const
    {
        return QVariant::toByteArray();
    }

    //QBitArray toBitArray() const { return QVariant::toBitArray(); }
    QString toString() const
    {
        return QVariant::toString();
    }

    QStringList toStringList() const
    {
        return QVariant::toStringList();
    }

    QChar toChar() const
    {
        return QVariant::toChar();
    }

    //QDate toDate() const { return QVariant::toDate(); }
    //QTime toTime() const { return QVariant::toTime(); }
    //QDateTime toDateTime() const { return QVariant::toDateTime(); }
    QList<QVariant> toList() const
    {
        return QVariant::toList();
    }

    QMap<QString, QVariant> toMap() const
    {
        return QVariant::toMap();
    }

    QHash<QString, QVariant> toHash() const
    {
        return QVariant::toHash();
    }

    bool isNull() const
    {
        return QVariant::isNull();
    }
};

Q_DECLARE_METATYPE(Variant);

// specializations of setValue() for simple string types (to avoid conversions)
template<>
inline void
Variant::setValue(const std::string & str)
{
    QVariant::setValue( QString( str.c_str() ) );
}

template<>
inline void
Variant::setValue(const char* const & str)
{
    QVariant::setValue( QString(str) );
}

#endif // NATRON_ENGINE_VARIANT_H_
