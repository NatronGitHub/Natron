//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

class QTextBrowser;
class QLabel;
class QTabWidget;
class QVBoxLayout;
class QHBoxLayout;
class Button;
class Gui;

class AboutWindow : public QDialog
{
    
    QVBoxLayout* _mainLayout;
    QLabel* _iconLabel;
    QTabWidget* _tabWidget;
    
    QTextBrowser* _aboutText;
    QTextBrowser* _libsText;
    QTextBrowser* _teamText;
    QTextBrowser* _licenseText;
    
    QWidget* _buttonContainer;
    QHBoxLayout* _buttonLayout;
    Button* _closeButton;
    Gui* _gui;
    
public:
    
    AboutWindow(Gui* gui,QWidget* parent = 0);
    
    void updateLibrariesVersions();
    
    virtual ~AboutWindow() {}
};

#endif // ABOUTWINDOW_H
