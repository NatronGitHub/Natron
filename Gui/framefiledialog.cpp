//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

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