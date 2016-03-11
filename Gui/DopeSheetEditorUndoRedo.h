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

#ifndef DOPESHEETEDITORUNDOREDO_H
#define DOPESHEETEDITORUNDOREDO_H

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
#include <QUndoCommand> // in QtGui on Qt4, in QtWidgets on Qt5
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Enums.h"

#include "Engine/Transform.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

typedef boost::shared_ptr<DopeSheetKey> DSKeyPtr;
typedef std::list<DSKeyPtr> DSKeyPtrList;


/**
 * Here you put the definitions of all your dope sheet undo commands.
 *
 *
 * -- How to --
 *
 * Natron use the Qt undo/redo framework as a undo system.
 *
 * Firstly, define a class that inherit from QUndoCommand. Create a new
 * function that will perform the undo action and the redo. Override
 * the undo() and redo() functions by using the function you just created.
 *
 * If you want to make your custom command compressible, define a new
 * compression ID in <NatronSources>/Engine/Global/GlobalDefines.h. Override
 * id() by returning this ID. Finally, override mergeWith() to define the
 * merge behavior.
 *
 * See the Qt documentation to learn more about the command compression.
 *
 *
 * -- Recommandations --
 *
 * As a DSNode, DSKnob, Node, NodeGui, KnobI, etc...is handled as a shared_ptr
 * by its owner class, if you want to store and use an instance in your custom
 * command, you must use a weak_ptr. In your un/redo function, use the
 * weak_ptr::lock() function to retrieve a shared_ptr to your object, then
 * test its value. If it's false, then return.
 *
 * In example, this check is useful when undoing a move keyframe action. If
 * the user closes the settings panel of the node that containing some selected
 * keys, it means that the node context doesn't exist anymore in the dope sheet
 * editor. This check prevent a crash in the case of a undo.
 *
 *
 * Storing a pointer to the dope sheet model in your class is necessary to
 * refresh the dope sheet view after an action. In most cases just emit the
 * DopeSheet::modelChanged() signal. Sometimes it's not necessary : since
 * the dope sheet view manages the node ranges it automatically update itself
 * when an important knob is modified. E.g. the DSLeftTrimReaderCommand doesn't
 * know anything about the DopeSheet class because when the first frame knob's
 * value is changed, the range of the reader is recomputed and it triggers a
 * refresh of the dope sheet view.
 *
 *
 * -- Available actions --
 *
 * The following actions are available as undo commands :
 *
 * - move the selected keyframes
 * - trim a reader by left
 * - trim a reader by right
 * - slip a reader on its original frame range
 * - move a reader
 * - delete the selected keyframes
 * - move a entire group
 * - change the interpolation of the selected keys
 * - paste the keyframes actually present in the clipboard
 */


/**
 * @brief The DSMoveKeysAndNodesCommand class describes an action that moves
 * the current selected keyframes on the dope sheet timeline.
 */
class DSMoveKeysAndNodesCommand : public QUndoCommand
{
public:
    DSMoveKeysAndNodesCommand(const DSKeyPtrList &keys,
                                const std::vector<boost::shared_ptr<DSNode> >& nodes,
                                double dt,
                                DopeSheetEditor *model,
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
    DSKeyPtrList _keys;
    std::vector<boost::shared_ptr<DSNode> > _nodes;
    NodesWList  _allDifferentNodes;
    double _dt;
    DopeSheetEditor *_model;
};


class DSTransformKeysCommand : public QUndoCommand
{
public:
    DSTransformKeysCommand(const DSKeyPtrList &keys,
                           const Transform::Matrix3x3& transform,
                           DopeSheetEditor *model,
                           QUndoCommand *parent = 0);
    
    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;
    
    int id() const OVERRIDE FINAL;
    bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;
    
private:
    /**
     * @brief Move the selected keyframes by 'dt' on the dope sheet timeline.
     */
    void transformKey(const DSKeyPtr& key);
    
private:
    
    
    struct TransformKeyData
    {
        DSKeyPtrList keys;
        boost::shared_ptr<Curve> oldCurve;
        boost::shared_ptr<Curve> newCurve;
    };
    
    typedef std::map<boost::shared_ptr<DSKnob>,TransformKeyData> TransformKeys;
    
    bool _firstRedoCalled;
    Transform::Matrix3x3 _transform;
    TransformKeys _keys;
    DopeSheetEditor *_model;
};



/**
 * @brief The DSLeftTrimReaderCommand class class describes an undoable action
 * that trim a reader by changing the value of its first frame.
 */
class DSLeftTrimReaderCommand : public QUndoCommand
{
public:
    DSLeftTrimReaderCommand(const boost::shared_ptr<DSNode> &reader,
                            double oldTime,
                            double newTime,
                            QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const OVERRIDE FINAL;
    bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;

private:
    /**
     * @brief Set the first frame of the handled reader to 'firstFrame'.
     */
    void trimLeft(double firstFrame);

private:
    boost::weak_ptr<DSNode> _readerContext;
    double _oldTime;
    double _newTime;
};


/**
 * @brief The DSRightTrimReaderCommand class describes an undoable action that
 * trim a reader by changing the value of its last frame.
 */
class DSRightTrimReaderCommand : public QUndoCommand
{
public:
    DSRightTrimReaderCommand(const boost::shared_ptr<DSNode> &reader,
                             double oldTime,
                             double newTime,
                             DopeSheetEditor * /*model*/,
                             QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const OVERRIDE FINAL;
    bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;

private:
    /**
     * @brief Set the last frame of the handled reader to 'lastFrame'.
     */
    void trimRight(double lastFrame);

private:
    boost::weak_ptr<DSNode> _readerContext;
    double _oldTime;
    double _newTime;
};

/**
 * @brief The DSSlipReaderCommand class describes an undoable action that trim
 * a reader by changing both its first and last frames.
 */
class DSSlipReaderCommand : public QUndoCommand
{
public:
    DSSlipReaderCommand(const boost::shared_ptr<DSNode> &reader,
                        double dt,
                        DopeSheetEditor *model,
                        QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const OVERRIDE FINAL;
    bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;

private:
    /**
     * @brief  Offset the first and last frames of the handled reader by 'dt'.
     */
    void slipReader(double dt);

private:
    boost::weak_ptr<DSNode> _readerContext;
    double _dt;
    DopeSheetEditor *_model;
};



/**
 * @brief The DSRemoveKeysCommand class describes an undoable action that
 * remove the keyframes selected in the dope sheet.
 *
 * A undo gets the keyframes back.
 */
class DSRemoveKeysCommand : public QUndoCommand
{
public:
    DSRemoveKeysCommand(const std::vector<DopeSheetKey> &keys,
                        DopeSheetEditor *model,
                        QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

private:
    void addOrRemoveKeyframe(bool add);

private:
    std::vector<DopeSheetKey> _keys;
    DopeSheetEditor *_model;
};



/**
 * @brief The DSKeyInterpolationChange struct descrbes a interpolation
 * change of a keyframe.
 */
struct DSKeyInterpolationChange
{
    DSKeyInterpolationChange(KeyframeTypeEnum oldInterpType,
                             KeyframeTypeEnum newInterpType,
                             const DSKeyPtr & key)
        : _oldInterpType(oldInterpType),
          _newInterpType(newInterpType),
          _key(key)
    {}

    KeyframeTypeEnum _oldInterpType;
    KeyframeTypeEnum _newInterpType;
    DSKeyPtr _key;
};


/**
 * @brief The DSSetSelectedKeysInterpolationCommand class describes an undoable
 * action that change the interpolation of the keyframes selected in the dope
 * sheet.
 */
class DSSetSelectedKeysInterpolationCommand : public QUndoCommand
{
public:
    DSSetSelectedKeysInterpolationCommand(const std::list<DSKeyInterpolationChange> &changes,
                                          DopeSheetEditor *model,
                                          QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

private:
    void setInterpolation(bool undo);

private:
    std::list<DSKeyInterpolationChange> _changes;
    DopeSheetEditor *_model;
};


/**
 * @brief The DSPasteKeysCommand class describes an undoable action that paste
 * the keyframes contained in the dope sheet clipboard.
 *
 * A undo removes the pasted keyframes.
 */
class DSPasteKeysCommand : public QUndoCommand
{

    
public:
    DSPasteKeysCommand(const std::vector<DopeSheetKey> &keys,
                       DopeSheetEditor *model,
                       QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

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
    std::vector<DopeSheetKey> _keys;
    DopeSheetEditor *_model;
};

NATRON_NAMESPACE_EXIT;

#endif // DOPESHEETEDITORUNDOREDO_H
