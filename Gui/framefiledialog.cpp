//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 




#include "framefiledialog.h"

FrameFileDialog::FrameFileDialog(QWidget* parent,
                                 const QString &caption,
                                 const QString &directory,
                                 const QString &filter) : QFileDialog(parent,caption,directory,filter){
   // setOption(QFileDialog::DontUseNativeDialog);
    setViewMode(QFileDialog::Detail);
    setFileMode(ExistingFiles);
}
FrameFileDialog::~FrameFileDialog(){
    
}