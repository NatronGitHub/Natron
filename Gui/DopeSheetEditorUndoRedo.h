#ifndef DOPESHEETEDITORUNDOREDO_H
#define DOPESHEETEDITORUNDOREDO_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <vector>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif
#include "Global/Enums.h"
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QUndoCommand> // in QtGui on Qt4, in QtWidgets on Qt5
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

class DopeSheet;
class DSSelectedKey;
class DSNode;

typedef boost::shared_ptr<DSSelectedKey> DSKeyPtr;
typedef std::list<DSKeyPtr> DSKeyPtrList;

/**
 * @brief The DSMoveKeysCommand class
 *
 *
 */
class DSMoveKeysCommand : public QUndoCommand
{
public:
    DSMoveKeysCommand(const DSKeyPtrList &keys,
                      double dt,
                      DopeSheet *model,
                      QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const;
    bool mergeWith(const QUndoCommand *other);

private:
    void moveSelectedKeyframes(double dt);

private:
    DSKeyPtrList _keys;
    double _dt;
    DopeSheet *_model;
};

/**
 * @brief The DSLeftTrimReaderCommand class
 *
 *
 */
class DSLeftTrimReaderCommand : public QUndoCommand
{
public:
    DSLeftTrimReaderCommand(DSNode *dsNodeReader,
                            double oldTime,
                            double newTime,
                            DopeSheet *model,
                            QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const;
    bool mergeWith(const QUndoCommand *other);

private:
    void trimLeft(double firstFrameTime);

private:
    DSNode *_dsNodeReader;
    double _oldTime;
    double _newTime;
    DopeSheet *_model;
};

/**
 * @brief The DSRightTrimReaderCommand class
 *
 *
 */
class DSRightTrimReaderCommand : public QUndoCommand
{
public:
    DSRightTrimReaderCommand(DSNode *dsNodeReader,
                             double oldTime,
                             double newTime,
                             DopeSheet *model,
                             QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const;
    bool mergeWith(const QUndoCommand *other);

private:
    void trimRight(double time);

private:
    DSNode *_dsNodeReader;
    double _oldTime;
    double _newTime;
    DopeSheet *_model;
};

class DSMoveReaderCommand : public QUndoCommand
{
public:
    DSMoveReaderCommand(DSNode *dsNodeReader,
                        double oldTime, double newTime,
                        DopeSheet *model,
                        QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const;
    bool mergeWith(const QUndoCommand *other);

private:
    void moveClip(double time);

private:
    DSNode *_dsNodeReader;
    double _oldTime;
    double _newTime;
    DopeSheet *_model;
};

/**
 * @brief The DSRemoveKeysCommand class
 *
 *
 */
class DSRemoveKeysCommand : public QUndoCommand
{
public:
    DSRemoveKeysCommand(const std::vector<DSSelectedKey> &keys,
                        DopeSheet *model,
                        QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

private:
    void addOrRemoveKeyframe(bool add);

private:
    std::vector<DSSelectedKey> _keys;
    DopeSheet *_model;
};

/**
 * @brief The DSMoveGroupCommand class
 *
 *
 */
class DSMoveGroupCommand : public QUndoCommand
{
public:
    DSMoveGroupCommand(DSNode *dsNodeGroup,
                       double dt,
                       DopeSheet *model,
                       QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

    int id() const;
    bool mergeWith(const QUndoCommand *other);

private:
    void moveGroupKeyframes(double dt);

private:
    DSNode *_dsNodeGroup;
    double _dt;
    DopeSheet *_model;
};

class DSChangeNodeLabelCommand : public QUndoCommand
{
public:
    DSChangeNodeLabelCommand(DSNode *dsNode,
                             const QString &oldLabel,
                             const QString &newLabel,
                             QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

private:
    void changeNodeLabel(const QString &label);

private:
    DSNode *_dsNode;
    QString _oldLabel;
    QString _newLabel;
};



/**
 * @brief The DSKeyInterpolationChange struct
 *
 *
 */
struct DSKeyInterpolationChange
{
    DSKeyInterpolationChange(Natron::KeyframeTypeEnum oldInterpType,
                             Natron::KeyframeTypeEnum newInterpType,
                             const DSKeyPtr & key)
        : _oldInterpType(oldInterpType),
          _newInterpType(newInterpType),
          _key(key)
    {
    }

    Natron::KeyframeTypeEnum _oldInterpType;
    Natron::KeyframeTypeEnum _newInterpType;
    DSKeyPtr _key;
};


/**
 * @brief The DSSetSelectedKeysInterpolationCommand class
 *
 *
 */
class DSSetSelectedKeysInterpolationCommand : public QUndoCommand
{
public:
    DSSetSelectedKeysInterpolationCommand(const std::list<DSKeyInterpolationChange> &changes,
                                          DopeSheet *model,
                                          QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

private:
    void setInterpolation(bool undo);

private:
    std::list<DSKeyInterpolationChange> _changes;
    DopeSheet *_model;
};


/**
 * @brief The DSAddKeysCommand class
 *
 *
 */
class DSPasteKeysCommand : public QUndoCommand
{
public:
    DSPasteKeysCommand(const std::vector<DSSelectedKey> &keys,
                       DopeSheet *model,
                       QUndoCommand *parent = 0);

    void undo() OVERRIDE FINAL;
    void redo() OVERRIDE FINAL;

private:
    void addOrRemoveKeyframe(bool add);

private:
    std::vector<DSSelectedKey> _keys;
    DopeSheet *_model;
};



#endif // DOPESHEETEDITORUNDOREDO_H
