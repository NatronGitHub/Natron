//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Gui/EditExpressionDialog.h"

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
#include <QTimer>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QColorDialog>
#include <QGroupBox>
#include <QtGui/QVector4D>
#include <QStyleFactory>
#include <QComboBox>
#include <QDialogButtonBox>
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

#include "Gui/AnimationButton.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveGui.h"
#include "Gui/CustomParamInteract.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Group_KnobGui.h"
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
#include "Gui/Utils.h"
#include "Gui/ViewerTab.h"

using namespace Natron;

EditExpressionDialog::EditExpressionDialog(int dimension,KnobGui* knob,QWidget* parent)
: EditScriptDialog(parent)
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
    boost::shared_ptr<KnobI> k = _knob->getKnob();
    
    QString title(tr("Set expression on "));
    title.append(k->getName().c_str());
    if (_dimension != -1 && k->getDimension() > 1) {
        title.append(".");
        title.append(k->getDimensionName(_dimension).c_str());
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
        _knob->getKnob()->validateExpression(expr.toStdString(),_dimension == -1 ? 0 : _dimension,isUseRetButtonChecked()
                                                  ,&exprResult);
    } catch(const std::exception& e) {
        QString err = QString(tr("ERROR") + ": %1").arg(e.what());
        return err;
    }
    return exprResult.c_str();
}


QString
EditExpressionDialog::getCustomHelp()
{
    return getHelpPart1() + "<br/>" +
    getHelpThisNodeVariable() + "<br/>" +
    getHelpThisGroupVariable() + "<br/>" +
    getHelpThisParamVariable() + "<br/>" +
    getHelpDimensionVariable() + "<br/>" +
    getHelpPart2();
}


void
EditExpressionDialog::getImportedModules(QStringList& modules) const
{
    modules.push_back("math");
}

void
EditExpressionDialog::getDeclaredVariables(std::list<std::pair<QString,QString> >& variables) const
{
    variables.push_back(std::make_pair("thisNode", tr("the current node")));
    variables.push_back(std::make_pair("thisGroup", tr("When thisNode belongs to a group, it references the parent group node, otherwise it will reference the current application instance")));
    variables.push_back(std::make_pair("thisParam", tr("the current param being edited")));
    variables.push_back(std::make_pair("dimension", tr("Defined only if the parameter is multi-dimensional, it references the dimension of the parameter being edited (0-based index")));
    variables.push_back(std::make_pair("frame", tr("the current time on the timeline")));
}
