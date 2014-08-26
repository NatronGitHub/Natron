//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef SHORTCUTEDITOR_H
#define SHORTCUTEDITOR_H

#include <boost/scoped_ptr.hpp>

#include <QWidget>
#include "Global/Macros.h"
#include "Gui/LineEdit.h"

struct ShortCutEditorPrivate;
class ShortCutEditor : public QWidget
{
    Q_OBJECT
public:
    
    ShortCutEditor(QWidget* parent);
    
    virtual ~ShortCutEditor();
    
public slots:
    
    void onSelectionChanged();
    
    void onResetButtonClicked();
    
    void onRestoreDefaultsButtonClicked();
    
    void onApplyButtonClicked();
    
    void onCancelButtonClicked();
    
    void onOkButtonClicked();
    
private:
        
    boost::scoped_ptr<ShortCutEditorPrivate> _imp;
};

class KeybindRecorder : public LineEdit
{
public:
    
    KeybindRecorder(QWidget* parent);
    
    virtual ~KeybindRecorder();
    
private:
    
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
};

#endif // SHORTCUTEDITOR_H
