//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_KNOBTYPES_H_
#define NATRON_ENGINE_KNOBTYPES_H_

#include <vector>
#include <string>

#include <QObject>

#include "Engine/Knob.h"

#include "Global/Macros.h"

/******************************INT_KNOB**************************************/

class Int_Knob: public Knob
{

    Q_OBJECT

public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Int_Knob(holder, description, dimension);
    }

    Int_Knob(KnobHolder *holder, const std::string &description, int dimension);

    void disableSlider();

    bool isSliderDisabled() const;

public:
    void setMinimum(int mini, int index = 0);

    void setMaximum(int maxi, int index = 0);

    void setDisplayMinimum(int mini, int index = 0);

    void setDisplayMaximum(int maxi, int index = 0);

    void setIncrement(int incr, int index = 0);

    void setIncrement(const std::vector<int> &incr);

    /*minis & maxis must have the same size*/
    void setMinimumsAndMaximums(const std::vector<int> &minis, const std::vector<int> &maxis);

    void setDisplayMinimumsAndMaximums(const std::vector<int> &minis, const std::vector<int> &maxis);

    const std::vector<int> &getMinimums() const;

    const std::vector<int> &getMaximums() const;

    const std::vector<int> &getIncrements() const;

    const std::vector<int> &getDisplayMinimums() const;

    const std::vector<int> &getDisplayMaximums() const;

signals:

    void minMaxChanged(int mini, int maxi, int index = 0);

    void incrementChanged(int incr, int index = 0);

private:
    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL;

    virtual bool canAnimate() const OVERRIDE FINAL;

    virtual std::string typeName() const OVERRIDE FINAL;

private:

    std::vector<int> _minimums, _maximums, _increments, _displayMins, _displayMaxs;
    bool _disableSlider;

};

/******************************BOOL_KNOB**************************************/

class Bool_Knob: public Knob
{

public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Bool_Knob(holder, description, dimension);
    }

    Bool_Knob(KnobHolder *holder, const std::string &description, int dimension);

    /// Can this type be animated?
    /// BooleanParam animation may not be quite perfect yet,
    /// @see Curve::getValueAt() for the animation code.
    static bool canAnimateStatic() { return true; }

private:

    virtual bool canAnimate() const OVERRIDE FINAL;

    virtual std::string typeName() const OVERRIDE FINAL;
};

/******************************DOUBLE_KNOB**************************************/

class Double_Knob: public Knob
{
    Q_OBJECT

public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Double_Knob(holder, description, dimension);
    }

    Double_Knob(KnobHolder *holder, const std::string &description, int dimension);


    void disableSlider();

    bool isSliderDisabled() const;

    const std::vector<double> &getMinimums() const;

    const std::vector<double> &getMaximums() const;

    const std::vector<double> &getIncrements() const;

    const std::vector<int> &getDecimals() const;

    const std::vector<double> &getDisplayMinimums() const;

    const std::vector<double> &getDisplayMaximums() const;

    void setMinimum(double mini, int index = 0);

    void setMaximum(double maxi, int index = 0);

    void setDisplayMinimum(double mini, int index = 0);

    void setDisplayMaximum(double maxi, int index = 0);

    void setIncrement(double incr, int index = 0);

    void setDecimals(int decis, int index = 0);


    /*minis & maxis must have the same size*/
    void setMinimumsAndMaximums(const std::vector<double> &minis, const std::vector<double> &maxis);

    void setDisplayMinimumsAndMaximums(const std::vector<double> &minis, const std::vector<double> &maxis);

    void setIncrement(const std::vector<double> &incr);

    void setDecimals(const std::vector<int> &decis);

signals:
    void minMaxChanged(double mini, double maxi, int index = 0);

    void incrementChanged(double incr, int index = 0);

    void decimalsChanged(int deci, int index = 0);

private:
    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL;

    virtual bool canAnimate() const OVERRIDE FINAL;

    virtual std::string typeName() const OVERRIDE FINAL;

private:

    std::vector<double> _minimums, _maximums, _increments, _displayMins, _displayMaxs;
    std::vector<int> _decimals;
    bool _disableSlider;

};

/******************************BUTTON_KNOB**************************************/

class Button_Knob: public Knob
{

public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Button_Knob(holder, description, dimension);
    }

    Button_Knob(KnobHolder  *holder, const std::string &description, int dimension);

private:
    virtual bool canAnimate() const OVERRIDE FINAL;

    virtual std::string typeName() const OVERRIDE FINAL;
};

/******************************CHOICE_KNOB**************************************/

class Choice_Knob: public Knob
{
    Q_OBJECT
public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Choice_Knob(holder, description, dimension);
    }

    Choice_Knob(KnobHolder *holder, const std::string &description, int dimension);

    /*Must be called right away after the constructor.*/
    void populate(const std::vector<std::string> &entries, const std::vector<std::string> &entriesHelp = std::vector<std::string>());

    const std::vector<std::string> &getEntries() const;

    const std::vector<std::string> &getEntriesHelp() const;

    int getActiveEntry() const;

    const std::string &getActiveEntryText() const;

    /// Can this type be animated?
    /// ChoiceParam animation may not be quite perfect yet,
    /// @see Curve::getValueAt() for the animation code.
    static bool canAnimateStatic() { return false; }

signals:

    void populated();

private:
    virtual bool canAnimate() const OVERRIDE FINAL;

    virtual std::string typeName() const OVERRIDE FINAL;

private:
    std::vector<std::string> _entries;
    std::vector<std::string> _entriesHelp;
};

/******************************SEPARATOR_KNOB**************************************/

class Separator_Knob: public Knob
{

public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Separator_Knob(holder, description, dimension);
    }

    Separator_Knob(KnobHolder *holder, const std::string &description, int dimension);

private:
    virtual bool canAnimate() const OVERRIDE FINAL;

    virtual std::string typeName() const OVERRIDE FINAL;

};
/******************************RGBA_KNOB**************************************/

/**
 * @brief A color knob with of variable dimension. Each color is a double ranging in [0. , 1.]
 * In dimension 1 the knob will have a single channel being a gray-scale
 * In dimension 3 the knob will have 3 channel R,G,B
 * In dimension 4 the knob will have R,G,B and A channels.
**/
class Color_Knob: public Knob
{
public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Color_Knob(holder, description, dimension);
    }


    Color_Knob(KnobHolder *holder, const std::string &description, int dimension);

    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL;

private:

    virtual bool canAnimate() const OVERRIDE FINAL;

    virtual std::string typeName() const OVERRIDE FINAL;
};

/******************************STRING_KNOB**************************************/
class String_Knob: public Knob
{
public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new String_Knob(holder, description, dimension);
    }

    String_Knob(KnobHolder *holder, const std::string &description, int dimension);
    std::string getString() const;

    /// Can this type be animated?
    /// String animation consists in setting constant strings at
    /// each keyframe, which are valid until the next keyframe.
    /// It can be useful for titling/subtitling.
    static bool canAnimateStatic() { return false; }

private:

    virtual bool canAnimate() const OVERRIDE FINAL;

    virtual std::string typeName() const OVERRIDE FINAL;
};

/******************************STRING_KNOB**************************************/
class Custom_Knob: public Knob
{
public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Custom_Knob(holder, description, dimension);
    }

    Custom_Knob(KnobHolder *holder, const std::string &description, int dimension);
    std::string getString() const;

    /// Can this type be animated?
    /// Custom anymation requires calling the interpolation
    /// function given by the plugin.
    static bool canAnimateStatic() { return false; }

private:

    virtual bool canAnimate() const OVERRIDE FINAL;

    virtual std::string typeName() const OVERRIDE FINAL;
};

/******************************GROUP_KNOB**************************************/
class Group_Knob: public Knob
{
    Q_OBJECT

    std::vector<Knob *> _children;
public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Group_Knob(holder, description, dimension);
    }

    Group_Knob(KnobHolder *holder, const std::string &description, int dimension);

    void addKnob(Knob *k);

    const std::vector<Knob *> &getChildren() const;

private:

    virtual bool canAnimate() const OVERRIDE FINAL;

    virtual std::string typeName() const OVERRIDE FINAL;
};

/******************************TAB_KNOB**************************************/

class Tab_Knob : public Knob
{
    Q_OBJECT
public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Tab_Knob(holder, description, dimension);
    }

    Tab_Knob(KnobHolder *holder, const std::string &description, int dimension);

    void addTab(const std::string &typeName);

    void addKnob(const std::string &tabName, Knob *k);

    const std::map<std::string, std::vector<Knob *> > &getKnobs() const;


private:
    virtual bool canAnimate() const OVERRIDE FINAL;

    virtual std::string typeName() const OVERRIDE FINAL;

private:
    std::map<std::string, std::vector<Knob *> > _knobs;
};

/******************************RichText_Knob**************************************/
class RichText_Knob : public Knob
{
public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new RichText_Knob(holder, description, dimension);
    }

    RichText_Knob(KnobHolder *holder, const std::string &description, int dimension);

    virtual bool canAnimate() const OVERRIDE FINAL;


    virtual std::string typeName() const OVERRIDE FINAL;

    std::string getString() const;
};

#endif // NATRON_ENGINE_KNOBTYPES_H_
