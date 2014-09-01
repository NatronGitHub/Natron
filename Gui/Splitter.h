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


#ifndef SPLITTER_H
#define SPLITTER_H

#include <list>
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QSplitter>
#include <QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

/**
 * @class A thread-safe wrapper over QSplitter
 **/
class QString;
class Splitter
    : public QSplitter
{
public:

    Splitter(QWidget* parent = 0);

    Splitter(Qt::Orientation orientation,
             QWidget * parent = 0);

    virtual ~Splitter()
    {
    }

    void addWidget_mt_safe(QWidget * widget);

    QByteArray saveState_mt_safe() const;

    QString serializeNatron() const;

    void restoreNatron(const QString & serialization);

    void setSizes_mt_safe(const QList<int> & list);

    void setObjectName_mt_safe(const QString & str);

    QString objectName_mt_safe() const;

    void insertChild_mt_safe(int i,QWidget* w);

    void removeChild_mt_safe(QWidget* w);

    void getChildren_mt_safe(std::list<QWidget*> & children) const;

private:

    mutable QMutex _lock;
};

#endif // SPLITTER_H
