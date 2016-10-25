/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#ifndef AnimationModuleEditorUNDOREDO_H
#define AnimationModuleEditorUNDOREDO_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
#include <QUndoCommand> // in QtGui on Qt4, in QtWidgets on Qt5
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Enums.h"

#include "Engine/Transform.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

#if 0

class DSMoveKeysAndNodesCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(DSMoveKeysAndNodesCommand)

public:
    DSMoveKeysAndNodesCommand(const AnimKeyFramePtrList &keys,
                              const std::vector<NodeAnimPtr >& nodes,
                              double dt,
                              AnimationModuleEditor *model,
                              QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const OVERRIDE FINAL;
    bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;

private:
    /**
     * @brief Move the selected keyframes by 'dt' on the dope sheet timeline.
     */
    void moveSelection(double dt);

private:
    AnimKeyFramePtrList _keys;
    std::vector<NodeAnimPtr > _nodes;
    NodesWList _allDifferentNodes;
    double _dt;
    AnimationModuleEditor *_model;
};




/**
 * @brief The DSPasteKeysCommand class describes an undoable action that paste
 * the keyframes contained in the dope sheet clipboard.
 *
 * A undo removes the pasted keyframes.
 */
class DSPasteKeysCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(DSPasteKeysCommand)

public:
    DSPasteKeysCommand(const std::vector<AnimKeyFrame> &keys,
                       const std::list<KnobAnimPtr >& dstKnobs,
                       bool pasteRelativeToRefTime,
                       AnimationModuleEditor *model,
                       QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;


    static void setKeyValueFromKnob(const KnobIPtr& knob, double keyTime, KeyFrame* key);
private:
    /**
     * @brief If 'add' is true, paste the keyframes contained in the dope
     * sheet clipboard at the time of the current frame indicator.
     *
     * Otherwise delete the keys.
     */
    void addOrRemoveKeyframe(bool add);

private:

    int _refTime;
    int _refKeyindex;
    bool _pasteRelativeToRefTime;
    std::vector<AnimKeyFrame> _keys;
    std::list<boost::weak_ptr<KnobAnim> > _dstKnobs;
    AnimationModuleEditor *_model;
};
#endif

NATRON_NAMESPACE_EXIT;

#endif // AnimationModuleEditorUNDOREDO_H
