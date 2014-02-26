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

#ifndef NATRON_ENGINE_KNOBFILE_H_
#define NATRON_ENGINE_KNOBFILE_H_

#include <vector>
#include <map>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <boost/scoped_ptr.hpp>

#include "Engine/Knob.h"

#include "Global/Macros.h"

class StringAnimationManager;

/**
 * @brief A small class representing an element of a file name.
 * It can be either a text part, or a view part or a frame number part.
 **/
struct FileNameElement {
    
    enum Type { TEXT = 0 , SHORT_VIEW, LONG_VIEW , FRAME_NUMBER };
    
    FileNameElement(const QString& data,FileNameElement::Type type)
    : data(data)
    , type(type)
    {}
    
    QString data;
    Type type;
};

/**
 * @brief A class representing the content of a filename. 
 * Initialize it passing it a real filename and it will initialize the data structures
 * depending on the filename content.
 **/
struct FileNameContentPrivate;
class FileNameContent {
    
public:
    
    FileNameContent(const QString& absoluteFilename);
    
    ~FileNameContent();
    
    /**
     * @brief Returns the file path, e.g: /Users/Lala/Pictures/ with the trailing separator.
     **/
    const QString& getPath() const;
    
    /**
     * @brief Returns the filename without its path.
     **/
    const QString& fileName() const;
    
    /**
     * @brief Returns the absolute filename as it was given in the constructor arguments.
     **/
    const QString& absoluteFileName() const;
    
    /**
     * @brief Returns true if a single number was found in the filename.
     **/
    bool hasSingleNumber() const;
    
    /**
     * @brief Returns true if the filename is composed only of digits.
     **/
    bool isFileNameComposedOnlyOfDigits() const;
    
    /**
     * @brief Returns true if a single view indicator was found in the filename.
     **/
    bool hasSingleView() const;
    
    /**
     * @brief Returns the file pattern found in the filename with hash characters style for frame number (i.e: ###)
     * A few things about this pattern:
     * - The file path is not prepended to the file name.
     * - Each hash tag corresponding to a potential frame number will be indexed. For example,
     * The following filename:
     * my80sequence001.jpg would be transformed in my##1sequence###2.jpg.
     * This is because there may be several numbers existing in the filename, and we have
     * no clue given just this filename what actually corresponds to the frame number.
     * - Each view tag will also be indexed in the same way that the hash tags are.
     * For example, the following filename:
     * my.left.sequence.r.10.jpg would result in my.%V1.sequence.%v2.#1.jpg
     **/
    const QString& getFilePattern() const;
    
private:
    
    boost::scoped_ptr<FileNameContentPrivate> _imp;
    
};

/******************************FILE_KNOB**************************************/

class File_Knob : public Knob
{
    
    Q_OBJECT
    
public:

    static Knob *BuildKnob(KnobHolder  *holder, const std::string &description, int dimension) {
        return new File_Knob(holder, description, dimension);
    }

    File_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
    virtual ~File_Knob();

    static const std::string& typeNameStatic();
    
    void setAsInputImage() { _isInputImage = true; }
    
    bool isInputImageFile() const { return _isInputImage; }
    
    void setFiles(const QStringList& files);

    void setFiles(const std::map<int, QString>& fileSequence);
    
    void getFiles(QStringList* files);
    
    static void filesListToSequence(const QStringList& files,std::map<int, QString>* sequence);
    
    /**
     * @brief firstFrame
     * @return Returns the index of the first frame in the sequence held by this Reader.
     */
    int firstFrame() const;
    
    /**
     * @brief lastFrame
     * @return Returns the index of the last frame in the sequence held by this Reader.
     */
    int lastFrame() const;
    
    void open_file() { emit openFile(isAnimationEnabled()); }

    /**
     * @brief getRandomFrameName
     * @param f The index of the frame.
     * @return The file name associated to the frame index. Returns an empty string if it couldn't find it.
     */
    QString getRandomFrameName(int f, bool loadNearestIfNotFound) const;


    virtual bool isTypeCompatible(const Knob& other) const OVERRIDE FINAL;
    
    const StringAnimationManager& getAnimation() const { return *_animation; }
    
    /**
     * @brief Removes from str any path it may have and returns the path.
     * The file doesn't have to necessarily exist on the disk.
     * For example: /Users/Lala/Pictures/mySequence001.jpg would transform 'filename' in
     * mySequence001.jpg and would return /Users/Lala/Pictures/
     **/
    static QString removePath(QString& filename);
    
    
    ///map: < time, map < view_index, file name > >
    ///Explanation: for each frame number, there may be multiple views, each mapped to a filename.
    ///If there'are 2 views, index 0 is considered to be the left view and index 1 is considered to be the right view.
    ///If there'are multiple views, the index is corresponding to the view
    typedef std::map<int,std::map<int,QString> > FileSequence;
    
    /**
     * @brief Given a pattern string, returns a string list with all the file names
     * that matches the mattern.
     * The rules of the pattern are:
     *
     * 1) Hashes character ('#') are counted as one digit. One digit account for extra
     * 0 padding that may exist prepending the actual number. Therefore
     * /Users/Lala/Pictures/mySequence###.jpg will accept the following filenames:
     * /Users/Lala/Pictures/mySequence001.jpg
     * /Users/Lala/Pictures/mySequence100.jpg
     * /Users/Lala/Pictures/mySequence1000.jpg
     * But not /Users/Lala/Pictures/mySequence01.jpg because it has less digits than the hashes characters specified.
     * Neither /Users/Lala/Pictures/mySequence01000.jpg because it has extra padding and more digits than hashes characters specified.
     *
     * 2) The same as hashes characters can be achieved by specifying the option %0<paddingCharactersCount>d
     * Ex: /Users/Lala/Pictures/mySequence###.jpg can be achieved the same way with /Users/Lala/Pictures/mySequence%03d.jpg
     *
     * 3) %v will be replaced automatically by 'l' or 'r' to search for a matching filename.
     *  For example /Users/Lala/Pictures/mySequence###_%v.jpg would accept:
     * /Users/Lala/Pictures/mySequence001_l.jpg and /Users/Lala/Pictures/mySequence001_r.jpg
     * Note that %v in the middle of the filename can only be used with a non letter character separating
     * it from the rest of the filename. Without this restriction it makes it very difficult to find
     * matching files. Note that %v with multi-view expends for view2 , view3 etc.. and not v2, v3 ...
     *
     * %V would achieve exactly the same but would be replaced by 'left' or 'right' instead.
     * For multi-view it would expend to 'view2','view3', etc...
     *
     * @returns True if it could extract a valid sequence from the pattern (even with 1 filename in it). False if the pattern
     * is not valid.
     **/
    static bool filesListFromPattern(const QString& pattern,File_Knob::FileSequence* sequence);
    
    
signals:
    
    void openFile(bool);
    
private:
    
    virtual void keyframeRemoved_virtual(int dimension, double time) OVERRIDE FINAL;
    
    virtual void animationRemoved_virtual(int dimension) OVERRIDE FINAL;
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
    virtual void cloneExtraData(const Knob &other) OVERRIDE FINAL;
    
    ///called by setValueAtTime
    virtual Natron::Status variantToKeyFrameValue(int time,const Variant& v,double* returnValue) OVERRIDE FINAL;
    
    ///called by getValueAtTime
    virtual void variantFromInterpolatedValue(double interpolated,Variant* returnValue) const OVERRIDE FINAL;
    
    virtual void loadExtraData(const QString& str) OVERRIDE FINAL;
    
    virtual QString saveExtraData() const OVERRIDE FINAL;


    int frameCount() const;
    
    /**
     * @brief nearestFrame
     * @return Returns the index of the nearest frame in the Range [ firstFrame() - lastFrame( ].
     * @param f The index of the frame to modify.
     */
    int nearestFrame(int f) const;


    static const std::string _typeNameStr;
    StringAnimationManager* _animation;
    int _isInputImage;
};

/******************************OUTPUT_FILE_KNOB**************************************/

class OutputFile_Knob: public Knob
{
    
    Q_OBJECT
public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new OutputFile_Knob(holder, description, dimension);
    }

    OutputFile_Knob(KnobHolder *holder, const std::string &description, int dimension);

    std::string getFileName() const;

    void setFile(const QString& file);

    static const std::string& typeNameStatic();
    
    void setAsOutputImageFile() { _isOutputImage = true; }
    
    bool isOutputImageFile() const { return _isOutputImage; }
    
    void open_file() { emit openFile(_sequenceDialog && _isOutputImage); }
    
    void turnOffSequences() { _sequenceDialog = false; }
    
    bool isSequencesDialogEnabled() const { return _sequenceDialog; }
    
    virtual bool isTypeCompatible(const Knob& other) const OVERRIDE FINAL;

    /**
     * @brief Generates a filename out of a pattern
     * @see File_Knob::filesListFromPattern
     **/
    static QString generateFileNameFromPattern(const QString& pattern,int frameNumber,int viewNumber);

signals:
    
    void openFile(bool);
    
private:

    virtual bool canAnimate() const OVERRIDE FINAL;

    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    static const std::string _typeNameStr;
    bool _isOutputImage;
    bool _sequenceDialog;
};


/******************************PATH_KNOB**************************************/
class Path_Knob: public Knob
{
    
    Q_OBJECT
public:
    
    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Path_Knob(holder, description, dimension);
    }
    
    Path_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
    static const std::string& typeNameStatic();
    
    void open_file() { emit openFile(); }
    
    void setMultiPath(bool b);
    
    bool isMultiPath() const;
    
    virtual bool isTypeCompatible(const Knob& other) const OVERRIDE FINAL;

signals:
    
    void openFile();
    
private:
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    static const std::string _typeNameStr;
    bool _isMultiPath;
};

#endif // NATRON_ENGINE_KNOBFILE_H_
