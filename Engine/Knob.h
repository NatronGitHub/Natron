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

#ifndef NATRON_ENGINE_KNOB_H_
#define NATRON_ENGINE_KNOB_H_

#include <vector>
#include <string>

#include "Global/GlobalDefines.h"
#include "Engine/Curve.h"

class KnobHolder;
class AppInstance;

/******************************KNOB_BASE**************************************/

class Knob : public QObject, public AnimatingParam
{
    Q_OBJECT
    
public:

    explicit Knob(KnobHolder*  holder,const std::string& description,int dimension = 1);
    
    /**
     * @brief Never delete a knob on your ownn. If you need to delete a knob pre-emptivly
     * see Knob::remove() , otherwise the KnobFactory will take care of allocation / deallocation
     * for you;
    **/
    virtual ~Knob();

    /**
     * @brief Call this if you want to remove the Knob dynamically (for example in response to
     * another knob value changed). You must not call the delete operator as the factory is
     * responsible for allocation/deallocation.
     **/
    void remove();
    
    /**
     * @brief Must return the type name of the knob. This name will be used by the KnobFactory
     * to create an instance of this knob.
    **/
    virtual std::string typeName() const = 0;

    /**
     * @brief Must return true if this knob can animate (i.e: if we can set different values depending on the time)
     * Some parameters cannot animate, for example a file selector.
     **/
    virtual bool canAnimate() const = 0;

    /**
      * @brief If the parameter is multidimensional, this is the label thats the that will be displayed
      * for a dimension.
    **/
    virtual std::string getDimensionName(int dimension) const {(void)dimension; return "";}

    /**
     * @brief Used to bracket calls to evaluateValueChange. This indicates than a series of calls will be made, and
     * the derived class can attempt to concatenate evaluations into a single one. For example to avoid multiple calls
     * to render.
    **/
    virtual void beginValueChange(AnimatingParam::ValueChangedReason reason) OVERRIDE FINAL;

    /**
     * @brief Used to bracket calls to evaluateValueChange. This indicates than a series of calls will be made, and
     * the derived class can attempt to concatenate evaluations into a single one. For example to avoid multiple calls
     * to render.
    **/
    virtual void endValueChange(AnimatingParam::ValueChangedReason reason) OVERRIDE FINAL;

    /**
     * @brief Called on project loading. This copies the value from AnimatingParam to the knob.
    **/
    void onStartupRestoration(const AnimatingParam& other);

    void turnOffAnimation() ;

    bool isAnimationEnabled() const;

    const std::string& getDescription() const;

    const std::vector<U64>& getHashVector() const;

    KnobHolder* getHolder() const;

    /**
     * @brief Called by a render task to copy all the knobs values to another separate instance
     * to decorellate the values modified by the gui (i.e the user) and the values used to render.
     * This ensures thread-safety.
    **/
    void cloneValue(const Knob& other);

    void turnOffNewLine();
    
    void setSpacingBetweenItems(int spacing);
    
    void setEnabled(bool b);
    
    void setSecret(bool b);
    
    /*Call this to change the knob name. The name is not the text label displayed on
     the GUI but what is passed to the valueChangedByUser signal. By default the
     name is the same as the description(i.e: the text label).*/
    void setName(const std::string& name);
    
    std::string getName() const;
    
    void setParentKnob(Knob* knob);
    
    Knob* getParentKnob() const;
    
    int determineHierarchySize() const;
    
    bool isSecret() const ;
    
    bool isEnabled() const;
    
    void setInsignificant(bool b);
    
    bool isPersistent() const;

    void setPersistent(bool b);

    void turnOffUndoRedo();
    
    bool canBeUndone() const;
    
    bool isInsignificant() const;
    
    void setHintToolTip(const std::string& hint);
    
    const std::string& getHintToolTip() const;


public slots:
    
    /*Set the value of the knob but does NOT emit the valueChanged signal.
     This is called by the GUI hence does not change the value of any
     render thread storage.*/
    void onValueChanged(int dimension,const Variant& variant);
    
//    void onKnobUndoneChange();
    
//    void onKnobRedoneChange();

    void onTimeChanged(SequenceTime);
        

signals:
    
    void deleted();
    
    /*Emitted when the value is changed internally by a call to setValue*/
    void valueChanged(int dimension);

    void secretChanged();
    
    void enabledChanged();
    
    void restorationComplete();
    
private:
    
    
    /** @brief This function can be implemented if you want to clone more data than just the value
     * of the knob. Cloning happens when a render request is made: all knobs values of the GUI
     * are cloned into small copies in order to be sure they will not be modified further on.
     * This function is useful if you need to copy for example an extra bit of information.
     * e.g: the File_Knob has to copy not only the QStringList containing the file names, but
     * also the sequence it has parsed.
    **/
    virtual void cloneExtraData(const Knob& other){(void)other;}
    
    /** @brief This function is called right after that the _value has changed
     * but before any signal notifying that it has changed. It can be useful
     * to do some processing to create new informations.
     * e.g: The File_Knob parses the files list to create a mapping of
     * <time,file> .
    **/
    virtual void processNewValue(){}

    /**
     * @brief Must be implemented to evaluate a value that has changed at the given dimension.
     * The reason can be of any type : time changed, user edited or plugin edited.
     **/
    virtual void evaluateValueChange(int dimension,AnimatingParam::ValueChangedReason reason) OVERRIDE FINAL;

    virtual void evaluateAnimationChange() OVERRIDE FINAL;

private:
    struct KnobPrivate;
    boost::scoped_ptr<KnobPrivate> _imp;
};

/**
 * @brief A Knob holder is a class that stores Knobs and interact with them in some way.
 * It serves 2 purpose:
 * 1) It automatically deletes the knobs, you don't have to manually call delete
 * 2) It calls a set of begin/end valueChanged whenever a knob value changed. It also
 * calls evaluate() which should then trigger an evaluation of the freshly changed value
 * (i.e force a new render).
 **/
class KnobHolder {
    
    AppInstance* _app;
    std::vector< boost::shared_ptr<Knob> > _knobs;
    bool _betweenBeginEndParamChanged;
    bool _insignificantChange;
    std::vector <Knob*> _knobsChanged;
public:

    friend class Knob;
    
    KnobHolder(AppInstance* appInstance);
    
    virtual ~KnobHolder();

    /**
     * @brief Clone each knob of "other" into this KnobHolder.
     * WARNING: other must have exactly the same number of knobs.
     **/
    void cloneKnobs(const KnobHolder& other);

    AppInstance* getApp() const {return _app;}

    int getAppAge() const;
    
    const std::vector< boost::shared_ptr<Knob> >& getKnobs() const { return _knobs; }

    void beginValuesChanged(AnimatingParam::ValueChangedReason reason, bool isSignificant);
    
    void endValuesChanged(AnimatingParam::ValueChangedReason reason);

    void refreshAfterTimeChange(SequenceTime time);
    
private:
    bool wasBeginCalled() const { return _betweenBeginEndParamChanged; }

    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() = 0;

    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    virtual void evaluate(Knob* knob,bool isSignificant) = 0;

    /**
     * @brief Should be implemented by any deriving class that maintains
     * a hash value based on the knobs.
     **/
    void invalidateHash();

    void onValueChanged(Knob* k,AnimatingParam::ValueChangedReason reason,bool isSignificant);

    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void beginKnobsValuesChanged(AnimatingParam::ValueChangedReason reason){(void)reason;}
    
    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void endKnobsValuesChanged(AnimatingParam::ValueChangedReason reason){(void)reason;}
    
    /**
     * @brief Called whenever a param changes. It calls the virtual
     * portion paramChangedByUser(...) and brackets the call by a begin/end if it was
     * not done already.
     **/
    virtual void onKnobValueChanged(Knob* k,AnimatingParam::ValueChangedReason reason){(void)k;(void)reason;}

    void triggerAutoSave();
    
    /*Add a knob to the vector. This is called by the
     Knob class.*/
    void addKnob(boost::shared_ptr<Knob> k){ _knobs.push_back(k); }
    
    /*Removes a knob to the vector. This is called by the
     Knob class.*/
    void removeKnob(Knob* k);

};


#endif // NATRON_ENGINE_KNOB_H_
