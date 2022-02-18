/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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
GCC_ONLY_DIAG_OFF(class-memaccess)
#include <QtCore/QVector>
GCC_ONLY_DIAG_ON(class-memaccess)
#include <QtCore/QMutex>
#include <QtCore/QString>

#include "Global/GlobalDefines.h"
#include "Engine/Knob.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"
#include "Engine/ChoiceOption.h"

#define kFontSizeTag "<font size=\""
#define kFontColorTag "color=\""
#define kFontFaceTag "face=\""
#define kFontEndTag "</font>"
#define kBoldStartTag "<b>"
#define kBoldEndTag "</b>"
#define kItalicStartTag "<i>"
#define kItalicEndTag "</i>"

NATRON_NAMESPACE_ENTER

/******************************KnobInt**************************************/

class KnobInt
    : public QObject, public KnobIntBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobInt(holder, label, dimension, declaredByPlugin);
    }

    KnobInt(KnobHolder* holder,
            const std::string &label,
            int dimension,
            bool declaredByPlugin);

    KnobInt(KnobHolder* holder,
            const QString &label,
            int dimension,
            bool declaredByPlugin);

    void disableSlider();

    bool isSliderDisabled() const;

    bool isAutoFoldDimensionsEnabled() const { return false; };

    static const std::string & typeNameStatic();

    void setAsRectangle()
    {
        if (getDimension() == 4) {
            _isRectangle = true;
            disableSlider();
        }
    }

    bool isRectangle() const
    {
        return _isRectangle;
    }

public:

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    void setIncrement(int incr, int index = 0);

    void setIncrement(const std::vector<int> &incr);

    const std::vector<int> &getIncrements() const;


Q_SIGNALS:


    void incrementChanged(double incr, int index = 0);

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    std::vector<int> _increments;
    bool _disableSlider;
    bool _isRectangle;
    static const std::string _typeNameStr;
};

/******************************KnobBool**************************************/

class KnobBool
    :  public KnobBoolBase
{
public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobBool(holder, label, dimension, declaredByPlugin);
    }

    KnobBool(KnobHolder* holder,
             const std::string &label,
             int dimension,
             bool declaredByPlugin);

    KnobBool(KnobHolder* holder,
             const QString &label,
             int dimension,
             bool declaredByPlugin);

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

/******************************KnobDouble**************************************/

class KnobDouble
    :  public QObject, public KnobDoubleBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobDouble(holder, label, dimension, declaredByPlugin);
    }

    KnobDouble(KnobHolder* holder,
               const std::string &label,
               int dimension,
               bool declaredByPlugin );

    KnobDouble(KnobHolder* holder,
               const QString &label,
               int dimension,
               bool declaredByPlugin );

    virtual ~KnobDouble();

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    void disableSlider();

    bool isSliderDisabled() const;

    void setCanAutoFoldDimensions(bool v) { _autoFoldEnabled = v; }

    bool isAutoFoldDimensionsEnabled() const { return _autoFoldEnabled; };

    const std::vector<double> &getIncrements() const;
    const std::vector<int> &getDecimals() const;

    void setIncrement(double incr, int index = 0);

    void setDecimals(int decis, int index = 0);

    void setIncrement(const std::vector<double> &incr);

    void setDecimals(const std::vector<int> &decis);

    static const std::string & typeNameStatic();

    ValueIsNormalizedEnum getValueIsNormalized(int dimension) const
    {
        return _valueIsNormalized[dimension];
    }

    void setValueIsNormalized(int dimension,
                              ValueIsNormalizedEnum state)
    {
        _valueIsNormalized[dimension] = state;
    }

    void setSpatial(bool spatial)
    {
        _spatial = spatial;
    }

    bool getIsSpatial() const
    {
        return _spatial;
    }

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
    void setDefaultValuesAreNormalized(bool normalized)
    {
        _defaultValuesAreNormalized = normalized;
    }

    /**
     * @brief Returns whether the default values are stored normalized or not.
     **/
    bool getDefaultValuesAreNormalized() const
    {
        return _defaultValuesAreNormalized;
    }

    /**
     * @brief Denormalize the given value according to the RoD of the attached effect's input's RoD.
     * WARNING: Can only be called once setValueIsNormalized has been called!
     **/
    double denormalize(int dimension, double time, double value) const;

    /**
     * @brief Normalize the given value according to the RoD of the attached effect's input's RoD.
     * WARNING: Can only be called once setValueIsNormalized has been called!
     **/
    double normalize(int dimension, double time, double value) const;

    void setHasHostOverlayHandle(bool handle);

    bool getHasHostOverlayHandle() const;

    virtual bool useHostOverlayHandle() const OVERRIDE { return getHasHostOverlayHandle(); }

    void setAsRectangle()
    {
        if (getDimension() == 4) {
            _isRectangle = true;
        }
    }

    bool isRectangle() const
    {
        return _isRectangle;
    }

Q_SIGNALS:

    void incrementChanged(double incr, int index = 0);

    void decimalsChanged(int deci, int index = 0);

private:

    virtual bool computeValuesHaveModifications(int dimension,
                                                const double& value,
                                                const double& defaultValue) const OVERRIDE FINAL;
    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    bool _spatial;
    bool _isRectangle;
    std::vector<double>  _increments;
    std::vector<int> _decimals;
    bool _disableSlider;
    bool _autoFoldEnabled;

    /// to support ofx deprecated normalizd params:
    /// the first and second dimensions of the double param( hence a pair ) have a normalized state.
    /// BY default they have eValueIsNormalizedNone
    /// if the double type is one of
    /// - kOfxParamDoubleTypeNormalisedX - normalised size wrt to the project's X dimension (1D only),
    /// - kOfxParamDoubleTypeNormalisedXAbsolute - normalised absolute position on the X axis (1D only)
    /// - kOfxParamDoubleTypeNormalisedY - normalised size wrt to the project's Y dimension(1D only),
    /// - kOfxParamDoubleTypeNormalisedYAbsolute - normalised absolute position on the Y axis (1D only)
    /// - kOfxParamDoubleTypeNormalisedXY - normalised to the project's X and Y size (2D only),
    /// - kOfxParamDoubleTypeNormalisedXYAbsolute - normalised to the projects X and Y size, and is an absolute position on the image plane,
    std::vector<ValueIsNormalizedEnum> _valueIsNormalized;

    ///For double params respecting the kOfxParamCoordinatesNormalised
    ///This tells us that only the default value is stored normalized.
    ///This SHOULD NOT bet set for old deprecated < OpenFX 1.2 normalized parameters.
    bool _defaultValuesAreNormalized;
    bool _hasHostOverlayHandle;
    static const std::string _typeNameStr;
};

/******************************KnobButton**************************************/

class KnobButton
    : public KnobBoolBase
{
public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobButton(holder, label, dimension, declaredByPlugin);
    }

    KnobButton(KnobHolder* holder,
               const std::string &label,
               int dimension,
               bool declaredByPlugin);

    KnobButton(KnobHolder* holder,
               const QString &label,
               int dimension,
               bool declaredByPlugin);
    static const std::string & typeNameStatic();

    void setAsRenderButton()
    {
        _renderButton = true;
    }

    bool isRenderButton() const
    {
        return _renderButton;
    }

    // Trigger the knobChanged handler for this knob
    // Returns true if the knobChanged handler was caught and an action was done
    bool trigger();

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    void setCheckable(bool b)
    {
        _checkable = b;
    }

    bool getIsCheckable() const
    {
        return _checkable;
    }

    void setAsToolButtonAction(bool b)
    {
        _isToolButtonAction = b;
    }

    bool getIsToolButtonAction() const
    {
        return _isToolButtonAction;
    }

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
    bool _renderButton;
    bool _checkable;
    bool _isToolButtonAction;
};

/******************************KnobChoice**************************************/

class KnobChoice
    : public QObject, public KnobIntBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobChoice(holder, label, dimension, declaredByPlugin);
    }

    KnobChoice(KnobHolder* holder,
               const std::string &label,
               int dimension,
               bool declaredByPlugin);

    KnobChoice(KnobHolder* holder,
               const QString &label,
               int dimension,
               bool declaredByPlugin);

    virtual ~KnobChoice();

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
    bool populateChoices(const std::vector<ChoiceOption> &entries);

    void resetChoices();

    void appendChoice( const ChoiceOption& entry);

    bool isActiveEntryPresentInEntries() const;

    std::vector<ChoiceOption> getEntries_mt_safe() const;
    ChoiceOption getEntry(int v) const;
    ChoiceOption getActiveEntry();
    void setActiveEntry(const ChoiceOption& opt);

    int getNumEntries() const;

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
    
    int choiceRestorationId(KnobChoice* knob, const std::string& optionID);
    void choiceRestoration(KnobChoice* knob, const std::string& optionID, int id);

    /**
     * @brief When set the menu will have a "New" entry which the user can select to create a new entry on its own.
     **/
    void setHostCanAddOptions(bool add);

    bool getHostCanAddOptions() const;

    void setCascading(bool cascading)
    {
        _isCascading = cascading;
    }

    bool isCascading() const
    {
        return _isCascading;
    }

    /// set the KnobChoice value from the label
    ValueChangedReturnCodeEnum setValueFromID(const std::string & value,
                                                 int dimension,
                                                 bool turnOffAutoKeying = false);

    /// set the KnobChoice default value from the label
    void setDefaultValueFromID(const std::string & value, int dimension = 0);
    void setDefaultValueFromIDWithoutApplying(const std::string & value, int dimension = 0);

public Q_SLOTS:

    void onOriginalKnobPopulated();
    void onOriginalKnobEntriesReset();
    void onOriginalKnobEntryAppend();

Q_SIGNALS:

    void populated();
    void entriesReset();
    void entryAppended();

private:

    virtual void onKnobAboutToAlias(const KnobIPtr& slave) OVERRIDE FINAL;

    void findAndSetOldChoice();

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;
    virtual void handleSignalSlotsForAliasLink(const KnobIPtr& alias, bool connect) OVERRIDE FINAL;
    virtual void onInternalValueChanged(int dimension, double time, ViewSpec view) OVERRIDE FINAL;
    virtual void cloneExtraData(KnobI* other, int dimension = -1, int otherDimension = -1) OVERRIDE FINAL;
    virtual bool cloneExtraDataAndCheckIfChanged(KnobI* other, int dimension = -1, int otherDimension = -1) OVERRIDE FINAL;
    virtual void cloneExtraData(KnobI* other, double offset, const RangeD* range, int dimension = -1, int otherDimension = -1) OVERRIDE FINAL;

private:

    mutable QMutex _entriesMutex;
    std::vector<ChoiceOption> _entries;
    ChoiceOption _currentEntry; // protected by _entriesMutex
    bool _addNewChoice;
    static const std::string _typeNameStr;
    bool _isCascading;
};

/******************************KnobSeparator**************************************/

class KnobSeparator
    : public KnobBoolBase
{
public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobSeparator(holder, label, dimension, declaredByPlugin);
    }

    KnobSeparator(KnobHolder* holder,
                  const std::string &label,
                  int dimension,
                  bool declaredByPlugin);

    KnobSeparator(KnobHolder* holder,
                  const QString &label,
                  int dimension,
                  bool declaredByPlugin);
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

/******************************RGBA_KNOB**************************************/

/**
 * @brief A color knob with of variable dimension. Each color is a double ranging in [0. , 1.]
 * In dimension 1 the knob will have a single channel being a gray-scale
 * In dimension 3 the knob will have 3 channel R,G,B
 * In dimension 4 the knob will have R,G,B and A channels.
 **/
class KnobColor
    :  public QObject, public KnobDoubleBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobColor(holder, label, dimension, declaredByPlugin);
    }

    KnobColor(KnobHolder* holder,
              const std::string &label,
              int dimension,
              bool declaredByPlugin);

    KnobColor(KnobHolder* holder,
              const QString &label,
              int dimension,
              bool declaredByPlugin);
    static const std::string & typeNameStatic();

    bool areAllDimensionsEnabled() const;

    void activateAllDimensions()
    {
        Q_EMIT mustActivateAllDimensions();
    }

    void setPickingEnabled(bool enabled)
    {
        Q_EMIT pickingEnabled(enabled);
    }

    /**
     * @brief When simplified, the GUI of the knob should not have any spinbox and sliders but just a label to click and openup a color dialog
     **/
    void setSimplified(bool simp);
    bool isSimplified() const;

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

public Q_SLOTS:

    void onDimensionSwitchToggled(bool b);

Q_SIGNALS:

    void pickingEnabled(bool);

    void minMaxChanged(double mini, double maxi, int index = 0);

    void displayMinMaxChanged(double mini, double maxi, int index = 0);

    void mustActivateAllDimensions();

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    bool _allDimensionsEnabled;
    bool _simplifiedMode;
    static const std::string _typeNameStr;
};

/******************************KnobString**************************************/


class KnobString
    : public AnimatingKnobStringHelper
{
public:


    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobString(holder, label, dimension, declaredByPlugin);
    }

    KnobString(KnobHolder* holder,
               const std::string &label,
               int dimension,
               bool declaredByPlugin);

    KnobString(KnobHolder* holder,
               const QString &label,
               int dimension,
               bool declaredByPlugin);

    virtual ~KnobString();

    /// Can this type be animated?
    /// String animation consists in setting constant strings at
    /// each keyframe, which are valid until the next keyframe.
    /// It can be useful for titling/subtitling.
    static bool canAnimateStatic()
    {
        return true;
    }

    static const std::string & typeNameStatic();

    void setAsMultiLine()
    {
        _multiLine = true;
    }

    void setUsesRichText(bool useRichText)
    {
        _richText = useRichText;
    }

    bool isMultiLine() const
    {
        return _multiLine;
    }

    bool usesRichText() const
    {
        return _richText;
    }

    void setAsCustomHTMLText(bool custom)
    {
        _customHtmlText = custom;
    }

    bool isCustomHTMLText() const
    {
        return _customHtmlText;
    }

    void setAsLabel();

    bool isLabel() const
    {
        return _isLabel;
    }

    void setAsCustom()
    {
        _isCustom = true;
    }

    bool isCustomKnob() const
    {
        return _isCustom;
    }

    virtual bool supportsInViewerContext() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return !_multiLine;
    }

    /**
     * @brief Relevant for multi-lines with rich text enables. It tells if
     * the string has content without the html tags
     **/
    bool hasContentWithoutHtmlTags();

private:

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
    bool _multiLine;
    bool _richText;
    bool _customHtmlText;
    bool _isLabel;
    bool _isCustom;
};

/******************************KnobGroup**************************************/
class KnobGroup
    :  public QObject, public KnobBoolBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    std::vector<KnobIWPtr> _children;
    bool _isTab;
    bool _isToolButton;
    bool _isDialog;

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobGroup(holder, label, dimension, declaredByPlugin);
    }

    KnobGroup(KnobHolder* holder,
              const std::string &label,
              int dimension,
              bool declaredByPlugin);

    KnobGroup(KnobHolder* holder,
              const QString &label,
              int dimension,
              bool declaredByPlugin);

    void addKnob(const KnobIPtr& k);
    void removeKnob(KnobI* k);

    bool moveOneStepUp(KnobI* k);
    bool moveOneStepDown(KnobI* k);

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


/******************************PAGE_KNOB**************************************/

class KnobPage
    :  public QObject, public KnobBoolBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobPage(holder, label, dimension, declaredByPlugin);
    }

    KnobPage(KnobHolder* holder,
             const std::string &label,
             int dimension,
             bool declaredByPlugin);

    KnobPage(KnobHolder* holder,
             const QString &label,
             int dimension,
             bool declaredByPlugin);

    void addKnob(const KnobIPtr& k);

    void setAsToolBar(bool b)
    {
        _isToolBar = b;
    }

    bool getIsToolBar() const
    {
        return _isToolBar;
    }

    bool moveOneStepUp(KnobI* k);
    bool moveOneStepDown(KnobI* k);

    void removeKnob(KnobI* k);

    void insertKnob(int index, const KnobIPtr& k);

    std::vector<KnobIPtr>  getChildren() const;
    static const std::string & typeNameStatic();

private:
    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    bool _isToolBar;
    std::vector<KnobIWPtr> _children;
    static const std::string _typeNameStr;
};


/******************************KnobParametric**************************************/

class KnobParametric
    :  public QObject, public KnobDoubleBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    mutable QMutex _curvesMutex;
    std::vector<CurvePtr> _curves, _defaultCurves;
    std::vector<RGBAColourD> _curvesColor;
public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobParametric(holder, label, dimension, declaredByPlugin);
    }

    KnobParametric(KnobHolder* holder,
                   const std::string &label,
                   int dimension,
                   bool declaredByPlugin );

    KnobParametric(KnobHolder* holder,
                   const QString &label,
                   int dimension,
                   bool declaredByPlugin );

    void setPeriodic(bool periodic);
    void setCurveColor(int dimension, double r, double g, double b);

    void getCurveColor(int dimension, double* r, double* g, double* b);

    void setParametricRange(double min, double max);

    void setDefaultCurvesFromCurves();

    std::pair<double, double> getParametricRange() const WARN_UNUSED_RETURN;
    CurvePtr getParametricCurve(int dimension) const;
    CurvePtr getDefaultParametricCurve(int dimension) const;
    StatusEnum addControlPoint(ValueChangedReasonEnum reason, int dimension, double key, double value, KeyframeTypeEnum interpolation = eKeyframeTypeSmooth) WARN_UNUSED_RETURN;
    StatusEnum addControlPoint(ValueChangedReasonEnum reason, int dimension, double key, double value, double leftDerivative, double rightDerivative, KeyframeTypeEnum interpolation = eKeyframeTypeSmooth) WARN_UNUSED_RETURN;
    StatusEnum getValue(int dimension, double parametricPosition, double *returnValue) const WARN_UNUSED_RETURN;
    StatusEnum getNControlPoints(int dimension, int *returnValue) const WARN_UNUSED_RETURN;
    StatusEnum getNthControlPoint(int dimension,
                                  int nthCtl,
                                  double *key,
                                  double *value) const WARN_UNUSED_RETURN;
    StatusEnum getNthControlPoint(int dimension,
                                  int nthCtl,
                                  double *key,
                                  double *value,
                                  double *leftDerivative,
                                  double *rightDerivative) const WARN_UNUSED_RETURN;

    StatusEnum setNthControlPointInterpolation(ValueChangedReasonEnum reason,
                                               int dimension,
                                               int nThCtl,
                                               KeyframeTypeEnum interpolation) WARN_UNUSED_RETURN;

    StatusEnum setNthControlPoint(ValueChangedReasonEnum reason,
                                  int dimension,
                                  int nthCtl,
                                  double key,
                                  double value) WARN_UNUSED_RETURN;

    StatusEnum setNthControlPoint(ValueChangedReasonEnum reason,
                                  int dimension,
                                  int nthCtl,
                                  double key,
                                  double value,
                                  double leftDerivative,
                                  double rightDerivative) WARN_UNUSED_RETURN;


    StatusEnum deleteControlPoint(ValueChangedReasonEnum reason, int dimension, int nthCtl) WARN_UNUSED_RETURN;
    StatusEnum deleteAllControlPoints(ValueChangedReasonEnum reason, int dimension) WARN_UNUSED_RETURN;
    static const std::string & typeNameStatic() WARN_UNUSED_RETURN;

    void saveParametricCurves(std::list<Curve >* curves) const;

    void loadParametricCurves(const std::list<Curve > & curves);

Q_SIGNALS:


    ///emitted when the state of a curve changed at the indicated dimension
    void curveChanged(int);

    void curveColorChanged(int);

private:

    virtual void onKnobAboutToAlias(const KnobIPtr& slave) OVERRIDE FINAL;
    virtual void resetExtraToDefaultValue(int dimension) OVERRIDE FINAL;
    virtual bool hasModificationsVirtual(int dimension) const OVERRIDE FINAL;
    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;
    virtual void cloneExtraData(KnobI* other, int dimension = -1, int otherDimension = -1) OVERRIDE FINAL;
    virtual bool cloneExtraDataAndCheckIfChanged(KnobI* other, int dimension = -1, int otherDimension = -1) OVERRIDE FINAL;
    virtual void cloneExtraData(KnobI* other, double offset, const RangeD* range, int dimension = -1, int otherDimension = -1) OVERRIDE FINAL;
    static const std::string _typeNameStr;
};

/**
 * @brief A Table containing strings. The number of columns is static.
 **/
class KnobTable
    : public QObject, public KnobStringBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    KnobTable(KnobHolder* holder,
              const std::string &description,
              int dimension,
              bool declaredByPlugin);

    KnobTable(KnobHolder* holder,
              const QString &description,
              int dimension,
              bool declaredByPlugin);

    virtual ~KnobTable();

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

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobLayers(holder, label, dimension, declaredByPlugin);
    }

    KnobLayers(KnobHolder* holder,
               const std::string &description,
               int dimension,
               bool declaredByPlugin)
        : KnobTable(holder, description, dimension, declaredByPlugin)
    {
    }

    virtual ~KnobLayers()
    {
    }

    virtual int getColumnsCount() const OVERRIDE FINAL
    {
        return 2;
    }

    virtual std::string getColumnLabel(int col) const OVERRIDE FINAL
    {
        if (col == 0) {
            return tr("Name").toStdString();
        } else if (col == 1) {
            return tr("Channels").toStdString();
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

private:

    virtual const std::string & typeName() const OVERRIDE FINAL
    {
        return typeNameStatic();
    }

    static const std::string _typeNameStr;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_KNOBTYPES_H
