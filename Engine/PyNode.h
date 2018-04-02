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

#ifndef NODEWRAPPER_H
#define NODEWRAPPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

/**
 * @brief Simple wrap for the EffectInstance and Node class that is the API we want to expose to the Python
 * Engine module.
 **/

#include <list>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Engine/ImagePlaneDesc.h"
#include "Engine/Knob.h" // KnobI
#include "Engine/PyNodeGroup.h" // Group
#include "Engine/RectD.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

class ImageLayer
{
    QString _layerName;
    QString _componentsPrettyName;
    QStringList _componentsName;
    ImagePlaneDescPtr _comps;

public:

    ImageLayer(const QString& layerName,
               const QString& componentsPrettyName,
               const QStringList& componentsName);

    ImageLayer(const ImagePlaneDesc& comps);

    const ImagePlaneDesc& getInternalComps() const
    {
        return *_comps;
    }

    ~ImageLayer() {}

    static int getHash(const ImageLayer& layer);

    bool isColorPlane() const;

    int getNumComponents() const;

    const QString& getLayerName() const;
    const QStringList& getComponentsNames() const;
    const QString& getComponentsPrettyName() const;

    bool operator==(const ImageLayer& other) const;

    bool operator!=(const ImageLayer& other) const
    {
        return !(*this == other);
    }

    //For std::map
    bool operator<(const ImageLayer& other) const;

    /*
     * These are default presets image components
     */
    static ImageLayer getNoneComponents();
    static ImageLayer getRGBAComponents();
    static ImageLayer getRGBComponents();
    static ImageLayer getAlphaComponents();
    static ImageLayer getBackwardMotionComponents();
    static ImageLayer getForwardMotionComponents();
    static ImageLayer getDisparityLeftComponents();
    static ImageLayer getDisparityRightComponents();
};

class UserParamHolder
{
    KnobHolder* _holder;

public:

    UserParamHolder();

    UserParamHolder(KnobHolder* holder);

    virtual ~UserParamHolder() {}

    void setHolder(KnobHolder* holder);

    /////////////Functions to create custom parameters//////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    //////////// A parameter may have some properties set after creation, though most of them are not dynamic:
    //////////// they need to be set before calling refreshUserParamsGUI() which will create the GUI for these parameters.
    //////////// Here's a list of the properties and whether they must be set before refreshUserParamsGUI() or can be set
    //////////// dynamically after refreshUserParamsGUI() was called. A non-dynamic property can no longer be changed once
    //////////// refreshUserParamsGUI() has been called.
    //////////// If a Setter function contains a (*) that means it can only be called for user parameters,
    //////////// it has no effect on already declared non-user parameters.
    ////////////
    //////////// Name:              Type:           Dynamic:            Setter:                 Getter:               Default:
    ////////////
    //////////// name               string          no                  None                    getScriptName         ""
    //////////// label              string          no                  None                    getLabel              ""
    //////////// help               string          yes                 setHelp(*)              getHelp               ""
    //////////// addNewLine         bool            no                  setAddNewLine(*)        getAddNewLine         True
    //////////// persistent         bool            yes                 setPersistent(*)        getIsPersistent       True
    //////////// evaluatesOnChange  bool            yes                 setEvaluateOnChange(*)  getEvaluateOnChange   True
    //////////// animates           bool            no                  setAnimationEnabled(*)  getIsAnimationEnabled (1)
    //////////// visible            bool            yes                 setVisible              getIsVisible          True
    //////////// enabled            bool            yes                 setEnabled              getIsEnabled          True
    ////////////
    //////////// Properties on IntParam, Int2DParam, Int3DParam, DoubleParam, Double2DParam, Double3DParam, ColorParam only:
    ////////////
    //////////// min                int/double      yes                 setMinimum(*)            getMinimum            INT_MIN
    //////////// max                int/double      yes                 setMaximum(*)            getMaximum            INT_MAX
    //////////// displayMin         int/double      yes                 setDisplayMinimum(*)     getDisplayMinimum     INT_MIN
    //////////// displayMax         int/double      yes                 setDisplayMaximum(*)     getDisplayMaximum     INT_MAX
    ////////////
    //////////// Properties on ChoiceParam only:
    ////////////
    //////////// options            list<string>    yes                 setOptions/addOption(*)  getOption             empty list
    ////////////
    //////////// Properties on FileParam, OutputFileParam only:
    ////////////
    //////////// sequenceDialog     bool            yes                 setSequenceEnabled(*)    None                  False
    ////////////
    //////////// Properties on StringParam only:
    ////////////
    //////////// type               TypeEnum        no                  setType(*)               None                  eStringTypeDefault
    ////////////
    //////////// Properties on PathParam only:
    ////////////
    //////////// multiPathTable     bool            no                  setAsMultiPathTable(*)   None                  False
    ////////////
    ////////////
    //////////// Properties on GroupParam only:
    ////////////
    //////////// isTab              bool            no                  setAsTab(*)              None                   False
    ////////////
    ////////////
    ////////////  (1): animates is set to True by default only if it is one of the following parameters:
    ////////////  IntParam Int2DParam Int3DParam
    ////////////  DoubleParam Double2DParam Double3DParam
    ////////////  ColorParam
    ////////////
    ////////////  Note that ParametricParam , GroupParam, PageParam, ButtonParam, FileParam, OutputFileParam,
    ////////////  PathParam cannot animate at all.

    IntParam* createIntParam(const QString& name, const QString& label);
    Int2DParam* createInt2DParam(const QString& name, const QString& label);
    Int3DParam* createInt3DParam(const QString& name, const QString& label);
    DoubleParam* createDoubleParam(const QString& name, const QString& label);
    Double2DParam* createDouble2DParam(const QString& name, const QString& label);
    Double3DParam* createDouble3DParam(const QString& name, const QString& label);
    BooleanParam* createBooleanParam(const QString& name, const QString& label);
    ChoiceParam* createChoiceParam(const QString& name, const QString& label);
    ColorParam* createColorParam(const QString& name, const QString& label, bool useAlpha);
    StringParam* createStringParam(const QString& name, const QString& label);
    FileParam* createFileParam(const QString& name, const QString& label);
    OutputFileParam* createOutputFileParam(const QString& name, const QString& label);
    PathParam* createPathParam(const QString& name, const QString& label);
    ButtonParam* createButtonParam(const QString& name, const QString& label);
    SeparatorParam* createSeparatorParam(const QString& name, const QString& label);
    GroupParam* createGroupParam(const QString& name, const QString& label);
    PageParam* createPageParam(const QString& name, const QString& label);
    ParametricParam* createParametricParam(const QString& name, const QString& label, int nbCurves);

    bool removeParam(Param* param);

    /**
     * @brief To be called once you have added or removed any user parameter to update the GUI with the changes.
     * This may be expensive so try to minimize the number of calls to this function.
     **/
    void refreshUserParamsGUI();

    virtual bool onKnobValueChanged(KnobI* k,
                                    ValueChangedReasonEnum reason,
                                    double time,
                                    ViewSpec view,
                                    bool originatedFromMainThread)
    {
        Q_UNUSED(k);
        Q_UNUSED(reason);
        Q_UNUSED(time);
        Q_UNUSED(view);
        Q_UNUSED(originatedFromMainThread);

        return false;
    }
};

class Effect
    : public Group, public UserParamHolder
{
    NodeWPtr _node;

public:

    Effect(const NodePtr& node);

    ~Effect();

    NodePtr getInternalNode() const;

    bool isReaderNode();

    bool isWriterNode();

    /**
     * @brief Removes the node from the project. It will no longer be possible to use it.
     * @param autoReconnect If set to true, outputs connected to this node will try to connect to the input of this node automatically.
     **/
    void destroy(bool autoReconnect = true);

    /**
     * @brief Returns the maximum number of inputs that can be connected to the node.
     **/
    int getMaxInputCount() const;

    /**
     * @brief Determines whether a connection is possible for the given node at the given input number.
     **/
    bool canConnectInput(int inputNumber, const Effect* node) const;

    /**
     * @brief Attempts to connect the Effect 'input' to the given inputNumber.
     * This function uses canSetInput(int,Effect) to determine whether a connection is possible.
     * There's no auto-magic behind this function: you must explicitely disconnect any already connected Effect
     * to the given inputNumber otherwise this function will return false.
     **/
    bool connectInput(int inputNumber, const Effect* input);

    /**
     * @brief Disconnects any Effect connected to the given inputNumber.
     **/
    void disconnectInput(int inputNumber);

    /**
     * @brief Returns the Effect connected to the given inputNumber
     * @returns Pointer to an Effect, the caller is responsible for freeing it.
     **/
    Effect* getInput(int inputNumber) const;
    Effect* getInput(const QString& inputLabel) const;

    /**
     * @brief Returns the name of the Effect as used internally
     **/
    QString getScriptName() const;

    /**
     * @brief Set the internal script name of the effect
     * @returns False upon failure, True upon success.
     **/
    bool setScriptName(const QString& scriptName);

    /**
     * @brief Returns the name of the Effect as displayed on the GUI
     **/
    QString getLabel() const;

    /**
     * @brief Set the name of the Effect as used on the GUI
     **/
    void setLabel(const QString& name);

    /**
     * @brief Returns the ID of the plug-in embedded into the Effect
     **/
    QString getPluginID() const;

    /**
     * @brief Returns the label of the input at the given index
     **/
    QString getInputLabel(int inputNumber);

    /**
     * @brief Returns a list of all parameters for the Effect. These are the parameters located in the settings panel
     * on the GUI.
     **/
    std::list<Param*> getParams() const;

    /**
     * @brief Returns a pointer to the Param named after the given name or NULL if no parameter with the given name could be found.
     **/
    Param* getParam(const QString& name) const;

    /**
     * @brief When called, all parameter changes will not call the callback onParamChanged and will not attempt to trigger a new render.
     * A call to allowEvaluation() should be made to restore the state of the Effect
     **/
    void beginChanges();

    void endChanges();

    /**
     * @brief Get the current time on the timeline or the time of the frame being rendered by the caller thread if a render
     * is ongoing in that thread.
     **/
    int getCurrentTime() const;

    /**
     * @brief Set the position of the top left corner of the node in the nodegraph. This is ignored in background mode.
     **/
    void setPosition(double x, double y);
    void getPosition(double* x, double* y) const;

    /**
     * @brief Set the size of the bounding box of the node in the nodegraph. This is ignored in background mode.
     **/
    void setSize(double w, double h);
    void getSize(double* w, double* h) const;

    /**
     * @brief Get the colour of the node as it appears on the nodegraph.
     **/
    void getColor(double* r, double *g, double* b) const;
    void setColor(double r, double g, double b);

    /**
     * @brief Returns true if the node is selected in the nodegraph
     **/
    bool isNodeSelected() const;

    /**
     * @brief Get the user page param. Note that user created params (with the function above) may only be added to user created pages,
     * that is, the page returned by getUserPageParam() or in any page created by createPageParam().
     * This function never returns NULL, it will ensure that the User page exists.
     **/
    PageParam* getUserPageParam() const;
    ////////////////////////////////////////////////////////////////////////////

    /**
     * @brief Get the roto context for this node if it has any. At the time of writing only the Roto node has a roto context.
     **/
    Roto* getRotoContext() const;

    /**
     * @brief Get the tracker context for this node if it has any. Currently only Tracker has one.
     **/
    Tracker* getTrackerContext() const;

    RectD getRegionOfDefinition(double time, int /* Python API: do not use ViewIdx */ view) const;

    static Param* createParamWrapperForKnob(const KnobIPtr& knob);

    void setSubGraphEditable(bool editable);

    bool addUserPlane(const QString& planeName, const QStringList& channels);

    std::list<ImageLayer> getAvailableLayers(int inputNb) const;

    double getFrameRate() const;

    double getPixelAspectRatio() const;

    NATRON_NAMESPACE::ImageBitDepthEnum getBitDepth() const;
    NATRON_NAMESPACE::ImagePremultiplicationEnum getPremult() const;

    void setPagesOrder(const QStringList& pages);
};

NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT

#endif // NODEWRAPPER_H
