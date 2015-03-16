//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NATRON_GUI_LABEL_H_
#define NATRON_GUI_LABEL_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QLabel> // in QtGui on Qt4, in QtWidgets on Qt5
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"

namespace Natron {

class Label
    : public QLabel
{
    
    Q_OBJECT
    Q_PROPERTY(bool altered READ getAltered WRITE setAltered)

public:

    Label(const QString &text,
          QWidget *parent = 0,
          Qt::WindowFlags f = 0);

    Label(QWidget *parent = 0,
          Qt::WindowFlags f = 0);
    
    bool getAltered() const;
    
    void setAltered(bool a);
    
private:
    
    bool altered;
};

} // namespace Natron

#endif // NATRON_GUI_-LABEL_H_
