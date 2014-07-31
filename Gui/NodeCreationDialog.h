//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */



#ifndef NODECREATIONDIALOG_H
#define NODECREATIONDIALOG_H


#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#ifndef Q_MOC_RUN
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

struct NodeCreationDialogPrivate;
class NodeCreationDialog : public QDialog
{
public:
    
    explicit NodeCreationDialog(QWidget* parent);
    
    virtual ~NodeCreationDialog() OVERRIDE;
    
    QString getNodeName() const;
    
private:

    
    virtual void keyPressEvent(QKeyEvent *e) OVERRIDE FINAL;
    
    boost::scoped_ptr<NodeCreationDialogPrivate> _imp;
    
    
};
#endif // NODECREATIONDIALOG_H
