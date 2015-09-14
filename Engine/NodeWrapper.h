/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

/**
* @brief Simple wrap for the EffectInstance and Node class that is the API we want to expose to the Python
* Engine module.
**/

#ifndef NODEWRAPPER_H
#define NODEWRAPPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Engine/Knob.h" // KnobI
#include "Engine/NodeGroupWrapper.h" // Goup
//#include "Engine/ParameterWrapper.h"
#include "Engine/RectD.h"

namespace Natron {
class Node;
}

class Roto;
class Param;
class IntParam;
class Int2DParam;
class Int3DParam;
class BooleanParam;
class DoubleParam;
class Double2DParam;
class Double3DParam;
class ChoiceParam;
class ColorParam;
class StringParam;
class FileParam;
class OutputFileParam;
class PathParam;
class ButtonParam;
class GroupParam;
class PageParam;
class ParametricParam;
class KnobHolder;

class UserParamHolder
{
    KnobHolder* _holder;
public:
    
    UserParamHolder()
    : _holder(0)
    {
        
    }
    
    UserParamHolder(KnobHolder* holder)
    : _holder(holder)
    {
        
    }
    
    virtual ~UserParamHolder() {}
    
    void setHolder(KnobHolder* holder)
    {
        assert(!_holder);
        _holder = holder;
    }
    
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
    //////////// persistent         bool            yes                 setPersistant(*)        getIsPersistant       True
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
    
    IntParam* createIntParam(const std::string& name, const std::string& label);
    Int2DParam* createInt2DParam(const std::string& name, const std::string& label);
    Int3DParam* createInt3DParam(const std::string& name, const std::string& label);
    
    DoubleParam* createDoubleParam(const std::string& name, const std::string& label);
    Double2DParam* createDouble2DParam(const std::string& name, const std::string& label);
    Double3DParam* createDouble3DParam(const std::string& name, const std::string& label);
    
    BooleanParam* createBooleanParam(const std::string& name, const std::string& label);
    
    ChoiceParam* createChoiceParam(const std::string& name, const std::string& label);
    
    ColorParam* createColorParam(const std::string& name, const std::string& label, bool useAlpha);
    
    StringParam* createStringParam(const std::string& name, const std::string& label);
    
    FileParam* createFileParam(const std::string& name, const std::string& label);
    
    OutputFileParam* createOutputFileParam(const std::string& name, const std::string& label);
    
    PathParam* createPathParam(const std::string& name, const std::string& label);
    
    ButtonParam* createButtonParam(const std::string& name, const std::string& label);
    
    GroupParam* createGroupParam(const std::string& name, const std::string& label);
    
    PageParam* createPageParam(const std::string& name, const std::string& label);
    
    ParametricParam* createParametricParam(const std::string& name, const std::string& label,int nbCurves);
    
    bool removeParam(Param* param);
    
    /**
     * @brief To be called once you have added or removed any user parameter to update the GUI with the changes.
     * This may be expensive so try to minimize the number of calls to this function.
     **/
    void refreshUserParamsGUI();

};

class Effect : public Group, public UserParamHolder
{
    boost::shared_ptr<Natron::Node> _node;
    
public:
    
    Effect(const boost::shared_ptr<Natron::Node>& node);
    
    ~Effect();
    
    boost::shared_ptr<Natron::Node> getInternalNode() const;
    
    
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
    bool canConnectInput(int inputNumber,const Effect* node) const;
    
    /**
     * @brief Attempts to connect the Effect 'input' to the given inputNumber.
     * This function uses canSetInput(int,Effect) to determine whether a connection is possible.
     * There's no auto-magic behind this function: you must explicitely disconnect any already connected Effect
     * to the given inputNumber otherwise this function will return false.
     **/
    bool connectInput(int inputNumber,const Effect* input);

    /**
     * @brief Disconnects any Effect connected to the given inputNumber.
     **/
    void disconnectInput(int inputNumber);
    
    /**
     * @brief Returns the Effect connected to the given inputNumber
     * @returns Pointer to an Effect, the caller is responsible for freeing it.
     **/
    Effect* getInput(int inputNumber) const;
    
    /**
     * @brief Returns the name of the Effect as used internally
     **/
    std::string getScriptName() const;
    
    /**
     * @brief Set the internal script name of the effect
     * @returns False upon failure, True upon success.
     **/
    bool setScriptName(const std::string& scriptName);
    
    /**
     * @brief Returns the name of the Effect as displayed on the GUI
     **/
    std::string getLabel() const;
    
    /**
     * @brief Set the name of the Effect as used on the GUI
     **/
    void setLabel(const std::string& name);
    
    /**
     * @brief Returns the ID of the plug-in embedded into the Effect
     **/
    std::string getPluginID() const;
    
    /**
     * @brief Returns the label of the input at the given index
     **/
    std::string getInputLabel(int inputNumber);
    
    /**
     * @brief Returns a list of all parameters for the Effect. These are the parameters located in the settings panel
     * on the GUI.
     **/
    std::list<Param*> getParams() const;
    
    /**
     * @brief Returns a pointer to the Param named after the given name or NULL if no parameter with the given name could be found.
     **/
    Param* getParam(const std::string& name) const;
    
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
    void setPosition(double x,double y);
    void getPosition(double* x, double* y) const;
    
    /**
     * @brief Set the size of the bounding box of the node in the nodegraph. This is ignored in background mode.
     **/
    void setSize(double w,double h);
    void getSize(double* w, double* h) const;
    
    /**
     * @brief Get the colour of the node as it appears on the nodegraph.
     **/
    void getColor(double* r,double *g, double* b) const;
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
     * @brief Create a new child node for this node, currently this is only supported by the tracker node.
     **/
    Effect* createChild();
    
    /**
     * @brief Get the roto context for this node if it has any. At the time of writing only the Roto node has a roto context.
     **/
    Roto* getRotoContext() const;
    
    RectD getRegionOfDefinition(int time,int view) const;
    
    static Param* createParamWrapperForKnob(const boost::shared_ptr<KnobI>& knob);
    
    void setSubGraphEditable(bool editable);
    
    bool addUserPlane(const std::string& planeName, const std::vector<std::string>& channels);
};

#endif // NODEWRAPPER_H
