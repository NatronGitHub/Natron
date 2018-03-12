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

#ifndef NATRON_ENGINE_SETTINGS_H
#define NATRON_ENGINE_SETTINGS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>
#include <map>
#include <vector>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/Knob.h"
#include "Engine/ChoiceOption.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER


enum KnownHostNameEnum
{
    eKnownHostNameNatron = 0,
    eKnownHostNameNuke,
    eKnownHostNameFusion,
    eKnownHostNameCatalyst,
    eKnownHostNameVegas,
    eKnownHostNameToxik,
    eKnownHostNameScratch,
    eKnownHostNameDustBuster,
    eKnownHostNameResolve,
    eKnownHostNameResolveLite,
    eKnownHostNameMistika,
    eKnownHostNamePablo,
    eKnownHostNameMotionStudio,
    eKnownHostNameShake,
    eKnownHostNameBaselight,
    eKnownHostNameFrameCycler,
    eKnownHostNameNucoda,
    eKnownHostNameAvidDS,
    eKnownHostNameDX,
    eKnownHostNameTitlerPro,
    eKnownHostNameNewBlueOFXBridge,
    eKnownHostNameRamen,
    eKnownHostNameTuttleOfx,
    eKnownHostNameNone,
};


typedef std::map<std::string, KeybindShortcut> GroupShortcutsMap;
typedef std::map<std::string, GroupShortcutsMap> ApplicationShortcutsMap;

/*The current settings in the preferences menu.*/
struct SettingsPrivate;
class Settings
    : public KnobHolder
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    enum EnableOpenGLEnum
    {
        eEnableOpenGLEnabled = 0,
        eEnableOpenGLDisabled,
        eEnableOpenGLDisabledIfBackground,
    };

private: // inherits from KnobHolder
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    Settings();

public:
    static SettingsPtr create()
    {
        return SettingsPtr( new Settings() );
    }

    virtual ~Settings();

    virtual bool canKnobsAnimate() const OVERRIDE FINAL
    {
        return false;
    }

    virtual bool onKnobValueChanged(const KnobIPtr& k,
                                    ValueChangedReasonEnum reason,
                                    TimeValue time,
                                    ViewSetSpec view) OVERRIDE FINAL;

    std::string getDiskCachePath() const;

    std::size_t getGeneralPurposeCacheSize() const;

    std::size_t getTileCacheSize() const;

    bool getColorPickerLinear() const;

    int getNumberOfThreads() const;

    void setNumberOfThreads(int threadsNb);

    void populateSystemFonts(const std::vector<std::string>& fonts);
    
    bool doesKnobChangeRequireRestart(const KnobIPtr& knob);

    bool doesKnobChangeRequireOFXCacheClear(const KnobIPtr& knob);

    // should settings be saved?
    void setSaveSettings(bool);

    bool getSaveSettings() const;

    enum LoadSettingsType
    {
        eLoadSettingsNone = 0x0,
        eLoadSettingsTypeKnobs = 0x1,
        eLoadSettingsTypePlugins = 0x2,
        eLoadSettingsTypeShortcuts = 0x4
    };


    /**
     * @brief Load the all the settings from the settings file.
     * @param loadType An or of the LoadSettingsType enum indicating what to load from
     * file.
     **/
    void loadSettingsFromFile(LoadSettingsType loadType);


    /**
     * @brief Dump all settings to the settings file.
     **/
    void saveSettingsToFile();

    std::string getSettingsAbsoluteFilePath() const;

    std::string getApplicationFontFamily() const;

    int getApplicationFontSize() const;

    bool isAutoPreviewOnForNewProjects() const;

    void getOpenFXPluginsSearchPaths(std::list<std::string>* paths) const;

    bool getUseStdOFXPluginsLocation() const;

    bool isRenderInSeparatedProcessEnabled() const;

    bool isRenderQueuingEnabled() const;

    void setRenderQueuingEnabled(bool enabled);

    void restoreAllSettingsToDefaults();

    void restorePageToDefaults(const KnobPagePtr& tab);

    int getMaximumUndoRedoNodeGraph() const;

    int getAutoSaveDelayMS() const;

    bool isAutoSaveEnabledForUnsavedProjects() const;

    bool isSnapToNodeEnabled() const;

    bool isCheckForUpdatesEnabled() const;

    void setCheckUpdatesEnabled(bool enabled);

    bool isCrashReportingEnabled() const;

    int getMaxPanelsOpened() const;

    void setMaxPanelsOpened(int maxPanels);

    bool loadBundledPlugins() const;

    bool preferBundledPlugins() const;

    void getDefaultNodeColor(float *r, float *g, float *b) const;

    void getDefaultBackdropColor(float *r, float *g, float *b) const;

    int getDisconnectedArrowLength() const;

    void getGeneratorColor(float *r, float *g, float *b) const;
    void getReaderColor(float *r, float *g, float *b) const;
    void getWriterColor(float *r, float *g, float *b) const;
    void getColorGroupColor(float *r, float *g, float *b) const;
    void getFilterGroupColor(float *r, float *g, float *b) const;
    void getTransformGroupColor(float *r, float *g, float *b) const;
    void getTimeGroupColor(float *r, float *g, float *b) const;
    void getDrawGroupColor(float *r, float *g, float *b) const;
    void getKeyerGroupColor(float *r, float *g, float *b) const;
    void getChannelGroupColor(float *r, float *g, float *b) const;
    void getMergeGroupColor(float *r, float *g, float *b) const;
    void getViewsGroupColor(float *r, float *g, float *b) const;
    void getDeepGroupColor(float *r, float *g, float *b) const;

    bool getRenderOnEditingFinishedOnly() const;
    void setRenderOnEditingFinishedOnly(bool render);

    bool getIconsBlackAndWhite() const;

    std::string getHostName() const;

    const std::string& getKnownHostName(KnownHostNameEnum e) const;


    std::string getDefaultLayoutFile() const;

    bool getLoadProjectWorkspce() const;

    bool useCursorPositionIncrements() const;

    bool isAutoProjectFormatEnabled() const;

    bool isAutoFixRelativeFilePathEnabled() const;

    ///////////////////////////////////////////////////////
    // "Viewers" pane
    KnobChoicePtr getViewerBitDepthKnob() const;
    ImageBitDepthEnum getViewersBitDepth() const;
    int getCheckerboardTileSize() const;
    void getCheckerboardColor1(double* r, double* g, double* b, double* a) const;
    void getCheckerboardColor2(double* r, double* g, double* b, double* a) const;
    bool isAutoWipeEnabled() const;
    bool isAutoProxyEnabled() const;
    unsigned int getAutoProxyMipMapLevel() const;
    int getMaxOpenedNodesViewerContext() const;
    bool isViewerKeysEnabled() const;
    ///////////////////////////////////////////////////////

    bool areRGBPixelComponentsSupported() const;

    bool isTransformConcatenationEnabled() const;

    bool useInputAForMergeAutoConnect() const;

    /**
     * @brief If the OCIO startup check parameter is set to true, warn the user if the OCIO config is different
     * from the default value held by NATRON_CUSTOM_OCIO_CONFIG_NAME
     * This can only be called once the 1st AppInstance has been loaded otherwise dialogs could not be created.
     **/
    void doOCIOStartupCheckIfNeeded();


    bool notifyOnFileChange() const;

    bool isAggressiveCachingEnabled() const;

    bool isAutoTurboEnabled() const;

    void setAutoTurboModeEnabled(bool e);

    bool isFileDialogEnabledForNewWriters() const;

    void setOptionalInputsAutoHidden(bool hidden);
    bool areOptionalInputsAutoHidden() const;

    void getPythonGroupsSearchPaths(std::list<std::string>* templates) const;
    void appendPythonGroupsPath(const std::string& path);

    std::string getOnProjectCreatedCB();
    std::string getDefaultOnProjectLoadedCB();
    std::string getDefaultOnProjectSaveCB();
    std::string getDefaultOnProjectCloseCB();
    std::string getDefaultOnNodeCreatedCB();
    std::string getDefaultOnNodeDeleteCB();

    void setOnProjectCreatedCB(const std::string& func);
    void setOnProjectLoadedCB(const std::string& func);

    bool isAutoDeclaredVariablePrintActivated() const;

    void setAutoDeclaredVariablePrintEnabled(bool enabled);

    bool isPluginIconActivatedOnNodeGraph() const;

    bool isNodeGraphAntiAliasingEnabled() const;

    void getSunkenColor(double* r, double* g, double* b) const;
    void getBaseColor(double* r, double* g, double* b) const;
    void getRaisedColor(double* r, double* g, double* b) const;
    void getSelectionColor(double* r, double* g, double* b) const;
    void getInterpolatedColor(double* r, double* g, double* b) const;
    void getKeyframeColor(double* r, double* g, double* b) const;
    void getTrackerKeyframeColor(double* r, double* g, double* b) const;
    void getExprColor(double* r, double* g, double* b) const;
    void getCloneColor(double* r, double* g, double* b) const;
    void getTextColor(double* r, double* g, double* b) const;
    void getAltTextColor(double* r, double* g, double* b) const;
    void getTimelinePlayheadColor(double* r, double* g, double* b) const;
    void getTimelineBoundsColor(double* r, double* g, double* b) const;
    void getTimelineBGColor(double* r, double* g, double* b) const;
    void getCachedFrameColor(double* r, double* g, double* b) const;
    void getAnimationModuleEditorBackgroundColor(double* r, double* g, double* b) const;
    void getAnimationModuleEditorRootRowBackgroundColor(double* r, double* g, double* b, double *a) const;
    void getAnimationModuleEditorKnobRowBackgroundColor(double* r, double* g, double* b, double *a) const;
    void getAnimationModuleEditorScaleColor(double* r, double* g, double* b) const;
    void getAnimationModuleEditorGridColor(double* r, double* g, double* b) const;
    void getSliderColor(double* r, double* g, double* b) const;

    void getSEKeywordColor(double* r, double* g, double* b) const;
    void getSEOperatorColor(double* r, double* g, double* b) const;
    void getSEBraceColor(double* r, double* g, double* b) const;
    void getSEDefClassColor(double* r, double* g, double* b) const;
    void getSEStringsColor(double* r, double* g, double* b) const;
    void getSECommentsColor(double* r, double* g, double* b) const;
    void getSESelfColor(double* r, double* g, double* b) const;
    void getSENumbersColor(double* r, double* g, double* b) const;
    void getSECurLineColor(double* r, double* g, double* b) const;

    int getSEFontSize() const;
    std::string getSEFontFamily() const;

    void getPluginIconFrameColor(int *r, int *g, int *b) const;

    bool isNaNHandlingEnabled() const;

    bool isDefaultAppearanceOutdated() const;
    void restoreDefaultAppearance();

    std::string getUserStyleSheetFilePath() const;

#ifdef NATRON_DOCUMENTATION_ONLINE
    int getDocumentationSource() const;
#endif
    int getServerPort() const;
    void setServerPort(int port) const;

    QString makeHTMLDocumentation(bool genHTML) const;

    bool isAutoScrollEnabled() const;

    GLRendererID getActiveOpenGLRendererID() const;

    bool isOpenGLRenderingEnabled() const;

    GLRendererID getOpenGLCPUDriver() const;

    int getMaxOpenGLContexts() const;

    bool isDriveLetterToUNCPathConversionEnabled() const;

    bool getIsFullRecoverySaveModeEnabled() const;

    KnobPathPtr getFileDialogFavoritePathsKnob() const;

    void addKeybind(const std::string & grouping,
                    const std::string & id,
                    const std::string& label,
                    const std::string & description,
                    const KeyboardModifiers & modifiers,
                    Key symbol,
                    const KeyboardModifiers & modifiersMask = KeyboardModifiers(eKeyboardModifierNone));

    void removeKeybind(const std::string& grouping, const std::string& id);

    /**
     * @brief Returns true if the given keyboard symbol and modifiers match the given action.
     * The symbol parameter is to be casted to the Qt::Key enum
     **/
    bool matchesKeybind(const std::string & group,
                        const std::string & actionID,
                        const KeyboardModifiers & modifiers,
                        Key symbol) const;

    bool getShortcutKeybind(const std::string & group,
                            const std::string & actionID,
                            KeyboardModifiers* modifiers,
                            Key* symbol) const;

    void setShortcutKeybind(const std::string & group,
                            const std::string & actionID,
                            const KeyboardModifiers & modifiers,
                            Key symbol);

    void restoreDefaultShortcuts();


    /**
     * @brief Register an action to the shortcut manager indicating it is using a shortcut.
     * This is used to update the action's shortcut when it gets modified by the user.
     **/
    void addShortcutAction(const std::string & grouping, const std::string & id, KeybindListenerI* action);
    void removeShortcutAction(const std::string & grouping, const std::string & id, KeybindListenerI* action);

    const ApplicationShortcutsMap& getAllShortcuts() const;

    void populateShortcuts();


Q_SIGNALS:

    void shortcutsChanged();

    void settingChanged(const KnobIPtr& knob, ValueChangedReasonEnum reason);

private:

    virtual void initializeKnobs() OVERRIDE FINAL;

    boost::scoped_ptr<SettingsPrivate> _imp;

};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_SETTINGS_H
