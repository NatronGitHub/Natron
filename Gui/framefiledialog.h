//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
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
