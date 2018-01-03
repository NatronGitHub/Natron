/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#ifndef Gui_AddKnobDialog_h
#define Gui_AddKnobDialog_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct AddKnobDialogPrivate;
class AddKnobDialog
    : public QDialog
{
    Q_OBJECT

public:

    enum ParamDataTypeEnum
    {
        eParamDataTypeInteger, // 0
        eParamDataTypeInteger2D, // 1
        eParamDataTypeInteger3D, // 2
        eParamDataTypeFloatingPoint, //3
        eParamDataTypeFloatingPoint2D, // 4
        eParamDataTypeFloatingPoint3D, // 5
        eParamDataTypeColorRGB, // 6
        eParamDataTypeColorRGBA, // 7
        eParamDataTypeChoice, // 8
        eParamDataTypeCheckbox, // 9
        eParamDataTypeLabel, // 10
        eParamDataTypeTextInput, // 11
        eParamDataTypeInputFile, // 12
        eParamDataTypeDirectory, // 13
        eParamDataTypeGroup, // 14
        eParamDataTypePage, // 15
        eParamDataTypeButton, // 16
        eParamDataTypeSeparator, // 17
        eParamDataTypeCount // 18
    };

    AddKnobDialog(DockablePanel* panel,
                  const KnobIPtr& knob,
                  const std::string& selectedPageName,
                  const std::string& selectedGroupName,
                  QWidget* parent);

    virtual ~AddKnobDialog();

    KnobIPtr getKnob() const;

    void setVisibleType(bool visible);

    void setType(ParamDataTypeEnum type);

    static int dataTypeDim(ParamDataTypeEnum t);

public Q_SLOTS:

    void onPageCurrentIndexChanged(int index);

    void onTypeCurrentIndexChanged(int index);

    void onOkClicked();

private:

    static const char* dataTypeString(ParamDataTypeEnum t);

    static ParamDataTypeEnum getChoiceIndexFromKnobType(const KnobIPtr& knob);

    boost::scoped_ptr<AddKnobDialogPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_AddKnobDialog_h
