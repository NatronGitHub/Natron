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


#ifndef SPLITTER_H
#define SPLITTER_H

#include <QSplitter>
#include <QMutex>

/**
 * @class A thread-safe wrapper over QSplitter
 **/
class Splitter : public QSplitter
{
public:
    
    Splitter(QWidget* parent = 0);
    
    Splitter(Qt::Orientation orientation, QWidget * parent = 0);
    
    virtual ~Splitter(){}
    
    void addWidget_mt_safe(QWidget * widget);
    
    QByteArray saveState_mt_safe() const;
    
    void setSizes_mt_safe(const QList<int> & list);
    
    void setObjectName_mt_safe(const QString& str);
    
    QString objectName_mt_safe() const;
private:
    
    virtual bool event(QEvent * e);
    
    mutable QMutex _lock;
};

#endif // SPLITTER_H
