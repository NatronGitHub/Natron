//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Label.h"

#include <QApplication>

using namespace Natron;

Label::Label(const QString &text,
             QWidget *parent,
             Qt::WindowFlags f)
    : QLabel(text, parent, f)
{
    setFont(QApplication::font()); // necessary, or the labels will get the default font size
}

Label::Label(QWidget *parent,
             Qt::WindowFlags f)
    : QLabel(parent, f)
{
    setFont(QApplication::font()); // necessary, or the labels will get the default font size
}
