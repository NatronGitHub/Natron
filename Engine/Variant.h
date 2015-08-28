/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef _Engine_Variant_h_
#define _Engine_Variant_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

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

class Variant
    : public QVariant
{
#if 0 // Variant is never serialized (although serialization is available from VariantSerialization.h)
    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const;

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version);

    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int version);
#endif

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

#endif // _Engine_Variant_h_
