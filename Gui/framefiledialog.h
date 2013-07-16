//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com

#ifndef __PowiterOsX__filedialog__
#define __PowiterOsX__filedialog__

#include <iostream>
#include <QtWidgets/QFileDialog>


class FrameFileDialog : public QFileDialog
{
    
    
public:
    
    FrameFileDialog(QWidget* parent,
               const QString &caption = QString(),
               const QString &directory = QString(),
               const QString &filter = QString());
    virtual ~FrameFileDialog();


};



#endif /* defined(__PowiterOsX__filedialog__) */
