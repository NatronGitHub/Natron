/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#ifndef PLUGIN_H
#define PLUGIN_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <set>
#include <map>
#include <list>
#include <QtCore/QString>
#include <QtCore/QStringList>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Global/Enums.h"
#include "Engine/PluginActionShortcut.h"
#include "Engine/PropertiesHolder.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief x1 pointer property indicating the function used to create an instance of the plug-in.
 * For OpenFX plug-in the actual OpenFX plug-in instance is created in the OfxEffectInstance class.
 * Default value - NULL
 * Must be set
 **/
#define kNatronPluginPropCreateFunc "NatronPluginPropCreateFunc"

/**
 * @brief x1 pointer property indicating the function used to create a render instance of the plug-in.
 * Default value - NULL
 * Must be set
 **/
#define kNatronPluginPropCreateRenderCloneFunc "NatronPluginPropCreateRenderCloneFunc"

/**
 * @brief x1 std::string property indicating the ID of the plug-in
 * Default value - Empty
 * Must be set
 **/
#define kNatronPluginPropID "NatronPluginPropID"

/**
 * @brief x1 std::string property indicating the label of the plug-in as it appears in the user interface
 * Default value - Empty
 * Must be set
 **/
#define kNatronPluginPropLabel "NatronPluginPropLabel"

/**
 * @brief x2 unsigned int property indicating the version of the plug-in.
 * Default value - 0, 0
 * Must be set
 **/
#define kNatronPluginPropVersion "NatronPluginPropVersion"

/**
 * @brief x1 std::string property (optional) indicating the description of the plug-in as seen in the documentation
 * or help button of the plug-in.
 * Default value - Empty
 **/
#define kNatronPluginPropDescription "NatronPluginPropDescription"

/**
 * @brief x1 bool property (optiona) indicating whether the kNatronPluginPropDescription is encoded in markdown or plain text.
 * Default value - false
 **/
#define kNatronPluginPropDescriptionIsMarkdown "NatronPluginPropDescriptionIsMarkdown"

/**
 * @brief x1 std::string property (optional) indicating the absolute directory path where the resources of the plug-in should be found.
 * For statically linked plug-ins, this may be left empty in which case Natron will look for resources in the Qt resources of the application.
 * Default value - PLUGIN_DEFAULT_RESOURCES_PATH
 **/
#define kNatronPluginPropResourcesPath "NatronPluginPropResourcesPath"


/**
 * @brief x1 std::string property (optional) indicating the filename of a PNG icon to display for the plugin.
 * The filename must be relative to the kNatronPluginPropResourcesPath location.
 * Default value - Empty
 **/
#define kNatronPluginPropIconFilePath "NatronPluginPropIconFilePath"

/**
 * @brief xN std::string property (optional) indicating the grouping into which the plug-in should be set
 * in the toolbar.
 * Default value - PLUGIN_GROUP_DEFAULT
 **/
#define kNatronPluginPropGrouping "NatronPluginPropGrouping"

/**
 * @brief x2 int property (optional) indicating the (key,modifier) shortcut to use to create an instance of this plug-in.
 * The user can always change it from the shortcut editor.
 * Because of different keyboard layouts, the shift modifier is ignored.
 * Default value - 0, 0
 **/
#define kNatronPluginPropShortcut "NatronPluginPropShortcut"

/**
 * @brief xN std::string property (optional) indicating for each grouping level the filename of a PNG icon
 * to be used in sub-menus.
 * This property must have the same dimension as kNatronPluginPropGrouping or be empty.
 * The filename must be relative to the kNatronPluginPropResourcesPath location.
 * Default value - Empty
 **/
#define kNatronPluginPropGroupIconFilePath "NatronPluginPropGroupIconFilePath"

/**
 * @brief x1 std::string property (optional) indicating for a PyPlug the absolute file path to the PyPlug script.
 * Default value - Empty
 **/
#define kNatronPluginPropPyPlugScriptAbsoluteFilePath "NatronPluginPropPyPlugScriptAbsoluteFilePath"

/**
 * @brief x1 std::string property (optional) indicating the ID of the plug-in container to use for the PyPlug
 * Default value - Empty
 * If empty, Natron assume the PyPlug is one of a default Group node.
 **/
#define kNatronPluginPropPyPlugContainerID "NatronPluginPropPyPlugContainerID"

/**
 * @brief x1 bool property (optional) indicating for a PyPlug whether it is encoded using old Python scripts or to
 * the newer YAML-based format.
 * Default value - false
 **/
#define kNatronPluginPropPyPlugIsPythonScript "NatronPluginPropPyPlugIsPythonScript"

/**
 * @brief x1 bool property (optional) indicating for a PyPlug if this is a toolset or not.
 * Toolsets are always encoded in Python.
 * Default value - Empty
 **/
#define kNatronPluginPropPyPlugIsToolset "NatronPluginPropPyPlugIsToolset"

/**
 * @brief x1 std::string property (optional) indicating for a PyPlug the file path of a Python script where callbacks
 * used by the PyPlug can be defined. The file path is relative to the directory where kNatronPluginPropPyPlugScriptAbsoluteFilePath is
 * contained.
 * Default value - Empty
 **/
#define kNatronPluginPropPyPlugExtScriptFile "NatronPluginPropPyPlugExtScriptFile"

/**
 * @brief x1 pointer property (optional) indicating for an OpenFX plug-in the pointer to the internal
 * OFX::Host::ImageEffect::ImageEffectPlugin structure.
 * Default value - NULL
 **/
#define kNatronPluginPropOpenFXPluginPtr "NatronPluginPropOpenFXPluginPtr"

/**
 * @brief x1 bool property (optional) indicating whether a plug-in is deprecated or not.
 * When true, the plug-in cannot be created by the user but can still be created when loading older projects.
 * Default value - false
 **/
#define kNatronPluginPropIsDeprecated "NatronPluginPropIsDeprecated"

/**
 * @brief x1 bool property (optional) indicating whether a plug-in is for internal use or not.
 * When true, the plug-in cannot be created by the user at all, only via Natron internally.
 * Default value - false
 **/
#define kNatronPluginPropIsInternalOnly "NatronPluginPropIsInternalOnly"

/**
 * @brief xN string property (optional) indicating for a reader or writer plug-in the file formats that it supported in input or output
 * Default value - Empty
 **/
#define kNatronPluginPropSupportedExtensions "NatronPluginPropSupportedExtensions"

/**
 * @brief x1 double property (optional) indicating whether a plug-in is for internal use or not.
 * This is used to choose the default plug-ins that should be used to read/write a specific file format.
 * Default Value - -1 for a plugin
 * Valid Values - The value should be between 0 and 100.
 *   0 - means bad implementation
 *   50 - means good implementation without specific optimization
 *   100 - means good implementation with specific optimizations
 *   -1 - means the plugin is not evaluated
 **/
#define kNatronPluginPropIOEvaluation "NatronPluginPropIOEvaluation"

/**
 * @brief x1 std::bitset<4> property indicating the supported number of components in output.
 * This is a bitset: each bit tells whether the plug-in supports N comps
 * (e.g: if the 4th bit is set to 1 it means theinput supports RGBA)
 * Default value - 0000, the plug-in will not be loaded
 * Must be specified
 **/
#define kNatronPluginPropOutputSupportedComponents "NatronPluginPropOutputSupportedComponents"

/**
 * @brief xN ImageBitDepthEnum property indicating the supported bitdepths in output.
 * This is a bitset: each bit tells whether the plug-in supports N comps
 * (e.g: if the 4th bit is set to 1 it means theinput supports RGBA)
 * Default value - Empty, the plug-in will not be loaded
 * Must be specified
 **/
#define kNatronPluginPropOutputSupportedBitDepths "NatronPluginPropOutputSupportedBitDepths"

/**
 * @brief x1 bool property (optional) indicating whether the plug-in behaves differently depending on the number of views in the project
 * Default value - false
 **/
#define kNatronPluginPropViewAware "NatronPluginPropViewAware"

/**
 * @brief x1 ViewInvarianceLevel property (optional) indicating for a plug-in that has the property kNatronPluginPropViewAware set to
 * true whether images vary depending on the view.
 * Default value - eViewInvarianceAllViewsVariant
 **/
#define kNatronPluginPropViewInvariant "NatronPluginPropViewInvariant"

/**
 * @brief x1 bool property (optional) indicating whether the plug-in knows about the multi-plane system and implements getLayersProducedAndNeeded
 * Default value - false
 **/
#define kNatronPluginPropMultiPlanar "NatronPluginPropMultiPlanar"

/**
 * @brief x1 bool property (optional) indicating whether the plug-in produces a different render while in draft mode
 * Default value - false
 **/
#define kNatronPluginPropSupportsDraftRender "NatronPluginPropSupportsDraftRender"

/**
 * @brief x1 bool property (optional) indicating whether the plug-in wants RGBA checkboxes added by the host or not.
 * Default value - false
 **/
#define kNatronPluginPropHostChannelSelector "NatronPluginPropHostChannelSelector"

/**
 * @brief x1 std::bitset<4> property (optional) indicating whether the plug-in wants each RGBA checkbox turned on or off by default.
 * The kNatronPluginPropHostChannelSelector property must be set to true
 * Default value - 0000
 **/
#define kNatronPluginPropHostChannelSelectorValue "NatronPluginPropHostChannelSelectorValue"


/**
 * @brief x1 bool property (optional) indicating whether the plug-in wants the host to add a mix parameter that dissolves between
 * the full effect at 1 to the input unmodified image at 0.
 * Default value - false
 **/
#define kNatronPluginPropHostMix "NatronPluginPropHostMix"

/**
 * @brief x1 bool property (optional) indicating whether the plug-in wants the host to add mask automatically the rendered
 * image.
 * Default value - false
 **/
#define kNatronPluginPropHostMask "NatronPluginPropHostMask"

/**
 * @brief x1 bool property (optional) indicating whether the plug-in wants the host to add a plane selector for the output and input(s)
 * of the effect. This is only relevant if the plug-in has its property kNatronPluginPropMultiPlanar set to false.
 * Default value - false
 **/
#define kNatronPluginPropHostPlaneSelector "NatronPluginPropHostPlaneSelector"

/**
* @brief x1 bool property (optional) indicating whether the plug-in can handle multiple inputs with different pixel aspect ratios
* Default value - false
**/
#define kNatronPluginPropSupportsMultiInputsPAR "NatronPluginPropSupportsMultiInputsPAR"

/**
 * @brief x1 bool property (optional) indicating whether the plug-in can handle multiple inputs with different image bit depths
 * Default value - false
 **/
#define kNatronPluginPropSupportsMultiInputsBitDepths "NatronPluginPropSupportsMultiInputsBitDepths"

/**
 * @brief x1 bool property (optional) indicating whether the plug-in can handle multiple inputs with different frame rate
 * Default value - false
 **/
#define kNatronPluginPropSupportsMultiInputsFPS "NatronPluginPropSupportsMultiInputsFPS"

/**
 * @brief x1 bool property (optional) indicating for a plug-in that has the property kNatronPluginPropMultiPlanar set to true whether
 * the plug-ins prefers all image planes rendered at once in a single render action call.
 * Default value - false
 **/
#define kNatronPluginPropRenderAllPlanesAtOnce "NatronPluginPropRenderAllPlanesAtOnce"

/**
 * @brief x1 PlanePassThroughEnum property (optional) indicating for a plug-in that has the property kNatronPluginPropMultiPlanar set to true whether
 * the planes should be asked on the input if not rendered by the plug-in or blocked.
 * Default value - ePassThroughPassThroughNonRenderedPlanes
 **/
#define kNatronPluginPropPlanesPassThrough "NatronPluginPropPlanesPassThrough"

/**
 * @brief A node of a tree data structure representing the grouping hierarchy of plug-ins. This is mainly for the GUI so it can create its toolbuttons and menus
 * If this node is a leaf it will have a valid plugin, otherwise it has just a name for menus
 **/
class PluginGroupNode
{

    std::list<PluginGroupNodePtr> _children;
    PluginGroupNodeWPtr _parent;
    PluginWPtr _plugin;

    // the name of the node in the hierarchy
    QString _name;

    // the icon file path of this menu item
    QString _iconFilePath;

public:
    PluginGroupNode(const PluginPtr& plugin,
                    const QString& name,
                    const QString& iconFilePath)
        : _children()
        , _parent()
        , _plugin(plugin)
        , _name(name)
        , _iconFilePath(iconFilePath)
    {
    }

    PluginPtr getPlugin() const
    {
        return _plugin.lock();
    }


    const std::list<PluginGroupNodePtr> & getChildren() const
    {
        return _children;
    }

    void tryAddChild(const PluginGroupNodePtr& plugin);

    void tryRemoveChild(const PluginGroupNodePtr& plugin);

    PluginGroupNodePtr getParent() const
    {
        return _parent.lock();
    }

    void setParent(const PluginGroupNodePtr& parent)
    {
        _parent = parent;
    }

    const QString& getTreeNodeName() const
    {
        return _name;
    }

    const QString& getTreeNodeIconFilePath() const
    {
        return _iconFilePath;
    }

    // the node ID is the same as name for menu items and the pluginID if this is a leaf
    QString getTreeNodeID() const;

};

// Identify a preset for a node
struct PluginPresetDescriptor
{
    QString presetFilePath;
    QString presetLabel;
    QString presetIconFile;
    Key symbol;
    KeyboardModifiers modifiers;
};

/**
 * @brief A Natron plug-in that can be instantiated in nodes
 **/
class Plugin : public PropertiesHolder
{
    // Plug-in label modified to reflect presence of multiple version of the same plug-in
    std::string _labelWithoutSuffix;

    // The shortcuts for the plug-in actions when it has knobs
    // that can be instantiated in the Viewer interface (such as Roto or Tracker).
    std::list<PluginActionShortcut> _actionShortcuts;

    // List of presets
    std::vector<PluginPresetDescriptor> _presets;

    // The mutex to use for a plug-in that has
    // an unsafe render thread safety.
    boost::shared_ptr<QMutex> _pluginLock;

    // OFX only: The context that was passed to describeInContext the first time
    ContextEnum _openfxContext;

    // OFX only: The descriptor returned by the plug-in by describeInContext
    OFX::Host::ImageEffect::Descriptor* _openfxDescriptor;

    // When not enabled, the user cannot create an instance of this plug-in.
    bool _isEnabled;

    // True if this plug-in is the highest major version of the plug-in
    bool _isHighestVersion;

    // When disabled, even though the plug-in flags that it can handle OpenGL rendering, Natron will not permit
    // any render issued to the plug-in to use OpenGL.
    bool _openGLEnabled;

    // When enabled, Natron follows what the plug-in specifies for the render-thread safety. When disabled Natron always
    // assume the plug-in is unsafe.
    bool _multiThreadEnabled;

    // When enabled, Natron follows what the plug-in specifies for the render scale support. When disabled Natron always
    // pass images of scale 1 to the plug-in.
    bool _renderScaleEnabled;

    std::vector<InputDescriptionPtr> _inputsDescription;

    // Default properties of the effect, each effect instance have a local copy of them and may modify them
    EffectDescriptionPtr _effectDescription;

private:

    virtual void initializeProperties() const OVERRIDE FINAL;

    Plugin();

public:

    static PluginPtr create(EffectBuilder createEffectFunc,
                            EffectRenderCloneBuilder createCloneFunc,
                            const std::string &pluginID,
                            const std::string &pluginLabel,
                            unsigned int majorVersion,
                            unsigned int minorVersion,
                            const std::vector<std::string> &pluginGrouping,
                            const std::vector<std::string> &groupIconFilePath = std::vector<std::string>());

    virtual ~Plugin();

    boost::shared_ptr<QMutex> getPluginLock() const;

    std::string getPluginID() const;


    /**
     * @brief Flag stored here so we don't have to recompute it.
     **/
    void setIsHighestMajorVersion(bool isHighest);
    bool getIsHighestMajorVersion() const;


    void addPresetFile(const PluginPresetDescriptor& preset);
    void sortPresetsByLabel();
    const std::vector<PluginPresetDescriptor>& getPresetFiles() const;

    std::string getPluginShortcutGroup() const;

    static std::string makeLabelWithoutSuffix(const std::string& label);
    std::string getLabelVersionMajorMinorEncoded() const;
    std::string getLabelVersionMajorEncoded() const;
    std::string getLabelWithoutSuffix() const;
    void setLabelWithoutSuffix(const std::string& label);
    std::string generateUserFriendlyPluginID() const;
    std::string generateUserFriendlyPluginIDMajorEncoded() const;
    std::string getPluginLabel() const;

    int getMajorVersion() const;
    int getMinorVersion() const;

    OFX::Host::ImageEffect::Descriptor* getOfxDesc(ContextEnum* ctx) const;
    void setOfxDesc(OFX::Host::ImageEffect::Descriptor* desc, ContextEnum ctx);

    bool getIsUserCreatable() const;

    bool isEnabled() const;
    void setEnabled(bool b);

    bool isRenderScaleEnabled() const;
    bool isMultiThreadingEnabled() const;
    bool isOpenGLEnabled() const;

    void setRenderScaleEnabled(bool b);
    void setMultiThreadingEnabled(bool b);
    void setOpenGLEnabled(bool b);

    void addActionShortcut(const PluginActionShortcut& shortcut);
    const std::list<PluginActionShortcut>& getShortcuts() const;

    QStringList getGroupingAsQStringList() const;
    std::string getGroupingString() const;

    const std::vector<InputDescriptionPtr>& getInputsDescription();

    void addInputDescription(const InputDescriptionPtr& desc);

    EffectDescriptionPtr getEffectDescriptor() const;

};

struct Plugin_compare_version
{
    bool operator() (const PluginPtr& lhs,
                     const PluginPtr& rhs) const
    {
        // see also OFX::Host::Plugin::trumps()
        return ( ( lhs->getMajorVersion() < rhs->getMajorVersion() ) ||
                ( ( lhs->getMajorVersion() == rhs->getMajorVersion() ) &&
                  ( lhs->getMinorVersion() < rhs->getMinorVersion() ) ) );
    }
};

typedef std::set<PluginPtr, Plugin_compare_version> PluginVersionsOrdered;
typedef std::map<std::string, PluginVersionsOrdered> PluginsMap;

struct IOPluginEvaluation
{
    std::string pluginID;
    double evaluation;

    IOPluginEvaluation()
        : pluginID()
        , evaluation(0)
    {
    }

    IOPluginEvaluation(const std::string& p,
                       double e)
        : pluginID(p)
        , evaluation(e)
    {}
};

struct IOPluginEvaluation_CompareLess
{
    bool operator() (const IOPluginEvaluation& lhs,
                     const IOPluginEvaluation& rhs) const
    {
        return lhs.evaluation < rhs.evaluation;
    }
};

struct FormatExtensionCompareCaseInsensitive
{
    bool operator() (const std::string& lhs, const std::string& rhs) const;
};

typedef std::set<IOPluginEvaluation, IOPluginEvaluation_CompareLess> IOPluginSetForFormat;
// For each extension format (lower case) a set of plug-in IDs sorted by increasing evaluation order.
// The best plug-in for a format is the set.rbegin()
typedef std::map<std::string, IOPluginSetForFormat, FormatExtensionCompareCaseInsensitive> IOPluginsMap;

NATRON_NAMESPACE_EXIT

#endif // PLUGIN_H
