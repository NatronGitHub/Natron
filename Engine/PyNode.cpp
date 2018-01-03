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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PyNode.h"

#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif


GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
#include "natronengine_python.h"
#include <shiboken.h> // produces many warnings
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
CLANG_DIAG_ON(mismatched-tags)
GCC_DIAG_ON(unused-parameter)


#include "Engine/Node.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/NodeGroup.h"
#include "Engine/PyAppInstance.h"
#include "Engine/PyRoto.h"
#include "Engine/PyTracker.h"
#include "Engine/Project.h"
#include "Engine/RotoPaintPrivate.h"
#include "Engine/TrackerNode.h"
#include "Engine/TrackerHelper.h"

#include "Engine/Hash64.h"

NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

ImageLayer::ImageLayer(const QString& layerName,
                       const QString& componentsPrettyName,
                       const QStringList& componentsName)
    : _layerName(layerName)
    , _componentsPrettyName(componentsPrettyName)
    , _componentsName(componentsName)
    , _comps()
{
    std::vector<std::string> channels( componentsName.size() );
    int i = 0;

    for (QStringList::const_iterator it = componentsName.begin(); it != componentsName.end(); ++it, ++i) {
        channels[i] = it->toStdString();
    }
    _comps.reset( new ImagePlaneDesc(layerName.toStdString(), "", componentsPrettyName.toStdString(), channels) );

}

ImageLayer::ImageLayer(const ImagePlaneDesc& comps)
    : _layerName( QString::fromUtf8( comps.getPlaneLabel().c_str() ) )
    , _componentsPrettyName( QString::fromUtf8( comps.getChannelsLabel().c_str() ) )
{
    const std::vector<std::string>& channels = comps.getChannels();

    for (std::size_t i = 0; i < channels.size(); ++i) {
        _componentsName.push_back( QString::fromUtf8( channels[i].c_str() ) );
    }
    _comps.reset( new ImagePlaneDesc(comps) );
}

int
ImageLayer::getHash(const ImageLayer& layer)
{
    Hash64 h;

    Hash64::appendQString(QString::fromUtf8( layer._comps->getPlaneLabel().c_str() ), &h );

    const std::vector<std::string>& comps = layer._comps->getChannels();
    for (std::size_t i = 0; i < comps.size(); ++i) {
        Hash64::appendQString(QString::fromUtf8( comps[i].c_str() ), &h );
    }

    return (int)h.value();
}

bool
ImageLayer::isColorPlane() const
{
    return _comps->isColorPlane();
}

int
ImageLayer::getNumComponents() const
{
    return _comps->getNumComponents();
}

const QString&
ImageLayer::getLayerName() const
{
    return _layerName;
}

const QStringList&
ImageLayer::getComponentsNames() const
{
    return _componentsName;
}

const QString&
ImageLayer::getComponentsPrettyName() const
{
    return _componentsPrettyName;
}

bool
ImageLayer::operator==(const ImageLayer& other) const
{
    return _comps == other._comps;
}

bool
ImageLayer::operator<(const ImageLayer& other) const
{
    return _comps < other._comps;
}

/*
 * These are default presets image components
 */
ImageLayer
ImageLayer::getNoneComponents()
{
    return ImageLayer( ImagePlaneDesc::getNoneComponents() );
}

ImageLayer
ImageLayer::getRGBAComponents()
{
    return ImageLayer( ImagePlaneDesc::getRGBAComponents() );
}

ImageLayer
ImageLayer::getRGBComponents()
{
    return ImageLayer( ImagePlaneDesc::getRGBComponents() );
}

ImageLayer
ImageLayer::getAlphaComponents()
{
    return ImageLayer( ImagePlaneDesc::getAlphaComponents() );
}

ImageLayer
ImageLayer::getBackwardMotionComponents()
{
    return ImageLayer( ImagePlaneDesc::getBackwardMotionComponents() );
}

ImageLayer
ImageLayer::getForwardMotionComponents()
{
    return ImageLayer( ImagePlaneDesc::getForwardMotionComponents() );
}

ImageLayer
ImageLayer::getDisparityLeftComponents()
{
    return ImageLayer( ImagePlaneDesc::getDisparityLeftComponents() );
}

ImageLayer
ImageLayer::getDisparityRightComponents()
{
    return ImageLayer( ImagePlaneDesc::getDisparityRightComponents() );
}

UserParamHolder::UserParamHolder()
    : _holder()
{
}

UserParamHolder::UserParamHolder(const KnobHolderPtr& holder)
    : _holder(holder)
{
}

void
UserParamHolder::setHolder(const KnobHolderPtr& holder)
{
    assert(!_holder.lock());
    _holder = holder;
}

Effect::Effect(const NodePtr& node)
    : Group()
    , UserParamHolder(node ? node->getEffectInstance() : KnobHolderPtr())
    , _node(node)
{
    if (node) {
        NodeGroupPtr grp;
        if ( node->getEffectInstance() ) {
            grp = toNodeGroup( node->getEffectInstance()->shared_from_this() );
            init( boost::dynamic_pointer_cast<NodeCollection>(grp) );
        }
    }
}

Effect::~Effect()
{
}

NodePtr
Effect::getInternalNode() const
{
    return _node.lock();
}

EffectInstancePtr
Effect::getCurrentEffectInstance() const
{
    NodePtr internalNode = getInternalNode();
    if (!internalNode) {
        return EffectInstancePtr();
    }

    // Get the last python API caller
    EffectInstancePtr effectTLS = appPTR->getLastPythonAPICaller_TLS();
    if (!effectTLS) {
        return internalNode->getEffectInstance();
    }

    // If the last caller is the same Node, return it
    if (effectTLS->getNode() == internalNode) {
        return effectTLS;
    } else {
        // If the python API caller is a render clone, make sure we return a clone for the same render.
        // If it's not a render clone, just return the main instance
        if (!effectTLS->isRenderClone()) {
            return internalNode->getEffectInstance();
        }
        FrameViewRenderKey key = {effectTLS->getCurrentRenderTime(), effectTLS->getCurrentRenderView(), effectTLS->getCurrentRender()};
        EffectInstancePtr ret = toEffectInstance(internalNode->getEffectInstance()->createRenderClone(key));
        return ret;
    }
}

bool
Effect::isReaderNode()
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return false;
    }

    return n->getEffectInstance()->isReader();
}

Group*
Effect::getContainerGroup() const
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return 0;
    }
    NodeCollectionPtr container = n->getGroup();
    if (!container) {
        return 0;
    }
    NodeGroupPtr isGroup = toNodeGroup(container);
    if (isGroup) {
        return App::createEffectFromNodeWrapper(isGroup->getNode());
    } else {
        assert(container == n->getApp()->getProject());
        return App::createAppFromAppInstance(n->getApp());
    }
}

bool
Effect::isWriterNode()
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return false;
    }

    return n->getEffectInstance()->isWriter();
}

void
Effect::destroy()
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return;
    }

    n->destroyNode();
}

int
Effect::getMaxInputCount() const
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return 0;
    }

    return n->getNInputs();
}

bool
Effect::canConnectInput(int inputNumber,
                        const Effect* node) const
{
    if (!node) {
        PythonSetNullError();
        return false;
    }

    if ( !node->getInternalNode() ) {
        PythonSetNullError();
        return false;
    }

    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return 0;
    }
    Node::CanConnectInputReturnValue ret = n->canConnectInput(node->getInternalNode(), inputNumber);

    return ret == Node::eCanConnectInput_ok ||
           ret == Node::eCanConnectInput_differentFPS ||
           ret == Node::eCanConnectInput_differentPars;
}

bool
Effect::connectInput(int inputNumber,
                     const Effect* input)
{

    if ( canConnectInput(inputNumber, input) ) {
        return getInternalNode()->connectInput(input->getInternalNode(), inputNumber);
    } else {
        return false;
    }
}

void
Effect::disconnectInput(int inputNumber)
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return;
    }
    n->disconnectInput(inputNumber);
}

Effect*
Effect::getInput(int inputNumber) const
{


    EffectInstancePtr effect = getCurrentEffectInstance();
    if (!effect) {
        PythonSetNullError();
        return 0;
    }
    EffectInstancePtr inputEffect = effect->getInputRenderEffectAtAnyTimeView(inputNumber);
    if (!inputEffect) {
        return 0;
    }
    NodePtr inputNode = inputEffect->getNode();
    if (inputNode) {
        return App::createEffectFromNodeWrapper(inputNode);
    }

    return 0;
}

Effect*
Effect::getInput(const QString& inputLabel) const
{
    EffectInstancePtr effect = getCurrentEffectInstance();
    if (!effect) {
        PythonSetNullError();
        return 0;
    }
    int maxInputs = effect->getNInputs();
    for (int i = 0; i < maxInputs; ++i) {
        if (QString::fromUtf8(effect->getNode()->getInputLabel(i).c_str()) == inputLabel) {

            EffectInstancePtr inputEffect = effect->getInputRenderEffectAtAnyTimeView(i);
            if (!inputEffect) {
                return 0;
            }
            NodePtr inputNode = inputEffect->getNode();
            if (inputNode) {
                return App::createEffectFromNodeWrapper(inputNode);
            }
        }
    }
    return 0;
}

QString
Effect::getScriptName() const
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return QString();
    }
    return QString::fromUtf8( n->getScriptName_mt_safe().c_str() );
}

bool
Effect::setScriptName(const QString& scriptName)
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return false;
    }
    try {
        n->setScriptName( scriptName.toStdString() );
    } catch (...) {
        return false;
    }

    return true;
}

QString
Effect::getLabel() const
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return QString();
    }
    return QString::fromUtf8( n->getLabel_mt_safe().c_str() );
}

void
Effect::setLabel(const QString& name)
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return;
    }
    return n->setLabel( name.toStdString() );
}

QString
Effect::getInputLabel(int inputNumber)
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return QString();
    }
    try {
        return QString::fromUtf8( n->getInputLabel(inputNumber).c_str() );
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_IndexError, e.what());
        getInternalNode()->getApp()->appendToScriptEditor( e.what() );
    }

    return QString();
}

QString
Effect::getPluginID() const
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return QString();
    }
    return QString::fromUtf8( n->getPluginID().c_str() );
}

Param*
Effect::createParamWrapperForKnob(const KnobIPtr& knob)
{

    if (!knob) {
        PythonSetNullError();
        return 0;
    }

    // Try to re-use an existing Effect object that was created for this node.
    KnobHolderPtr holder = knob->getHolder();
    if (holder) {


        // Get a pointer to the node if this param is held by a node
        NodePtr node;
        KnobTableItemPtr isItem = toKnobTableItem(holder);
        EffectInstancePtr isEffect = toEffectInstance(holder);
        if (isItem) {
            KnobItemsTablePtr model = isItem->getModel();
            if (!model) {
                return 0;
            }
            node = model->getNode();
        } else if (isEffect) {
            node = isEffect->getNode();
        }

        // Ge the app pointer, there may be none if the knob is an application setting
        AppInstancePtr app = holder->getApp();

        std::stringstream ss;
        // We assign the existing attribute to a temporary attribute that we delete afterwards
        ss << kPythonTmpCheckerVariable << " = ";

        if (!app) {
            // This is an application setting
            ss << NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.settings";
        } else {

            ss << holder->getApp()->getAppIDString();
            if (node) {
                // This is a knob of a node
                ss << "." << node->getFullyQualifiedName();
            }
            if (isItem) {
                // This is a knob of a table item (such as a Bezier/ Track etc...)
                ss << "." << isItem->getModel()->getPythonPrefix() << "." << isItem->getFullyQualifiedName();
            }
        }
        ss << "." << knob->getName();
        std::string script = ss.str();

        // Interpret the script and check if kPythonTmpCheckerVariable was set.
        // We then retrieve its C++ equivalent and return it
        bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, 0, 0);

        // Clear errors if our call to interpretPythonScript failed, we don't want the
        // calling function to fail aswell.
        NATRON_PYTHON_NAMESPACE::clearPythonStdErr();
        if (ok) {

            // Check if the kPythonTmpCheckerVariable was correctly assigned
            PyObject* pyParam = 0;
            PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
            if ( PyObject_HasAttrString(mainModule, kPythonTmpCheckerVariable) ) {
                pyParam = PyObject_GetAttrString(mainModule, kPythonTmpCheckerVariable);
                if (pyParam == Py_None) {
                    pyParam = 0;
                }
            }

            // The kPythonTmpCheckerVariable was set, check that it is valid and convert it to our C++ type
            Param* cppParam = 0;
            if (pyParam && Shiboken::Object::isValid(pyParam)) {
                cppParam = (Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)pyParam);
            }

            // Ensure we remove the kPythonTmpCheckerVariable attribute
            NATRON_PYTHON_NAMESPACE::interpretPythonScript("del " kPythonTmpCheckerVariable, 0, 0);
            NATRON_PYTHON_NAMESPACE::clearPythonStdErr();
            if (cppParam) {
                return cppParam;
            }
        }

    }

    // We did not find an already existing attribute, create one

    int dims = knob->getNDimensions();
    KnobIntPtr isInt = toKnobInt(knob);
    KnobDoublePtr isDouble = toKnobDouble(knob);
    KnobBoolPtr isBool = toKnobBool(knob);
    KnobChoicePtr isChoice = toKnobChoice(knob);
    KnobColorPtr isColor = toKnobColor(knob);
    KnobStringPtr isString = toKnobString(knob);
    KnobFilePtr isFile = toKnobFile(knob);
    KnobPathPtr isPath = toKnobPath(knob);
    KnobButtonPtr isButton = toKnobButton(knob);
    KnobGroupPtr isGroup = toKnobGroup(knob);
    KnobPagePtr isPage = toKnobPage(knob);
    KnobParametricPtr isParametric = toKnobParametric(knob);
    KnobSeparatorPtr isSep = toKnobSeparator(knob);

    if (isInt) {
        switch (dims) {
        case 1:

            return new IntParam(isInt);
        case 2:

            return new Int2DParam(isInt);
        case 3:

            return new Int3DParam(isInt);
        default:
            return new IntParam(isInt);
            break;
        }
    } else if (isDouble) {
        switch (dims) {
        case 1:

            return new DoubleParam(isDouble);
        case 2:

            return new Double2DParam(isDouble);
        case 3:

            return new Double3DParam(isDouble);
        default:
            return new DoubleParam(isDouble);
            break;
        }
    } else if (isBool) {
        return new BooleanParam(isBool);
    } else if (isChoice) {
        return new ChoiceParam(isChoice);
    } else if (isColor) {
        return new ColorParam(isColor);
    } else if (isString) {
        return new StringParam(isString);
    } else if (isFile) {
        return new FileParam(isFile);
    } else if (isPath) {
        return new PathParam(isPath);
    } else if (isGroup) {
        return new GroupParam(isGroup);
    } else if (isPage) {
        return new PageParam(isPage);
    } else if (isParametric) {
        return new ParametricParam(isParametric);
    } else if (isButton) {
        return new ButtonParam(isButton);
    } else if (isSep) {
        return new SeparatorParam(isSep);
    }

    return NULL;
} // Effect::createParamWrapperForKnob

std::list<Param*>
Effect::getParams() const
{
    std::list<Param*> ret;
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return ret;
    }
    const KnobsVec& knobs = n->getKnobs();

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        Param* p = createParamWrapperForKnob(*it);
        if (p) {
            ret.push_back(p);
        }
    }

    return ret;
}

Param*
Effect::getParam(const QString& name) const
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return 0;
    }
    QString fallbackSearchName;
    if (n->getApp()->isCreatingNode()) {
        // Before Natron 2.2.3, all dynamic choice parameters for multiplane had a string parameter.
        // The string parameter had the same name as the choice parameter plus "Choice" appended.
        // If we found such a parameter, retrieve the string from it.
        QString str = QString::fromUtf8("Choice");
        if (name.endsWith(str)) {
            fallbackSearchName = name.mid(0, name.size() - str.size());
        }
    }

    KnobIPtr knob = n->getKnobByName( name.toStdString() );

    if (knob) {
        return createParamWrapperForKnob(knob);
    } else {
        if (!fallbackSearchName.isEmpty()) {
            return getParam(fallbackSearchName);
        }
        return NULL;
    }
}

double
Effect::getCurrentTime() const
{
   EffectInstancePtr effect = getCurrentEffectInstance();

    if (!effect) {
        PythonSetNullError();
        return 0.;
    }
    return effect->getCurrentRenderTime();
}

void
Effect::setPosition(double x,
                    double y)
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return ;
    }
    n->setPosition(x, y);
}

void
Effect::getPosition(double* x,
                    double* y) const
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return;
    }
    n->getPosition(x, y);
}

void
Effect::setSize(double w,
                double h)
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return ;
    }
    n->setSize(w, h);
}

void
Effect::getSize(double* w,
                double* h) const
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return;
    }
    n->getSize(w, h);
}

void
Effect::getColor(double* r,
                 double *g,
                 double* b) const
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return;
    }
    bool hasColor = n->getColor(r, g, b);
    Q_UNUSED(hasColor);
}

void
Effect::setColor(double r,
                 double g,
                 double b)
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return ;
    }
    n->setColor(r, g, b);
}

bool
Effect::isNodeSelected() const
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return false;
    }
    return n->isUserSelected();
}


void
Effect::beginChanges()
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return;
    }
    n->getEffectInstance()->beginChanges();
    n->beginInputEdition();
}

void
Effect::endChanges()
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return;
    }
    n->getEffectInstance()->endChanges();
    n->endInputEdition(true);
}

void
Effect::beginParametersUndoCommand(const QString& name)
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return;
    }
    EffectInstancePtr effect = n->getEffectInstance();
    if (!effect) {
        PythonSetNullError();
        return;
    }
    effect->beginMultipleEdits(name.toStdString());
}

void
Effect::endParametersUndoCommand()
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return;
    }
    EffectInstancePtr effect = n->getEffectInstance();
    if (!effect) {
        PythonSetNullError();
        return;
    }
    effect->endMultipleEdits();
}

IntParam*
UserParamHolder::createIntParam(const QString& name,
                                const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobIntPtr knob = holder->createIntKnob(name.toStdString(), label.toStdString(), 1, KnobI::eKnobDeclarationTypeUser);

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }

        IntParam* ret = dynamic_cast<IntParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

Int2DParam*
UserParamHolder::createInt2DParam(const QString& name,
                                  const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobIntPtr knob = holder->createIntKnob(name.toStdString(), label.toStdString(), 2, KnobI::eKnobDeclarationTypeUser);

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }

        Int2DParam* ret = dynamic_cast<Int2DParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

Int3DParam*
UserParamHolder::createInt3DParam(const QString& name,
                                  const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobIntPtr knob = holder->createIntKnob(name.toStdString(), label.toStdString(), 3, KnobI::eKnobDeclarationTypeUser);

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }

        Int3DParam* ret = dynamic_cast<Int3DParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

DoubleParam*
UserParamHolder::createDoubleParam(const QString& name,
                                   const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobDoublePtr knob = holder->createDoubleKnob(name.toStdString(), label.toStdString(), 1, KnobI::eKnobDeclarationTypeUser);

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }

        DoubleParam* ret = dynamic_cast<DoubleParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

Double2DParam*
UserParamHolder::createDouble2DParam(const QString& name,
                                     const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobDoublePtr knob = holder->createDoubleKnob(name.toStdString(), label.toStdString(), 2, KnobI::eKnobDeclarationTypeUser);

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }

        Double2DParam* ret = dynamic_cast<Double2DParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

Double3DParam*
UserParamHolder::createDouble3DParam(const QString& name,
                                     const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobDoublePtr knob = holder->createDoubleKnob(name.toStdString(), label.toStdString(), 3, KnobI::eKnobDeclarationTypeUser);

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }

        Double3DParam* ret = dynamic_cast<Double3DParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

BooleanParam*
UserParamHolder::createBooleanParam(const QString& name,
                                    const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobBoolPtr knob = holder->createBoolKnob( name.toStdString(), label.toStdString(), KnobI::eKnobDeclarationTypeUser );

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }

        BooleanParam* ret = dynamic_cast<BooleanParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

ChoiceParam*
UserParamHolder::createChoiceParam(const QString& name,
                                   const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobChoicePtr knob = holder->createChoiceKnob( name.toStdString(), label.toStdString() , KnobI::eKnobDeclarationTypeUser);

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }

        ChoiceParam* ret = dynamic_cast<ChoiceParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

ColorParam*
UserParamHolder::createColorParam(const QString& name,
                                  const QString& label,
                                  bool useAlpha)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobColorPtr knob = holder->createColorKnob(name.toStdString(), label.toStdString(), useAlpha ? 4 : 3, KnobI::eKnobDeclarationTypeUser);

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }
        ColorParam* ret = dynamic_cast<ColorParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

StringParam*
UserParamHolder::createStringParam(const QString& name,
                                   const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobStringPtr knob = holder->createStringKnob( name.toStdString(), label.toStdString(), KnobI::eKnobDeclarationTypeUser );

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }
        StringParam* ret = dynamic_cast<StringParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

FileParam*
UserParamHolder::createFileParam(const QString& name,
                                 const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobFilePtr knob = holder->createFileKnob( name.toStdString(), label.toStdString(), KnobI::eKnobDeclarationTypeUser );

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }
        knob->setDialogType(KnobFile::eKnobFileDialogTypeOpenFile);
        FileParam* ret = dynamic_cast<FileParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

FileParam*
UserParamHolder::createOutputFileParam(const QString& name,
                                       const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobFilePtr knob = holder->createFileKnob( name.toStdString(), label.toStdString() , KnobI::eKnobDeclarationTypeUser);

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }
        knob->setDialogType(KnobFile::eKnobFileDialogTypeSaveFile);
        FileParam* ret = dynamic_cast<FileParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

PathParam*
UserParamHolder::createPathParam(const QString& name,
                                 const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobPathPtr knob = holder->createPathKnob( name.toStdString(), label.toStdString(), KnobI::eKnobDeclarationTypeUser );

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }

        PathParam* ret = dynamic_cast<PathParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

ButtonParam*
UserParamHolder::createButtonParam(const QString& name,
                                   const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobButtonPtr knob = holder->createButtonKnob( name.toStdString(), label.toStdString(), KnobI::eKnobDeclarationTypeUser );

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }
        ButtonParam* ret = dynamic_cast<ButtonParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

SeparatorParam*
UserParamHolder::createSeparatorParam(const QString& name,
                                      const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobSeparatorPtr knob = holder->createSeparatorKnob( name.toStdString(), label.toStdString(), KnobI::eKnobDeclarationTypeUser );

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }
        SeparatorParam* ret = dynamic_cast<SeparatorParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

GroupParam*
UserParamHolder::createGroupParam(const QString& name,
                                  const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobGroupPtr knob = holder->createGroupKnob( name.toStdString(), label.toStdString(), KnobI::eKnobDeclarationTypeUser );

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }
        GroupParam* ret = dynamic_cast<GroupParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

PageParam*
UserParamHolder::createPageParam(const QString& name,
                                 const QString& label)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobPagePtr knob = holder->createPageKnob( name.toStdString(), label.toStdString(), KnobI::eKnobDeclarationTypeUser );
    if (knob) {
        PageParam* ret = dynamic_cast<PageParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

ParametricParam*
UserParamHolder::createParametricParam(const QString& name,
                                       const QString& label,
                                       int nbCurves)
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return NULL;
    }
    KnobParametricPtr knob = holder->createParametricKnob(name.toStdString(), label.toStdString(), nbCurves, KnobI::eKnobDeclarationTypeUser);

    if (knob) {
        KnobPagePtr userPage = holder->getOrCreateUserPageKnob();
        if (userPage) {
            userPage->addKnob(knob);
        }
        ParametricParam* ret = dynamic_cast<ParametricParam*>(Effect::createParamWrapperForKnob(knob));
        assert(ret);
        return ret;
    } else {
        return 0;
    }
}

bool
UserParamHolder::removeParam(Param* param)
{
    if (!param) {
        PythonSetNullError();
        return false;
    }
    if ( !param->getInternalKnob() ) {
        PythonSetNullError();
        return false;
    }
    if ( param->getInternalKnob()->getKnobDeclarationType() != KnobI::eKnobDeclarationTypeUser ) {
        PythonSetNonUserKnobError();
        return false;
    }

    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return false;
    }
    holder->deleteKnob(param->getInternalKnob(), true);

    return true;
}

PageParam*
Effect::getUserPageParam() const
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return 0;
    }
    KnobPagePtr page = n->getEffectInstance()->getOrCreateUserPageKnob();
    if (!page) {
        return 0;
    }

    return new PageParam(page);
}

void
UserParamHolder::refreshUserParamsGUI()
{
    KnobHolderPtr holder = _holder.lock();
    if (!holder) {
        PythonSetNullError();
        return;
    }
    holder->recreateUserKnobs(false);
}

ItemsTable*
Effect::createItemsTableWrapper(const KnobItemsTablePtr& table)
{
    if (!table) {
        return 0;
    }

    NodePtr node = table->getNode();
    if (!node) {
        PythonSetNullError();
        return 0;
    }

    // First, try to re-use an existing ItemsTable object that was created for this node.
    // If not found, create one.
    std::stringstream ss;
    ss << kPythonTmpCheckerVariable << " = ";
    ss << node->getApp()->getAppIDString() << "." << node->getFullyQualifiedName() << "." << table->getPythonPrefix();
    std::string script = ss.str();
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, 0, 0);
    // Clear errors if our call to interpretPythonScript failed, we don't want the
    // calling function to fail aswell.
    NATRON_PYTHON_NAMESPACE::clearPythonStdErr();
    if (ok) {
        PyObject* pyTable = 0;
        PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
        if ( PyObject_HasAttrString(mainModule, kPythonTmpCheckerVariable) ) {
            pyTable = PyObject_GetAttrString(mainModule, kPythonTmpCheckerVariable);
            if (pyTable == Py_None) {
                pyTable = 0;
            }
        }
        ItemsTable* cppTable = 0;
        if (pyTable && Shiboken::Object::isValid(pyTable)) {
            cppTable = (ItemsTable*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMSTABLE_IDX], (SbkObject*)pyTable);
        }
        NATRON_PYTHON_NAMESPACE::interpretPythonScript("del " kPythonTmpCheckerVariable, 0, 0);
        NATRON_PYTHON_NAMESPACE::clearPythonStdErr();
        if (cppTable) {
            return cppTable;
        }
    }

    // create one

    RotoPaintKnobItemsTable* isRotoPaintTable = dynamic_cast<RotoPaintKnobItemsTable*>(table.get());
    if (isRotoPaintTable) {
        return new Roto(table);
    }
    TrackerNodePtr isTrackerNode = toTrackerNode(node->getEffectInstance());
    if (isTrackerNode) {
        return new Tracker(table, isTrackerNode->getTracker());
    }
    return new ItemsTable(table);


} // createItemsTableWrapper

ItemsTable*
Effect::getItemsTable(const QString& identifier) const
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return 0;
    }
    KnobItemsTablePtr table = n->getEffectInstance()->getItemsTable(identifier.toStdString());
    if (!table) {
        return 0;
    }
    return Effect::createItemsTableWrapper(table);
}

std::list<ItemsTable*>
Effect::getAllItemsTable() const
{
    NodePtr n = getInternalNode();
    std::list<ItemsTable*> ret;
    if (!n) {
        PythonSetNullError();
        return ret;
    }
    std::list<KnobItemsTablePtr> tables = n->getEffectInstance()->getAllItemsTables();
    if (tables.empty()) {
        return ret;
    }
    for (std::list<KnobItemsTablePtr>::const_iterator it = tables.begin(); it != tables.end(); ++it) {
        ItemsTable* table = Effect::createItemsTableWrapper(*it);
        ret.push_back(table);
    }
    return ret;
}


RectD
Effect::getRegionOfDefinition(double time,
                              int view) const
{
    RectD rod;

    EffectInstancePtr effect = getCurrentEffectInstance();
    if (!effect) {
        PythonSetNullError();
        return rod;
    }
    RenderScale s(1.);

    GetRegionOfDefinitionResultsPtr results;
    ActionRetCodeEnum stat = effect->getRegionOfDefinition_public(TimeValue(time), s, ViewIdx(view),  &results);
    if (isFailureRetCode(stat)) {
        return rod;
    }
    rod = results->getRoD();

    return rod;
}

RectD
Effect::getRegionOfDefinition(double time, const QString& view) const
{
    RectD rod;
    EffectInstancePtr effect = getCurrentEffectInstance();
    if (!effect) {
        PythonSetNullError();
        return rod;
    }
    const std::vector<std::string>& projectViews = effect->getApp()->getProject()->getProjectViewNames();
    bool foundView = false;
    ViewIdx foundViewIdx;
    std::string stdViewName = view.toStdString();
    foundView = Project::getViewIndex(projectViews, stdViewName, &foundViewIdx);
  
    if (!foundView) {
        PythonSetInvalidViewName(view);
        return rod;
    }
    return getRegionOfDefinition(time, foundViewIdx);

}

void
Effect::setSubGraphEditable(bool editable)
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return;
    }

    NodeGroupPtr isGroup = n->isEffectNodeGroup();
    if (isGroup) {
        isGroup->setSubGraphEditable(editable);
    }
}

bool
Effect::addUserPlane(const QString& planeName,
                     const QStringList& channels)
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return false;
    }
    if ( planeName.isEmpty() ||
         ( channels.size() < 1) ||
         ( channels.size() > 4) ) {
        PyErr_SetString(PyExc_ValueError, tr("Invalid arguments").toStdString().c_str());
        return false;
    }
    std::string compsGlobal;
    std::vector<std::string> chans( channels.size() );
    int i = 0;
    for (QStringList::const_iterator it = channels.begin(); it != channels.end(); ++it, ++i) {
        std::string c = it->toStdString();
        compsGlobal.append(c);
        chans[i] = c;
    }
    ImagePlaneDesc comp(planeName.toStdString(), "", compsGlobal, chans);


    return n->getEffectInstance()->addUserComponents(comp);
}

std::list<ImageLayer>
Effect::getAvailableLayers(int inputNb) const
{
    std::list<ImageLayer> ret;

    EffectInstancePtr effect = getCurrentEffectInstance();
    if (!effect) {
        PythonSetNullError();
        return ret;
    }
    TimeValue time = effect->getCurrentRenderTime();

    std::list<ImagePlaneDesc> availComps;
    effect->getAvailableLayers(time, ViewIdx(0), inputNb,  &availComps);
    for (std::list<ImagePlaneDesc>::iterator it = availComps.begin(); it != availComps.end(); ++it) {
        ret.push_back(ImageLayer(*it));
    }


    return ret;
}

double
Effect::getFrameRate() const
{
    EffectInstancePtr effect = getCurrentEffectInstance();

    if (!effect) {
        PythonSetNullError();
        return 24.;
    }

    return effect->getFrameRate();
}

double
Effect::getPixelAspectRatio() const
{
    EffectInstancePtr effect = getCurrentEffectInstance();

    if (!effect) {
        PythonSetNullError();
        return 1.;
    }

    return effect->getAspectRatio(-1);
}

ImageBitDepthEnum
Effect::getBitDepth() const
{
    EffectInstancePtr effect = getCurrentEffectInstance();

    if (!effect) {
        PythonSetNullError();
        return eImageBitDepthFloat;
    }

    return effect->getBitDepth(-1);
}

ImagePremultiplicationEnum
Effect::getPremult() const
{
    EffectInstancePtr effect = getCurrentEffectInstance();

    if (!effect) {
        PythonSetNullError();
        return eImagePremultiplicationPremultiplied;
    }

    return effect->getPremult();
}

void
Effect::setPagesOrder(const QStringList& pages)
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return ;
    }


    std::list<std::string> order;
    for (QStringList::const_iterator it = pages.begin(); it != pages.end(); ++it) {
        order.push_back( it->toStdString() );
    }
    n->setPagesOrder(order);
}


void
Effect::insertParamInViewerUI(Param* param, int index)
{
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return ;
    }
    if (!param) {
        PythonSetNullError();
        return;
    }
    KnobIPtr knob = param->getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    n->getEffectInstance()->insertKnobToViewerUI(knob, index);
}

void
Effect::removeParamFromViewerUI(Param* param)
{
    NodePtr node = getInternalNode();
    if (!node || !param) {
        PythonSetNullError();
        return;
    }
    KnobIPtr knob = param->getInternalKnob();
    if (!knob) {
        PythonSetNullError();
        return;
    }
    node->getEffectInstance()->removeKnobViewerUI(knob);
}

void
Effect::clearViewerUIParameters()
{
    NodePtr node = getInternalNode();
    if (!node) {
        PythonSetNullError();
        return;
    }
    node->getEffectInstance()->setViewerUIKnobs(KnobsVec());
}

bool
Effect::registerOverlay(PyOverlayInteract* interact, const std::map<QString, QString>& params)
{
    if (!interact) {
        PythonSetNullError();
        return false;
    }
    interact->initInternalInteract();
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return false;
    }
    std::map<std::string, std::string> knobsMap;
    for (std::map<QString, QString>::const_iterator it = params.begin(); it != params.end(); ++it) {
        knobsMap.insert(std::make_pair(it->first.toStdString(), it->second.toStdString()));
    }
    try {
        n->getEffectInstance()->registerOverlay(eOverlayViewportTypeViewer, interact->getInternalInteract(), knobsMap);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return false;
    }
    return true;
}

void
Effect::removeOverlay(PyOverlayInteract* interact)
{
    if (!interact) {
        PythonSetNullError();
        return;
    }
    NodePtr n = getInternalNode();

    if (!n) {
        PythonSetNullError();
        return;
    }
    return n->getEffectInstance()->removeOverlay(eOverlayViewportTypeViewer, interact->getInternalInteract());
}


NATRON_PYTHON_NAMESPACE_EXIT


NATRON_NAMESPACE_EXIT
