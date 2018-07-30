/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef NATRON_ENGINE_KNOBTYPES_H
#define NATRON_ENGINE_KNOBTYPES_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <string>
#include <map>

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QVector>
#include <QtCore/QMutex>
#include <QtCore/QString>

#include "Global/GlobalDefines.h"
#include "Engine/ChoiceOption.h"
#include "Engine/Knob.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"
#include "Engine/ChoiceOption.h"


NATRON_NAMESPACE_ENTER


#define kFontSizeTag "<font size=\""
#define kFontColorTag "color=\""
#define kFontFaceTag "face=\""
#define kFontEndTag "</font>"
#define kBoldStartTag "<b>"
#define kBoldEndTag "</b>"
#define kItalicStartTag "<i>"
#define kItalicEndTag "</i>"

#define kColorKnobDefaultUIColorspaceName "sRGB"

#define kKeyframePropChoiceOptionLabel "KeyframePropChoiceOptionLabel"

inline KnobBoolBasePtr
toKnobBoolBase(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobBoolBase>(knob);
}

inline KnobDoubleBasePtr
toKnobDoubleBase(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobDoubleBase>(knob);
}

inline KnobIntBasePtr
toKnobIntBase(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobIntBase>(knob);
}

inline KnobStringBasePtr
toKnobStringBase(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobStringBase>(knob);
}

/******************************KnobInt**************************************/
struct KnobIntPrivate;
class KnobInt
    : public QObject, public KnobIntBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobInt(const KnobHolderPtr& holder,
            const std::string &name,
            int dimension);

    KnobInt(const KnobHolderPtr& holder, const KnobIPtr& mainKnob);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                             const std::string &name,
                             int dimension)
    {
        return KnobHelperPtr(new KnobInt(holder, name, dimension));
    }

    static KnobHelperPtr createRenderClone(const KnobHolderPtr& holder,
                                           const KnobIPtr& mainKnob)
    {
        return KnobIntPtr(new KnobInt(holder, mainKnob));
    }

    virtual ~KnobInt();

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return true;
    }

    void disableSlider();


    bool isSliderDisabled() const;

    static const std::string & typeNameStatic();

    void setAsRectangle();

    bool isRectangle() const;

    void setValueCenteredInSpinBox(bool enabled);

    bool isValueCenteredInSpinBox() const;

    // For 2D int parameters, the UI will have a keybind recorder
    // and the first dimension stores the symbol and the 2nd the modifiers
    void setAsShortcutKnob(bool isShortcutKnob);
    bool isShortcutKnob() const;
public:

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    void setIncrement(int incr, DimIdx index = DimIdx(0));

    void setIncrement(const std::vector<int> &incr);

    const std::vector<int> &getIncrements() const;


Q_SIGNALS:


    void incrementChanged(double incr, DimIdx index);

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    boost::shared_ptr<KnobIntPrivate> _imp;
   
    static const std::string _typeNameStr;
};

inline KnobIntPtr
toKnobInt(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobInt>(knob);
}

/******************************KnobBool**************************************/

class KnobBool
    :  public KnobBoolBase
{
private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobBool(const KnobHolderPtr& holder,
             const std::string &name,
             int dimension);

    KnobBool(const KnobHolderPtr& holder, const KnobIPtr& mainKnob);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &name,
                                int dimension)
    {
        return KnobHelperPtr(new KnobBool(holder, name, dimension));
    }
    
    static KnobHelperPtr createRenderClone(const KnobHolderPtr& holder,
                                           const KnobIPtr& mainKnob)
    {
        return KnobBoolPtr(new KnobBool(holder, mainKnob));
    }


    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    /// Can this type be animated?
    /// BooleanParam animation may not be quite perfect yet,
    /// @see Curve::getValueAt() for the animation code.
    static bool canAnimateStatic()
    {
        return true;
    }

    static const std::string & typeNameStatic();
    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }


private:

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
};

inline KnobBoolPtr
toKnobBool(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobBool>(knob);
}

/******************************KnobDouble**************************************/

struct KnobDoublePrivate;
class KnobDouble
    :  public QObject, public KnobDoubleBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobDouble(const KnobHolderPtr& holder,
               const std::string &name,
               int dimension);

    KnobDouble(const KnobHolderPtr& holder, const KnobIPtr& mainKnob);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &name,
                                int dimension)
    {
        return KnobHelperPtr(new KnobDouble(holder, name, dimension));
    }

    static KnobHelperPtr createRenderClone(const KnobHolderPtr& holder,
                                           const KnobIPtr& mainKnob)
    {
        return KnobDoublePtr(new KnobDouble(holder, mainKnob));
    }

    virtual ~KnobDouble();

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return true;
    }

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    void disableSlider();

    bool isSliderDisabled() const;

    const std::vector<double> &getIncrements() const;
    const std::vector<int> &getDecimals() const;

    void setIncrement(double incr, DimIdx index = DimIdx(0));

    void setDecimals(int decis, DimIdx index = DimIdx(0));

    void setIncrement(const std::vector<double> &incr);

    void setDecimals(const std::vector<int> &decis);

    static const std::string & typeNameStatic();

    ValueIsNormalizedEnum getValueIsNormalized(DimIdx dimension) const;

    void setValueIsNormalized(DimIdx dimension,
                              ValueIsNormalizedEnum state);
    void setSpatial(bool spatial);

    bool getIsSpatial() const;
    /**
     * @brief Normalize the default values, set the _defaultValuesAreNormalized to true and
     * calls setDefaultValue with the good parameters.
     * Later when restoring the default values, this flag will be used to know whether we need
     * to denormalize the default stored values to the set the "live" values.
     * Hence this SHOULD NOT bet set for old deprecated < OpenFX 1.2 normalized parameters otherwise
     * they would be denormalized before being passed to the plug-in.
     *
     * If *all* the following conditions hold:
     * - this is a double value
     * - this is a non normalised spatial double parameter, i.e. kOfxParamPropDoubleType is set to one of
     *   - kOfxParamDoubleTypeX
     *   - kOfxParamDoubleTypeXAbsolute
     *   - kOfxParamDoubleTypeY
     *   - kOfxParamDoubleTypeYAbsolute
     *   - kOfxParamDoubleTypeXY
     *   - kOfxParamDoubleTypeXYAbsolute
     * - kOfxParamPropDefaultCoordinateSystem is set to kOfxParamCoordinatesNormalised
     * Knob<T>::resetToDefaultValue should denormalize
     * the default value, using the input size.
     * Input size be defined as the first available of:
     * - the RoD of the "Source" clip
     * - the RoD of the first non-mask non-optional input clip (in case there is no "Source" clip) (note that if these clips are not connected, you get the current project window, which is the default value for GetRegionOfDefinition)

     * see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamPropDefaultCoordinateSystem
     * and http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#APIChanges_1_2_SpatialParameters
     **/
    void setDefaultValuesAreNormalized(bool normalized);

    /**
     * @brief Returns whether the default values are stored normalized or not.
     **/
    bool getDefaultValuesAreNormalized() const;

    /**
     * @brief Denormalize the given value according to the RoD of the attached effect's input's RoD.
     * WARNING: Can only be called once setValueIsNormalized has been called!
     **/
    double denormalize(DimIdx dimension, TimeValue time, double value) const;

    /**
     * @brief Normalize the given value according to the RoD of the attached effect's input's RoD.
     * WARNING: Can only be called once setValueIsNormalized has been called!
     **/
    double normalize(DimIdx dimension, TimeValue time, double value) const;

    void setAsRectangle();

    bool isRectangle() const;
Q_SIGNALS:

    void incrementChanged(double incr, DimIdx index);

    void decimalsChanged(int deci, DimIdx index);

private:

    virtual bool hasModificationsVirtual(const KnobDimViewBasePtr& data, DimIdx dimension) const OVERRIDE FINAL;

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    boost::shared_ptr<KnobDoublePrivate> _imp;

    static const std::string _typeNameStr;
};

inline KnobDoublePtr
toKnobDouble(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobDouble>(knob);
}

/******************************KnobButton**************************************/

struct KnobButtonPrivate;
class KnobButton
    : public KnobBoolBase
{
private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobButton(const KnobHolderPtr& holder,
               const std::string &name,
               int dimension);

    KnobButton(const KnobHolderPtr& holder, const KnobIPtr& mainKnob);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &name,
                                int dimension)
    {
        return KnobHelperPtr(new KnobButton(holder, name, dimension));
    }

    static KnobHelperPtr createRenderClone(const KnobHolderPtr& holder,
                                           const KnobIPtr& mainKnob)
    {
        return KnobButtonPtr(new KnobButton(holder, mainKnob));
    }

    virtual ~KnobButton();

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return false;
    }

    static const std::string & typeNameStatic();

    void setAsRenderButton();

    bool isRenderButton() const;

    // Trigger the knobChanged handler for this knob
    // Returns true if the knobChanged handler was caught and an action was done
    bool trigger();

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    void setCheckable(bool b);

    bool getIsCheckable() const;

    void setAsToolButtonAction(bool b);

    bool getIsToolButtonAction() const;

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
    

    boost::shared_ptr<KnobButtonPrivate> _imp;
};

inline KnobButtonPtr
toKnobButton(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobButton>(knob);
}

/******************************KnobChoice**************************************/



class ChoiceKnobDimView : public ValueKnobDimView<int>
{
public:

    // For a choice parameter we need to know the strings
    std::vector<ChoiceOption> menuOptions;

    // Used in combination with the index stored in "value" in the ValueKnobDimView class, so that we also
    // have the string of the selected option(s) (there may be multiple in case multi-choice is enabled).
    std::vector<ChoiceOption> staticValueOption;

    //  Each item in the list will add a separator after the index specified by the integer.
    std::vector<int> separators;

    // Optional shortcuts visible for menu entries. All items in the menu don't need a shortcut
    // so they are mapped against their index. The string corresponds to a shortcut ID that was registered
    // on the node during the getPluginShortcuts function on the same node.
    std::map<int, std::string> shortcuts;

    // Optional icons for menu entries. All items in the menu don't need an icon
    // so they are mapped against their index.
    std::map<int, std::string> menuIcons;

    // A pointer to a callback called when the "new" item is invoked for a knob.
    // If not set the menu will not have the "new" item in the menu.
    typedef void (*KnobChoiceNewItemCallback)(const KnobChoicePtr& knob);
    KnobChoiceNewItemCallback addNewChoiceCallback;

    // When not empty, the size of the combobox will be fixed so that the content of this string can be displayed entirely.
    // This is so that the combobox can have a fixed custom width.
    std::string textToFitHorizontally;

    // When true the menu is considered cascading
    bool isCascading;

    // For choice menus that may change the entry that was selected by the user may disappear.
    // In this case, if this flag is true a warning will be displayed next to the menu.
    bool showMissingEntryWarning;

    // When the entry corresponding to the index is selected, the combobox frame will get the associated color.
    std::map<int, RGBAColourD> menuColors;

    ChoiceKnobDimView();

    virtual ~ChoiceKnobDimView()
    {
        
    }

    virtual int getValueFromKeyFrame(const KeyFrame& k) OVERRIDE FINAL;

    virtual KeyFrame makeKeyFrame(TimeValue time, const int& value) OVERRIDE FINAL;

    virtual bool setValueAndCheckIfChanged(const int& value) OVERRIDE;

    virtual bool copy(const CopyInArgs& inArgs, CopyOutArgs* outArgs) OVERRIDE;
};


typedef boost::shared_ptr<ChoiceKnobDimView> ChoiceKnobDimViewPtr;

inline ChoiceKnobDimViewPtr
toChoiceKnobDimView(const KnobDimViewBasePtr& data)
{
    return boost::dynamic_pointer_cast<ChoiceKnobDimView>(data);
}

struct KnobChoicePrivate;
class KnobChoice
    : public QObject, public KnobIntBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON


private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobChoice(const KnobHolderPtr& holder,
               const std::string &name,
               int dimension);

    KnobChoice(const KnobHolderPtr& holder, const KnobIPtr& mainInstance);

public:

    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &name,
                                int dimension)
    {
        return KnobHelperPtr(new KnobChoice(holder, name, dimension));
    }

    static KnobHelperPtr createRenderClone(const KnobHolderPtr& holder,
                                           const KnobIPtr& mainKnob)
    {
        return KnobChoicePtr(new KnobChoice(holder, mainKnob));
    }

    virtual ~KnobChoice();

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    /**
     * @brief Fills-up the menu with the given entries and optionnally their tooltip.
     * @param entriesHelp Can either be empty, meaning no-tooltip or must be of the size of the entries.
     * @param mergingFunctor If not set, the internal menu will be completely reset and replaced with the given entries.
     * Otherwise the internal menu entries will be merged with the given entries according to this equality functor.
     * @param mergingData Can be passed when mergingFunctor is not null to speed up the comparisons.
     *
     * @returns true if something changed, false otherwise.
     **/

    bool populateChoices(const std::vector<ChoiceOption>& entries);

    /**
     * @brief Set optional shortcuts visible for menu entries. All items in the menu don't need a shortcut
     * so they are mapped against their index. The string corresponds to a shortcut ID that was registered
     * on the node during the getPluginShortcuts function on the same node.
     **/
    void setShortcuts(const std::map<int, std::string>& shortcuts);
    std::map<int, std::string> getShortcuts() const;

    /**
     * @brief Set optional icons for menu entries. All items in the menu don't need an icon
     * so they are mapped against their index.
     **/
    void setIcons(const std::map<int, std::string>& icons);
    std::map<int, std::string> getIcons() const;


    /**
     * @brief Set a list of separators. Each item in the list will add a separator after the index
     * specified by the integer.
     **/
    void setSeparators(const std::vector<int>& separators);
    std::vector<int> getSeparators() const;

    /**
     * @brief Clears the menu, the current index will no longer correspond to a valid entry
     **/
    void resetChoices(ViewSetSpec view = ViewSetSpec::all());


    /**
     * @brief Append an option to the menu 
     * @param help Optionnally specify the tooltip that should be displayed when the user hovers the entry in the menu
     **/
    void appendChoice( const ChoiceOption& option, ViewSetSpec view = ViewSetSpec::all() );

    /**
     * @brief Returns true if the entry for the given view is valid, that is: it still belongs to the menu entries.
     **/
    bool isActiveEntryPresentInEntries(ViewIdx view) const;

    /**
     * @brief Get all menu entries
     **/
    std::vector<ChoiceOption> getEntries(ViewIdx view = ViewIdx(0)) const;

    /**
     * @brief Get one menu entry. Throws an invalid_argument exception if index is invalid
     **/
    ChoiceOption getEntry(int v, ViewIdx view = ViewIdx(0)) const;

    /**
     * @brief Get the active entry text
     **/
    ChoiceOption getCurrentEntry(ViewIdx view = ViewIdx(0));
    ChoiceOption getCurrentEntryAtTime(TimeValue time, ViewIdx view = ViewIdx(0));

    /**
     * @brief Set the active entry text. If the view does not exist in the knob an invalid
     * argument exception is thrown
     **/
    void setActiveEntry(const ChoiceOption& entry, ViewSetSpec view = ViewSetSpec::all(), ValueChangedReasonEnum reason = eValueChangedReasonUserEdited);


    /**
     * @brief For a multi-choice parameter, return the selected options
     **/
    std::vector<ChoiceOption> getCurrentEntries_multi(ViewIdx view = ViewIdx(0));
    std::vector<ChoiceOption> getCurrentEntriesAtTime_multi(TimeValue time,ViewIdx view = ViewIdx(0));

    /**
    * @brief For a multi-choice parameter, set the selected options
    **/
    void setActiveEntries_multi(const std::vector<ChoiceOption>& entries, ViewSetSpec view = ViewSetSpec::all(), ValueChangedReasonEnum reason = eValueChangedReasonUserEdited);

    int getNumEntries(ViewIdx view = ViewIdx(0)) const;

    /// Can this type be animated?
    /// ChoiceParam animation may not be quite perfect yet,
    /// @see Curve::getValueAt() for the animation code.
    static bool canAnimateStatic()
    {
        return true;
    }

    static const std::string & typeNameStatic();
    std::string getHintToolTipFull() const;

    static int choiceMatch(const std::string& choice, const std::vector<ChoiceOption>& entries, ChoiceOption* matchedEntry);


    /**
     * @brief When set the menu will have a "New" entry which the user can select to create a new entry on its own.
     **/
    void setNewOptionCallback(ChoiceKnobDimView::KnobChoiceNewItemCallback callback);

    ChoiceKnobDimView::KnobChoiceNewItemCallback getNewOptionCallback() const;

    void setCascading(bool cascading);

    bool isCascading() const;

    /// set the KnobChoice value from the label
    ValueChangedReturnCodeEnum setValueFromID(const std::string & value, ViewSetSpec view = ViewSetSpec::all(), ValueChangedReasonEnum reason = eValueChangedReasonUserEdited);

    /// set the KnobChoice default value from the label
    void setDefaultValueFromID(const std::string & value);
    void setDefaultValueFromIDWithoutApplying(const std::string & value);

    /// set the KnobChoice default value from the label. Only usable for a multi-choice.
    void setDefaultValuesFromID_multi(const std::vector<std::string> & value);
    void setDefaultValuesFromIDWithoutApplying_multi(const std::vector<std::string> & value);

    void setMissingEntryWarningEnabled(bool enabled);
    bool isMissingEntryWarningEnabled() const;

    void setColorForIndex(int index, const RGBAColourD& color);
    bool getColorForIndex(int index, RGBAColourD* color) const;


    void setTextToFitHorizontally(const std::string& text);
    std::string getTextToFitHorizontally() const;

    virtual bool canLinkWith(const KnobIPtr & other, DimIdx thisDimension, ViewIdx thisView, DimIdx otherDim, ViewIdx otherView, std::string* error) const OVERRIDE WARN_UNUSED_RETURN;

    virtual void onLinkChanged() OVERRIDE FINAL;

    virtual bool hasDefaultValueChanged(DimIdx dimension) const OVERRIDE FINAL;

    virtual void setCurrentDefaultValueAsInitialValue() OVERRIDE FINAL;

    std::string getDefaultEntryID() const;
    std::vector<std::string> getDefaultEntriesID_multi() const;

    /**
     * @brief Enables multi-choice selection. Disabled by default.
     * When multi-choice is enabled, the underlying integer value held by the choice parameter has no real meaning.
     * The setValue/getValue A.P.I is thus forbidden in this mode. Instead, use the getCurrentEntries_multi and
     * setActiveEntries_multi functions.
     **/
    void setMultiChoiceEnabled(bool enabled);
    bool isMultiChoiceEnabled() const;

Q_SIGNALS:

    void populated();
    void entriesReset();
    void entryAppended();

private:

    virtual void restoreValueFromSerialization(const SERIALIZATION_NAMESPACE::ValueSerialization& obj,
                                               DimIdx targetDimension,
                                               ViewIdx view) OVERRIDE;

    void restoreChoiceValue(std::string* choiceID, ChoiceOption* entry, int* index);

    virtual void onDefaultValueChanged(DimSpec dimension) OVERRIDE FINAL;

    virtual bool hasModificationsVirtual(const KnobDimViewBasePtr& data, DimIdx dimension) const OVERRIDE FINAL;

    virtual CurveTypeEnum getKeyFrameDataType() const OVERRIDE FINAL
    {
        return eCurveTypeChoice;
    }

    void restoreChoiceAfterMenuChanged();

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

    virtual KnobDimViewBasePtr createDimViewData() const OVERRIDE;


private:

    static const std::string _typeNameStr;
    boost::scoped_ptr<KnobChoicePrivate> _imp;

};

inline KnobChoicePtr
toKnobChoice(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobChoice>(knob);
}


/******************************KnobSeparator**************************************/

class KnobSeparator
    : public KnobBoolBase
{
private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobSeparator(const KnobHolderPtr& holder,
                  const std::string &name,
                  int dimension);

    KnobSeparator(const KnobHolderPtr& holder, const KnobIPtr& mainInstance);
public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &name,
                                int dimension)
    {
        return KnobHelperPtr(new KnobSeparator(holder, name, dimension));
    }

    static KnobHelperPtr createRenderClone(const KnobHolderPtr& holder,
                                           const KnobIPtr& mainKnob)
    {
        return KnobSeparatorPtr(new KnobSeparator(holder, mainKnob));
    }

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return false;
    }

    static const std::string & typeNameStatic();
    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
};

inline KnobSeparatorPtr
toKnobSeparator(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobSeparator>(knob);
}



/******************************KnobKeyFrameMarkers**************************************/

/**
 * @brief A Knob that does not hold any real value but can have keyframes that are represented as markers on the timeline
 **/
class KnobKeyFrameMarkers
: public KnobStringBase
{
private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobKeyFrameMarkers(const KnobHolderPtr& holder,
                  const std::string &name,
                  int dimension);

    KnobKeyFrameMarkers(const KnobHolderPtr& holder, const KnobIPtr& mainInstance);
public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &name,
                                int dimension)
    {
        return KnobHelperPtr(new KnobKeyFrameMarkers(holder, name, dimension));
    }

    static KnobHelperPtr createRenderClone(const KnobHolderPtr& holder,
                                           const KnobIPtr& mainKnob)
    {
        return KnobKeyFrameMarkersPtr(new KnobKeyFrameMarkers(holder, mainKnob));
    }

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return true;
    }

    static const std::string & typeNameStatic();
    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return true;
    }

    virtual CurveTypeEnum getKeyFrameDataType() const OVERRIDE;

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
};

inline KnobKeyFrameMarkersPtr
toKnobKeyFrameMarkers(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobKeyFrameMarkers>(knob);
}


/******************************RGBA_KNOB**************************************/

/**
 * @brief A color knob with of variable dimension. Each color is a double ranging in [0. , 1.]
 * In dimension 1 the knob will have a single channel being a gray-scale
 * In dimension 3 the knob will have 3 channel R,G,B
 * In dimension 4 the knob will have R,G,B and A channels.
 **/
struct KnobColorPrivate;
class KnobColor
    :  public QObject, public KnobDoubleBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobColor(const KnobHolderPtr& holder,
              const std::string &name,
              int dimension);

    KnobColor(const KnobHolderPtr& holder, const KnobIPtr& mainInstance);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &name,
                                int dimension)
    {
        return KnobHelperPtr(new KnobColor(holder, name, dimension));
    }

    static KnobHelperPtr createRenderClone(const KnobHolderPtr& holder,
                                           const KnobIPtr& mainKnob)
    {
        return KnobColorPtr(new KnobColor(holder, mainKnob));
    }

    virtual ~KnobColor();

    static const std::string & typeNameStatic();

    void setPickingEnabled(ViewSetSpec view, bool enabled)
    {
        Q_EMIT pickingEnabled(view, enabled);
    }

    /**
     * @brief When simplified, the GUI of the knob should not have any spinbox and sliders but just a label to click and openup a color dialog
     **/
    void setSimplified(bool simp);
    bool isSimplified() const;

    void setUIColorspaceName(const std::string& csName);
    const std::string& getUIColorspaceName() const;

    void setInternalColorspaceName(const std::string& csName);
    const std::string& getInternalColorspaceName() const;

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return true;
    }


Q_SIGNALS:

    void pickingEnabled(ViewSetSpec,bool);

    void minMaxChanged(double mini, double maxi, int index = 0);

    void displayMinMaxChanged(double mini, double maxi, int index = 0);

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    static const std::string _typeNameStr;

    boost::shared_ptr<KnobColorPrivate> _imp;
};

inline KnobColorPtr
toKnobColor(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobColor>(knob);
}

/******************************KnobString**************************************/

struct KnobStringPrivate;
class KnobString
    : public KnobStringBase
{
private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobString(const KnobHolderPtr& holder,
               const std::string &name,
               int dimension);

    KnobString(const KnobHolderPtr& holder, const KnobIPtr& mainInstance);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &name,
                                int dimension)
    {
        return KnobHelperPtr(new KnobString(holder, name, dimension));
    }

    static KnobHelperPtr createRenderClone(const KnobHolderPtr& holder,
                                           const KnobIPtr& mainKnob)
    {
        return KnobStringPtr(new KnobString(holder, mainKnob));
    }

    virtual ~KnobString();

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    /// Can this type be animated?
    /// String animation consists in setting constant strings at
    /// each keyframe, which are valid until the next keyframe.
    /// It can be useful for titling/subtitling.
    static bool canAnimateStatic()
    {
        return true;
    }

    static const std::string & typeNameStatic();

    void setAsMultiLine();

    void setUsesRichText(bool useRichText);

    bool isMultiLine() const;

    bool usesRichText() const;

    void setAsCustomHTMLText(bool custom);

    bool isCustomHTMLText() const;

    void setAsLabel();

    bool isLabel() const;

    void setAsCustom();

    bool isCustomKnob() const;

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    int getFontSize() const;

    void setFontSize(int size);

    std::string getFontFamily() const;

    void setFontFamily(const std::string& family);

    void getFontColor(double* r, double* g, double* b) const;

    void setFontColor(double r, double g, double b);

    bool getItalicActivated() const;

    void setItalicActivated(bool b) ;

    bool getBoldActivated() const;

    void setBoldActivated(bool b) ;

    /**
     * @brief Relevant for multi-lines with rich text enables. It tells if
     * the string has content without the html tags
     **/
    bool hasContentWithoutHtmlTags();

    static int getDefaultFontPointSize();

    static bool parseFont(const QString & s, int *fontSize, QString* fontFamily, bool* isBold, bool* isItalic, double* r, double *g, double* b);

    /**
     * @brief Make a html friendly font tag from font properties
     **/
    static QString makeFontTag(const QString& family, int fontSize, double r, double g, double b);

    /**
     * @brief Surround the given text by the given font tag
     **/
    static QString decorateTextWithFontTag(const QString& family, int fontSize, double r, double g, double b, bool isBold, bool isItalic, const QString& text);

    /**
     * @brief Remove any custom Natron html tag content from the given text and returned a stripped version of it.
     **/
    static QString removeNatronHtmlTag(QString text);

    /**
     * @brief Get the content in between custom Natron html tags if any
     **/
    static QString getNatronHtmlTagContent(QString text);

    /**
     * @brief The goal here is to remove all the tags added automatically by Natron (like font color,size family etc...)
     * so the user does not see them in the user interface. Those tags are  present in the internal value held by the knob.
     **/
    static QString removeAutoAddedHtmlTags(QString text, bool removeNatronTag = true);

    QString decorateStringWithCurrentState(const QString& str);

    /**
     * @brief Same as getValue() but decorates the string with the current font state. Only useful if rich text has been enabled
     **/
    QString getValueDecorated(TimeValue time, ViewIdx view);

private:

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;


    boost::shared_ptr<KnobStringPrivate> _imp;
};

inline KnobStringPtr
toKnobString(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobString>(knob);
}

/******************************KnobGroup**************************************/

struct KnobGroupPrivate;
class KnobGroup
    :  public QObject, public KnobBoolBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    boost::shared_ptr<KnobGroupPrivate> _imp;

private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobGroup(const KnobHolderPtr& holder,
              const std::string &name,
              int dimension);

    KnobGroup(const KnobHolderPtr& holder, const KnobIPtr& mainInstance);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &name,
                                int dimension)
    {
        return KnobHelperPtr(new KnobGroup(holder, name, dimension));
    }

    static KnobHelperPtr createRenderClone(const KnobHolderPtr& holder,
                                           const KnobIPtr& mainKnob)
    {
        return KnobGroupPtr(new KnobGroup(holder, mainKnob));
    }

    virtual ~KnobGroup();

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return false;
    }

    void addKnob(const KnobIPtr& k);
    void removeKnob(const KnobIPtr& k);

    bool moveOneStepUp(const KnobIPtr& k);
    bool moveOneStepDown(const KnobIPtr& k);

    void insertKnob(int index, const KnobIPtr& k);

    std::vector<KnobIPtr> getChildren() const;

    void setAsTab();

    bool isTab() const;

    void setAsToolButton(bool b);
    bool getIsToolButton() const;

    void setAsDialog(bool b);
    bool getIsDialog() const;

    static const std::string & typeNameStatic();

private:

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    static const std::string _typeNameStr;
};

inline KnobGroupPtr
toKnobGroup(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobGroup>(knob);
}

/******************************PAGE_KNOB**************************************/

struct KnobPagePrivate;
class KnobPage
    :  public QObject, public KnobBoolBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobPage(const KnobHolderPtr& holder,
             const std::string &name,
             int dimension);

    KnobPage(const KnobHolderPtr& holder, const KnobIPtr& mainInstance);

public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &name,
                                int dimension)
    {
        return KnobHelperPtr(new KnobPage(holder, name, dimension));
    }

    static KnobHelperPtr createRenderClone(const KnobHolderPtr& holder,
                                           const KnobIPtr& mainKnob)
    {
        return KnobPagePtr(new KnobPage(holder, mainKnob));
    }

    virtual ~KnobPage();

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return false;
    }

    void addKnob(const KnobIPtr& k);

    void setAsToolBar(bool b);

    bool getIsToolBar() const;

    bool moveOneStepUp(const KnobIPtr& k);
    bool moveOneStepDown(const KnobIPtr& k);

    void removeKnob(const KnobIPtr& k);

    void insertKnob(int index, const KnobIPtr& k);

    std::vector<KnobIPtr> getChildren() const;
    static const std::string & typeNameStatic();

private:
    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    static const std::string _typeNameStr;
    boost::shared_ptr<KnobPagePrivate> _imp;
};

inline KnobPagePtr
toKnobPage(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobPage>(knob);
}

/******************************KnobParametric**************************************/


class ParametricKnobDimView : public ValueKnobDimView<double>
{
public:

    CurvePtr parametricCurve;

    ParametricKnobDimView()
    : parametricCurve()
    {

    }

    virtual ~ParametricKnobDimView()
    {
        
    }

    virtual bool copy(const CopyInArgs& inArgs, CopyOutArgs* outArgs) OVERRIDE;
};

typedef boost::shared_ptr<ParametricKnobDimView> ParametricKnobDimViewPtr;

inline ParametricKnobDimViewPtr
toParametricKnobDimView(const KnobDimViewBasePtr& data)
{
    return boost::dynamic_pointer_cast<ParametricKnobDimView>(data);
}

struct KnobParametricPrivate;
class KnobParametric
    :  public QObject, public KnobDoubleBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    
    boost::scoped_ptr<KnobParametricPrivate> _imp;

private: // derives from KnobI
    KnobParametric(const KnobHolderPtr& holder,
                   const std::string &name,
                   int dimension);

    KnobParametric(const KnobHolderPtr& holder, const KnobIPtr& mainInstance);

    virtual void populate() OVERRIDE FINAL;

public:

    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &name,
                                int nDims)
    {
        return KnobHelperPtr(new KnobParametric(holder, name, nDims));
    }

    static KnobHelperPtr createRenderClone(const KnobHolderPtr& holder,
                                           const KnobIPtr& mainKnob)
    {
        return KnobParametricPtr(new KnobParametric(holder, mainKnob));
    }

    virtual ~KnobParametric();

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return false;
    }

    // Don't allow other knobs to slave to this one
    virtual bool canLinkWith(const KnobIPtr & /*other*/, DimIdx /*thisDimension*/, ViewIdx /*thisView*/,  DimIdx /*otherDim*/, ViewIdx /*otherView*/, std::string* /*error*/) const OVERRIDE FINAL;

    virtual void onLinkChanged() OVERRIDE FINAL;

    void setCurveColor(DimIdx dimension, double r, double g, double b);

    void setPeriodic(bool periodic);

    void getCurveColor(DimIdx dimension, double* r, double* g, double* b);

    void setParametricRange(double min, double max);

    void setDefaultCurvesFromCurves();

    std::pair<double, double> getParametricRange() const WARN_UNUSED_RETURN;
    CurvePtr getParametricCurve(DimIdx dimension, ViewIdx view) const;
    CurvePtr getDefaultParametricCurve(DimIdx dimension) const;
    ActionRetCodeEnum addControlPoint(ValueChangedReasonEnum reason, DimIdx dimension, double key, double value, KeyframeTypeEnum interpolation = eKeyframeTypeSmooth) WARN_UNUSED_RETURN;
    ActionRetCodeEnum addControlPoint(ValueChangedReasonEnum reason, DimIdx dimension, double key, double value, double leftDerivative, double rightDerivative, KeyframeTypeEnum interpolation = eKeyframeTypeSmooth) WARN_UNUSED_RETURN;
    ActionRetCodeEnum evaluateCurve(DimIdx dimension, ViewIdx view, double parametricPosition, double *returnValue) const WARN_UNUSED_RETURN;
    ActionRetCodeEnum getNControlPoints(DimIdx dimension, ViewIdx view, int *returnValue) const WARN_UNUSED_RETURN;
    ActionRetCodeEnum getNthControlPoint(DimIdx dimension,
                                  ViewIdx view,
                                  int nthCtl,
                                  double *key,
                                  double *value) const WARN_UNUSED_RETURN;
    ActionRetCodeEnum getNthControlPoint(DimIdx dimension,
                                  ViewIdx view,
                                  int nthCtl,
                                  double *key,
                                  double *value,
                                  double *leftDerivative,
                                  double *rightDerivative) const WARN_UNUSED_RETURN;

    ActionRetCodeEnum setNthControlPointInterpolation(ValueChangedReasonEnum reason,
                                               DimIdx dimension,
                                               ViewSetSpec view,
                                               int nThCtl,
                                               KeyframeTypeEnum interpolation) WARN_UNUSED_RETURN;

    ActionRetCodeEnum setNthControlPoint(ValueChangedReasonEnum reason,
                                  DimIdx dimension,
                                  ViewSetSpec view,
                                  int nthCtl,
                                  double key,
                                  double value) WARN_UNUSED_RETURN;

    ActionRetCodeEnum setNthControlPoint(ValueChangedReasonEnum reason,
                                  DimIdx dimension,
                                  ViewSetSpec view,
                                  int nthCtl,
                                  double key,
                                  double value,
                                  double leftDerivative,
                                  double rightDerivative) WARN_UNUSED_RETURN;


    ActionRetCodeEnum deleteControlPoint(ValueChangedReasonEnum reason, DimIdx dimension, ViewSetSpec view, int nthCtl) WARN_UNUSED_RETURN;
    ActionRetCodeEnum deleteAllControlPoints(ValueChangedReasonEnum reason, DimIdx dimension, ViewSetSpec view) WARN_UNUSED_RETURN;
    static const std::string & typeNameStatic() WARN_UNUSED_RETURN;

    void saveParametricCurves(std::map<std::string,std::list<SERIALIZATION_NAMESPACE::CurveSerialization > >* curves) const;

    void loadParametricCurves(const std::map<std::string,std::list<SERIALIZATION_NAMESPACE::CurveSerialization > >& curves);

    virtual void appendToHash(const ComputeHashArgs& args, Hash64* hash) OVERRIDE FINAL;

    virtual void clearRenderValuesCache() OVERRIDE FINAL;

    //////////// Overriden from AnimatingObjectI
    virtual CurvePtr getAnimationCurve(ViewIdx idx, DimIdx dimension) const OVERRIDE FINAL;
    virtual bool cloneCurve(ViewIdx view, DimIdx dimension, const Curve& curve, double offset, const RangeD* range) OVERRIDE;
    virtual void deleteValuesAtTime(const std::list<double>& times, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason) OVERRIDE;
    virtual bool warpValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* keyframes = 0) OVERRIDE ;
    virtual void removeAnimation(ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason) OVERRIDE ;
    virtual void deleteAnimationBeforeTime(TimeValue time, ViewSetSpec view, DimSpec dimension) OVERRIDE ;
    virtual void deleteAnimationAfterTime(TimeValue time, ViewSetSpec view, DimSpec dimension) OVERRIDE ;
    virtual void setInterpolationAtTimes(ViewSetSpec view, DimSpec dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys = 0) OVERRIDE ;
    virtual bool setLeftAndRightDerivativesAtTime(ViewSetSpec view, DimSpec dimension, TimeValue time, double left, double right)  OVERRIDE WARN_UNUSED_RETURN;
    virtual bool setDerivativeAtTime(ViewSetSpec view, DimSpec dimension, TimeValue time, double derivative, bool isLeft) OVERRIDE WARN_UNUSED_RETURN;

    //////////// End from AnimatingObjectI

Q_SIGNALS:


    ///emitted when the state of a curve changed at the indicated dimension
    void curveChanged(DimSpec);

    void curveColorChanged(DimSpec);

private:

    CurvePtr getParametricCurveInternal(DimIdx dimension, ViewIdx view, ParametricKnobDimViewPtr* data) const;

    void signalCurveChanged(DimSpec dimension, const KnobDimViewBasePtr& data);

    virtual KnobDimViewBasePtr createDimViewData() const OVERRIDE;

    ValueChangedReturnCodeEnum setKeyFrameInternal(TimeValue time,
                                                   double value,
                                                   DimIdx dimension,
                                                   ViewIdx view,
                                                   KeyFrame* newKey);

    virtual void resetExtraToDefaultValue(DimSpec dimension, ViewSetSpec view) OVERRIDE FINAL;
    virtual bool hasModificationsVirtual(const KnobDimViewBasePtr& data, DimIdx dimension) const OVERRIDE FINAL;
    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

    static const std::string _typeNameStr;
};

inline KnobParametricPtr
toKnobParametric(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobParametric>(knob);
}

/**
 * @brief A Table containing strings. The number of columns is static.
 **/
class KnobTable
    : public QObject, public KnobStringBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

protected: // derives from KnobI, parent of KnobLayer, KnobPath
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobTable(const KnobHolderPtr& holder,
              const std::string &name,
              int dimension);


    KnobTable(const KnobHolderPtr& holder,
              const KnobIPtr& mainInstance);
    
public:
    virtual ~KnobTable();

    virtual bool isAnimatedByDefault() const OVERRIDE FINAL
    {
        return false;
    }

    virtual bool canSplitViews() const OVERRIDE FINAL
    {
        return false;
    }

    void getTable(std::list<std::vector<std::string> >* table);
    void getTableSingleCol(std::list<std::string>* table);

    void decodeFromKnobTableFormat(const std::string& value, std::list<std::vector<std::string> >* table);
    std::string encodeToKnobTableFormat(const std::list<std::vector<std::string> >& table);
    std::string encodeToKnobTableFormatSingleCol(const std::list<std::string>& table);

    void setTable(const std::list<std::vector<std::string> >& table);
    void setTableSingleCol(const std::list<std::string>& table);
    void appendRow(const std::vector<std::string>& row);
    void appendRowSingleCol(const std::string& row);
    void insertRow(int index, const std::vector<std::string>& row);
    void insertRowSingleCol(int index, const std::string& row);
    void removeRow(int index);

    virtual int getColumnsCount() const = 0;
    virtual std::string getColumnLabel(int col) const = 0;
    virtual bool isCellEnabled(int row, int col, const QStringList& values) const = 0;
    virtual bool isCellBracketDecorated(int /*row*/,
                                        int /*col*/) const
    {
        return false;
    }

    virtual bool isColumnEditable(int /*col*/)
    {
        return true;
    }

    virtual bool useEditButton() const
    {
        return true;
    }

private:


    virtual bool canAnimate() const OVERRIDE FINAL
    {
        return false;
    }
};

class KnobLayers
    : public KnobTable
{
    Q_DECLARE_TR_FUNCTIONS(KnobLayers)

private: // derives from KnobI
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    KnobLayers(const KnobHolderPtr& holder,
               const std::string &name,
               int dimension)
        : KnobTable(holder, name, dimension)
    {
    }

    KnobLayers(const KnobHolderPtr& holder,
              const KnobIPtr& mainInstance)
    : KnobTable(holder, mainInstance)
    {

    }



public:
    static KnobHelperPtr create(const KnobHolderPtr& holder,
                                const std::string &name,
                                int dimension)
    {
        return KnobHelperPtr(new KnobLayers(holder, name, dimension));
    }


    static KnobHelperPtr createRenderClone(const KnobHolderPtr& holder,
                                            const KnobIPtr& mainInstance)
    {
        return KnobHelperPtr(new KnobLayers(holder, mainInstance));
    }
    

    virtual ~KnobLayers()
    {
    }


    virtual int getColumnsCount() const OVERRIDE FINAL
    {
        return 3;
    }

    virtual std::string getColumnLabel(int col) const OVERRIDE FINAL
    {
        if (col == 0) {
            return tr("Name").toStdString();
        } else if (col == 1) {
            return tr("Channels").toStdString();
        } else if (col == 2) {
            return tr("Components Type").toStdString();
        } else {
            return std::string();
        }
    }

    virtual bool isCellEnabled(int /*row*/,
                               int /*col*/,
                               const QStringList& /*values*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool isColumnEditable(int col) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        if (col == 1) {
            return false;
        }

        return true;
    }

    static const std::string & typeNameStatic() WARN_UNUSED_RETURN;

    std::string encodePlanesList(const std::list<ImagePlaneDesc>& planes);
    std::list<ImagePlaneDesc> decodePlanesList();

private:

    virtual const std::string & typeName() const OVERRIDE FINAL
    {
        return typeNameStatic();
    }

    static const std::string _typeNameStr;
};


inline KnobTablePtr
toKnobTable(const KnobIPtr& knob)
{
    return boost::dynamic_pointer_cast<KnobTable>(knob);
}


NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_KNOBTYPES_H
