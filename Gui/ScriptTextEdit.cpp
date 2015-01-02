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
#include "ScriptTextEdit.h"
#include <QStyle>

ScriptTextEdit::ScriptTextEdit(QWidget* parent)
: QTextEdit(parent)
, isOutput(false)
{
}

ScriptTextEdit::~ScriptTextEdit()
{
    
}

void
ScriptTextEdit::setOutput(bool o)
{
    isOutput = o;
    style()->unpolish(this);
    style()->polish(this);
    repaint();
}

bool
ScriptTextEdit::getOutput() const
{
    return isOutput;
}