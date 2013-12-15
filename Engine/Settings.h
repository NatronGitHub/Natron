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

#ifndef NATRON_ENGINE_SETTINGS_H_
#define NATRON_ENGINE_SETTINGS_H_

#include <string>
#include <map>
#include <vector>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include "Engine/Knob.h"

/*The current settings in the preferences menu.
 @todo Move this class to QSettings instead*/


namespace Natron {
    class LibraryBinary;
}

class Tab_Knob;
class Double_Knob;
class Int_Knob;
class Bool_Knob;
class Choice_Knob;

class Settings : public KnobHolder
{
public:

    Settings(AppInstance* appInstance);
    
    virtual ~Settings(){}
        
    virtual void evaluate(Knob* /*knob*/,bool /*isSignificant*/) OVERRIDE FINAL {}
    
    virtual void initializeKnobs() OVERRIDE FINAL;
        
    virtual void onKnobValueChanged(Knob* k,Natron::ValueChangedReason reason) OVERRIDE FINAL;
    
    ///This class should go away and we should move readers to OpenFX plugins.
    class ReadersSettings{
    public:
        
        ReadersSettings();
        
        /*Returns a pluginID if it could find a decoder for the filetype,
         otherwise returns NULL.*/
        Natron::LibraryBinary* decoderForFiletype(const std::string& type) const;
        
        /*changes the decoder for files identified by the filetype*/
        void changeMapping(const std::string& filetype,Natron::LibraryBinary* decoder);
        
        /*use to initialise default mapping*/
        void fillMap(const std::map<std::string,Natron::LibraryBinary*>& defaultMap);
        
        std::vector<std::string> supportedFileTypes() const;
    private:
        
        std::map<std::string,Natron::LibraryBinary* > _fileTypesMap;
    };
    
    ///This class should go away and we should move writers to OpenFX plugins.
    class WritersSettings{
    public:
        WritersSettings();
        
        /*Returns a pluginID if it could find an encoder for the filetype,
         otherwise returns NULL.*/
        Natron::LibraryBinary* encoderForFiletype(const std::string& type) const;
        
        /*changes the encoder for files identified by the filetype*/
        void changeMapping(const std::string& filetype, Natron::LibraryBinary* encoder);
        
        /*use to initialise default mapping*/
        void fillMap(const std::map<std::string,Natron::LibraryBinary*>& defaultMap);
        
        const std::map<std::string,Natron::LibraryBinary*>& getFileTypesMap() const {return _fileTypesMap;}
        
        
        int _maximumBufferSize;
        
    private:
        
        
        std::map<std::string,Natron::LibraryBinary*> _fileTypesMap;
        
    };
    
    int getViewersBitDepth() const;
    
    double getRamMaximumPercent() const;
    
    double getRamPlaybackMaximumPercent() const;
    
    U64 getMaximumDiskCacheSize() const;
    
    bool getColorPickerLinear() const;
   
    ///save the settings to the application's settings
    void saveSettings();
    
    ///restores the settings from disk
    void restoreSettings();
    
    ///deprecated members
    ReadersSettings readersSettings;
    WritersSettings writersSettings;
    
private:
    
    boost::shared_ptr<Tab_Knob> _generalTab;
    boost::shared_ptr<Bool_Knob> _linearPickers;
    
    boost::shared_ptr<Tab_Knob> _cachingTab;
    boost::shared_ptr<Int_Knob> _maxPlayBackPercent;
    boost::shared_ptr<Int_Knob> _maxRAMPercent;
    boost::shared_ptr<Int_Knob> _maxDiskCacheGB;
    
    boost::shared_ptr<Tab_Knob> _viewersTab;
    boost::shared_ptr<Choice_Knob> _texturesMode;
};

#endif // NATRON_ENGINE_SETTINGS_H_
