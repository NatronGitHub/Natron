/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "EditExpressionDialog.h"

#include <cassert>
#include <climits>
#include <cfloat>
#include <stdexcept>

#include <boost/weak_ptr.hpp>


#include <QtCore/QString>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFormLayout>
#include <QFileDialog>
#include <QTextEdit>
#include <QStyle> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QtCore/QTimer>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QColorDialog>
#include <QGroupBox>
#include <QtGui/QVector4D>
#include <QStyleFactory>
#include <QCompleter>

#include "Global/GlobalDefines.h"

#include "Engine/Curve.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobSerialization.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Variant.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ComboBox.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveGui.h"
#include "Gui/CustomParamInteract.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobGuiGroup.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/Menu.h"
#include "Gui/Menu.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/ScriptTextEdit.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/SpinBox.h"
#include "Gui/TabWidget.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ViewerTab.h"

NATRON_NAMESPACE_ENTER

EditExpressionDialog::EditExpressionDialog(Gui* gui,
                                           int dimension,
                                           const KnobGuiPtr& knob,
                                           QWidget* parent)
    : EditScriptDialog(gui, parent)
    , _dimension(dimension)
    , _knob(knob)
{
}

int
EditExpressionDialog::getDimension() const
{
    return _dimension;
}

void
EditExpressionDialog::setTitle()
{
    KnobIPtr k = _knob->getKnob();
    QString title( tr("Set expression on ") );

    title.append( QString::fromUtf8( k->getName().c_str() ) );
    if ( (_dimension != -1) && (k->getDimension() > 1) ) {
        title.append( QLatin1Char('.') );
        title.append( QString::fromUtf8( k->getDimensionName(_dimension).c_str() ) );
    }
    setWindowTitle(title);
}

bool
EditExpressionDialog::hasRetVariable() const
{
    return _knob->getKnob()->isExpressionUsingRetVariable(_dimension == -1 ? 0 : _dimension);
}

QString
EditExpressionDialog::compileExpression(const QString& expr)
{
    std::string exprResult;

    try {
        _knob->getKnob()->validateExpression(expr.toStdString(), _dimension == -1 ? 0 : _dimension, isUseRetButtonChecked()
                                             , &exprResult);
    } catch (const std::exception& e) {
        QString err = QString( tr("ERROR") + QLatin1String(": %1") ).arg( QString::fromUtf8( e.what() ) );

        return err;
    }

    return QString::fromUtf8( exprResult.c_str() );
}

QString
EditExpressionDialog::getCustomHelp()
{
    //QString sep = QString::fromUtf8("<br/>");

    return getHelpPart1() + /*sep +*/
           getHelpThisNodeVariable() + /*sep +*/
           getHelpThisGroupVariable() + /*sep +*/
           getHelpThisParamVariable() + /*sep +*/
           getHelpDimensionVariable() + /*sep +*/
           getHelpPart2();
}

void
EditExpressionDialog::getImportedModules(QStringList& modules) const
{
    modules.push_back( QString::fromUtf8("math") );
}

void
EditExpressionDialog::getDeclaredVariables(std::list<std::pair<QString, QString> >& variables) const
{
    variables.push_back( std::make_pair( QString::fromUtf8("thisNode"), tr("the current node") ) );
    variables.push_back( std::make_pair( QString::fromUtf8("thisGroup"), tr("When thisNode belongs to a group, it references the parent group node, otherwise it will reference the current application instance") ) );
    variables.push_back( std::make_pair( QString::fromUtf8("thisParam"), tr("the current param being edited") ) );
    variables.push_back( std::make_pair( QString::fromUtf8("dimension"), tr("Defined only if the parameter is multi-dimensional, it references the dimension of the parameter being edited (0-based index)") ) );
    variables.push_back( std::make_pair( QString::fromUtf8("frame"), tr("the current time on the timeline or the time passed to the get function") ) );
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_EditExpressionDialog.cpp"
