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

#ifndef NATRON_GUI_BUTTON_H_
#define NATRON_GUI_BUTTON_H_

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QPushButton>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

class Button : public QPushButton {
    
public:
    
    explicit Button(QWidget* parent = 0);
    
    explicit Button(const QString & text, QWidget * parent = 0);
    
    Button(const QIcon & icon, const QString & text, QWidget * parent = 0);
   
private:
    
    void initInternal();
};

#endif
