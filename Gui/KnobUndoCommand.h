//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NATRON_GUI_KNOBUNDOCOMMAND_H_
#define NATRON_GUI_KNOBUNDOCOMMAND_H_

#include <map>
#include <vector>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QUndoCommand>
CLANG_DIAG_ON(deprecated)

#include "Engine/Variant.h"
#include "Engine/Curve.h"
class KnobGui;

//================================================================


class KnobUndoCommand : public QUndoCommand
{
public:
    KnobUndoCommand(KnobGui *knob, const std::vector<Variant> &oldValue, const std::vector<Variant> &newValue, QUndoCommand *parent = 0);

    virtual ~KnobUndoCommand() OVERRIDE {}

private:
    virtual void undo() OVERRIDE FINAL;

    virtual void redo() OVERRIDE FINAL;

    virtual int id() const OVERRIDE FINAL;
    
    virtual bool mergeWith(const QUndoCommand *command) OVERRIDE FINAL;
    
private:
    // TODO: PIMPL
    std::vector<Variant> _oldValue;
    std::vector<Variant> _newValue;
    KnobGui *_knob;
    std::vector<int> _valueChangedReturnCode;
    std::vector<KeyFrame> _newKeys;
    std::vector<KeyFrame>  _oldKeys;
    bool _merge;
};


#endif // NATRON_GUI_KNOBUNDOCOMMAND_H_
