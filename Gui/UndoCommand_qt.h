/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */


#ifndef UNDOCOMMAND_QT_H
#define UNDOCOMMAND_QT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <QUndoCommand>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Gui/GuiFwd.h"
NATRON_NAMESPACE_ENTER;

class UndoCommand_qt
: public QUndoCommand
{
    boost::shared_ptr<UndoCommand> _command;

public:

    UndoCommand_qt(const UndoCommandPtr& command);

    virtual ~UndoCommand_qt();

    virtual void redo() OVERRIDE FINAL;

    virtual void undo() OVERRIDE FINAL;

    virtual int id() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool mergeWith(const QUndoCommand* other) OVERRIDE FINAL WARN_UNUSED_RETURN;};

NATRON_NAMESPACE_EXIT;

#endif // UNDOCOMMAND_QT_H
