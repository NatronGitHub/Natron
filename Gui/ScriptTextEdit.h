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
#ifndef SCRIPTTEXTEDIT_H
#define SCRIPTTEXTEDIT_H

#include <QTextEdit>

class ScriptTextEdit : public QTextEdit
{
    Q_PROPERTY(bool isOutput READ getOuput WRITE setOutput)
    
public:
    
    ScriptTextEdit(QWidget* parent);
    
    virtual ~ScriptTextEdit();
    
    void setOutput(bool o);
    
    bool getOutput() const;
private:
    
    bool isOutput;
};

#endif // SCRIPTTEXTEDIT_H
