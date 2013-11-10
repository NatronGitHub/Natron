//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef VARIANT_H
#define VARIANT_H

#include <QtCore/QVariant>
#include <QtCore/QMetaType>
/******************************VARIANT**************************************/

class Variant : public QVariant {
public:

    Variant()
    : QVariant()
    {
    }

    explicit Variant(const QVariant& qtVariant)
    : QVariant(qtVariant)
    {
    }

    template<typename T>
    Variant(T variant[] ,int count)
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
    T value() const {
        return QVariant::value<T>();
    }

    template<typename T>
    void setValue(const T &value) {
        QVariant::setValue(value);
    }

    template<typename T>
    void setValue(T variant[],int count){
        QList<QVariant> list;
        for (int i = 0; i < count; ++i) {
            list << QVariant(variant[i]);
        }
        QVariant::setValue(list);
    }

    int toInt(bool *ok = 0) const { return QVariant::toInt(ok); }
    uint toUInt(bool *ok = 0) const { return QVariant::toUInt(ok); }
    qlonglong toLongLong(bool *ok = 0) const { return QVariant::toLongLong(ok); }
    qulonglong toULongLong(bool *ok = 0) const { return QVariant::toULongLong(ok); }
    bool toBool() const { return QVariant::toBool(); }
    double toDouble(bool *ok = 0) const { return QVariant::toDouble(ok); }
    float toFloat(bool *ok = 0) const { return QVariant::toFloat(ok); }
    qreal toReal(bool *ok = 0) const { return QVariant::toReal(ok); }
    QByteArray toByteArray() const { return QVariant::toByteArray(); }
    //QBitArray toBitArray() const { return QVariant::toBitArray(); }
    QString toString() const { return QVariant::toString(); }
    QStringList toStringList() const { return QVariant::toStringList(); }
    QChar toChar() const { return QVariant::toChar(); }
    //QDate toDate() const { return QVariant::toDate(); }
    //QTime toTime() const { return QVariant::toTime(); }
    //QDateTime toDateTime() const { return QVariant::toDateTime(); }
    QList<QVariant> toList() const { return QVariant::toList(); }
    QMap<QString, QVariant> toMap() const { return QVariant::toMap(); }
    QHash<QString, QVariant> toHash() const { return QVariant::toHash(); }

    bool isNull() const{return QVariant::isNull();}
};
Q_DECLARE_METATYPE(Variant);

// specializations of setValue() for simple string types (to avoid conversions)
template<>
inline void Variant::setValue(const std::string& str){
    QVariant::setValue(QString(str.c_str()));
}

template<>
inline void Variant::setValue(const char* const& str){
    QVariant::setValue(QString(str));
}

#endif // VARIANT_H
