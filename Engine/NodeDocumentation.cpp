/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NodePrivate.h"

#include <QTextStream>

#include "Engine/Utils.h"

NATRON_NAMESPACE_ENTER;


QString
Node::makeDocumentation(bool genHTML) const
{
    QString ret;
    QString markdown;
    QTextStream ts(&ret);
    QTextStream ms(&markdown);

    QString pluginID, pluginLabel, pluginDescription, pluginIcon;
    int majorVersion = getMajorVersion();
    int minorVersion = getMinorVersion();
    std::vector<std::string> pluginGroup;
    bool pluginDescriptionIsMarkdown = false;
    QVector<QStringList> inputs;
    QVector<QStringList> items;

    {
        PluginPtr plugin = getPlugin();
        assert(plugin);

        pluginID = QString::fromUtf8(plugin->getPluginID().c_str());
        pluginLabel =  QString::fromUtf8( Plugin::makeLabelWithoutSuffix( plugin->getPluginLabel() ).c_str());
        pluginDescription =  QString::fromUtf8( plugin->getProperty<std::string>(kNatronPluginPropDescription).c_str() );
        pluginIcon = QString::fromUtf8(plugin->getProperty<std::string>(kNatronPluginPropIconFilePath).c_str());
        pluginGroup = plugin->getPropertyN<std::string>(kNatronPluginPropGrouping);
        pluginDescriptionIsMarkdown = plugin->getProperty<bool>(kNatronPluginPropDescriptionIsMarkdown);


        for (int i = 0; i < _imp->effect->getMaxInputCount(); ++i) {
            QStringList input;
            QString optional = _imp->effect->isInputOptional(i) ? tr("Yes") : tr("No");
            input << QString::fromStdString( _imp->effect->getInputLabel(i) ) << QString::fromStdString( _imp->effect->getInputHint(i) ) << optional;
            inputs.push_back(input);

            // Don't show more than doc for 4 inputs otherwise it will just clutter the page
            if (i == 3) {
                break;
            }
        }
    }

    // check for plugin icon
    QString pluginIconUrl;
    if ( !pluginIcon.isEmpty() ) {
        QFile iconFile(pluginIcon);
        if ( iconFile.exists() ) {
            if (genHTML) {
                pluginIconUrl.append( QString::fromUtf8("/LOCAL_FILE/") );
                pluginIconUrl.append(pluginIcon);
                pluginIconUrl.replace( QString::fromUtf8("\\"), QString::fromUtf8("/") );
            } else {
                pluginIconUrl.append(pluginID);
                pluginIconUrl.append(QString::fromUtf8(".png"));
            }
        }
    }

    // check for extra markdown file
    QString extraMarkdown;
    QString pluginMD = pluginIcon;
    pluginMD.replace( QString::fromUtf8(".png"), QString::fromUtf8(".md") );
    QFile pluginMarkdownFile(pluginMD);
    if ( pluginMarkdownFile.exists() ) {
        if ( pluginMarkdownFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
            extraMarkdown = QString::fromUtf8( pluginMarkdownFile.readAll() );
            pluginMarkdownFile.close();
        }
    }

    // generate knobs info
    KnobsVec knobs = getEffectInstance()->getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {

        if ( (*it)->getIsSecret() ) {
            continue;
        }

        if ( !(*it)->isDeclaredByPlugin() && !( isPyPlug() && (*it)->isUserKnob() ) ) {
            continue;
        }

        QString knobScriptName = QString::fromUtf8( (*it)->getName().c_str() );
        QString knobLabel = QString::fromUtf8( (*it)->getLabel().c_str() );
        QString knobHint = QString::fromUtf8( (*it)->getHintToolTip().c_str() );

        if ( knobScriptName.startsWith( QString::fromUtf8("NatronOfxParam") ) || knobScriptName == QString::fromUtf8("exportAsPyPlug") ) {
            continue;
        }

        QString defValuesStr, knobType;
        std::vector<std::pair<QString, QString> > dimsDefaultValueStr;
        KnobIntPtr isInt = toKnobInt(*it);
        KnobChoicePtr isChoice = toKnobChoice(*it);
        KnobBoolPtr isBool = toKnobBool(*it);
        KnobDoublePtr isDbl = toKnobDouble(*it);
        KnobStringPtr isString = toKnobString(*it);
        KnobSeparatorPtr isSep = toKnobSeparator(*it);
        KnobButtonPtr isBtn = toKnobButton(*it);
        KnobParametricPtr isParametric = toKnobParametric(*it);
        KnobGroupPtr isGroup = toKnobGroup(*it);
        KnobPagePtr isPage = toKnobPage(*it);
        KnobColorPtr isColor = toKnobColor(*it);

        if (isInt) {
            knobType = tr("Integer");
        } else if (isChoice) {
            knobType = tr("Choice");
        } else if (isBool) {
            knobType = tr("Boolean");
        } else if (isDbl) {
            knobType = tr("Double");
        } else if (isString) {
            knobType = tr("String");
        } else if (isSep) {
            knobType = tr("Seperator");
        } else if (isBtn) {
            knobType = tr("Button");
        } else if (isParametric) {
            knobType = tr("Parametric");
        } else if (isGroup) {
            knobType = tr("Group");
        } else if (isPage) {
            knobType = tr("Page");
        } else if (isColor) {
            knobType = tr("Color");
        } else {
            knobType = tr("N/A");
        }

        if (!isGroup && !isPage) {
            for (int i = 0; i < (*it)->getNDimensions(); ++i) {
                QString valueStr;

                if (!isBtn && !isSep && !isParametric) {
                    if (isChoice) {
                        int index = isChoice->getDefaultValue(DimIdx(i));
                        std::vector<std::string> entries = isChoice->getEntries();
                        if ( (index >= 0) && ( index < (int)entries.size() ) ) {
                            valueStr = QString::fromUtf8( entries[index].c_str() );
                        }
                        std::vector<std::string> entriesHelp = isChoice->getEntriesHelp();
                        if ( entries.size() == entriesHelp.size() ) {
                            knobHint.append( QString::fromUtf8("\n\n") );
                            for (size_t i = 0; i < entries.size(); i++) {
                                QString entry = QString::fromUtf8( entries[i].c_str() );
                                QString entryHelp = QString::fromUtf8( entriesHelp[i].c_str() );
                                if (!entry.isEmpty() && !entryHelp.isEmpty() ) {
                                    knobHint.append( QString::fromUtf8("**%1**: %2\n").arg(entry).arg(entryHelp) );
                                }
                            }
                        }
                    } else if (isInt) {
                        valueStr = QString::number( isInt->getDefaultValue(DimIdx(i)) );
                    } else if (isDbl) {
                        valueStr = QString::number( isDbl->getDefaultValue(DimIdx(i)) );
                    } else if (isBool) {
                        valueStr = isBool->getDefaultValue(DimIdx(i)) ? tr("On") : tr("Off");
                    } else if (isString) {
                        valueStr = QString::fromUtf8( isString->getDefaultValue(DimIdx(i)).c_str() );
                    } else if (isColor) {
                        valueStr = QString::number( isColor->getDefaultValue(DimIdx(i)) );
                    }
                }

                dimsDefaultValueStr.push_back( std::make_pair(QString::fromUtf8( (*it)->getDimensionName(DimIdx(i)).c_str() ), valueStr) );
            }

            for (std::size_t i = 0; i < dimsDefaultValueStr.size(); ++i) {
                if ( !dimsDefaultValueStr[i].second.isEmpty() ) {
                    if (dimsDefaultValueStr.size() > 1) {
                        defValuesStr.append(dimsDefaultValueStr[i].first);
                        defValuesStr.append( QString::fromUtf8(": ") );
                    }
                    defValuesStr.append(dimsDefaultValueStr[i].second);
                    if (i < dimsDefaultValueStr.size() - 1) {
                        defValuesStr.append( QString::fromUtf8(" ") );
                    }
                }
            }
            if ( defValuesStr.isEmpty() ) {
                defValuesStr = tr("N/A");
            }
        }

        if (!isPage && !isSep && !isGroup) {
            QStringList row;
            row << knobLabel << knobScriptName << knobType << defValuesStr << knobHint;
            items.append(row);
        }
    } // for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {


    // generate plugin info
    ms << pluginLabel << "\n==========\n\n";
    if (!pluginIconUrl.isEmpty()) {
        ms << "![](" << pluginIconUrl << ")\n\n";
    }
    ms << tr("*This documentation is for version %2.%3 of %1.*").arg(pluginLabel).arg(majorVersion).arg(minorVersion) << "\n\n";

    if (!pluginDescriptionIsMarkdown) {
        if (genHTML) {
            pluginDescription = NATRON_NAMESPACE::convertFromPlainText(pluginDescription, NATRON_NAMESPACE::WhiteSpaceNormal);

            // replace URLs with links
            QRegExp re( QString::fromUtf8("((http|ftp|https)://([\\w_-]+(?:(?:\\.[\\w_-]+)+))([\\w.,@?^=%&:/~+#-]*[\\w@?^=%&/~+#-])?)") );
            pluginDescription.replace( re, QString::fromUtf8("<a href=\"\\1\">\\1</a>") );
        } else {
            pluginDescription.replace( QString::fromUtf8("\n"), QString::fromUtf8("\n\n") );
        }
    }

    ms << pluginDescription << "\n\n";

    // create markdown table
    ms << tr("Inputs") << "\n----------\n\n";
    ms << tr("Input") << " | " << tr("Description") << " | " << tr("Optional") << "\n";
    ms << "--- | --- | ---\n";
    if (inputs.size() > 0) {
        Q_FOREACH(const QStringList &input, inputs) {
            QString inputName = input.at(0);
            inputName.replace(QString::fromUtf8("\n"),QString::fromUtf8("<br />"));
            if (inputName.isEmpty()) {
                inputName = QString::fromUtf8("&nbsp;");
            }

            QString inputDesc = input.at(1);
            inputDesc.replace(QString::fromUtf8("\n"),QString::fromUtf8("<br />"));
            if (inputDesc.isEmpty()) {
                inputDesc = QString::fromUtf8("&nbsp;");
            }

            QString inputOpt = input.at(2);
            if (inputOpt.isEmpty()) {
                inputOpt = QString::fromUtf8("&nbsp;");
            }

            ms << inputName << " | " << inputDesc << " | " << inputOpt << "\n";
        }
    }
    ms << tr("Controls") << "\n----------\n\n";
    ms << tr("Label (UI Name)") << " | " << tr("Script-Name") << " | " <<tr("Type") << " | " << tr("Default-Value") << " | " << tr("Function") << "\n";
    ms << "--- | --- | --- | --- | ---\n";
    if (items.size() > 0) {
        Q_FOREACH(const QStringList &item, items) {
            QString itemLabel = item.at(0);
            itemLabel.replace(QString::fromUtf8("\n"),QString::fromUtf8("<br />"));
            if (itemLabel.isEmpty()) {
                itemLabel = QString::fromUtf8("&nbsp;");
            }

            QString itemScript = item.at(1);
            itemScript.replace(QString::fromUtf8("\n"),QString::fromUtf8("<br />"));
            if (itemScript.isEmpty()) {
                itemScript = QString::fromUtf8("&nbsp;");
            }

            QString itemType = item.at(2);
            if (itemType.isEmpty()) {
                itemType = QString::fromUtf8("&nbsp;");
            }

            QString itemDefault = item.at(3);
            itemDefault.replace(QString::fromUtf8("\n"),QString::fromUtf8("<br />"));
            if (itemDefault.isEmpty()) {
                itemDefault = QString::fromUtf8("&nbsp;");
            }

            QString itemFunction = item.at(4);
            itemFunction.replace(QString::fromUtf8("\n"),QString::fromUtf8("<br />"));
            if (itemFunction.isEmpty()) {
                itemFunction = QString::fromUtf8("&nbsp;");
            }

            ms << itemLabel << " | " << itemScript << " | " << itemType << " | " << itemDefault << " | " << itemFunction << "\n";
        }
    }

    // add extra markdown if available
    if ( !extraMarkdown.isEmpty() ) {
        ms << "\n\n";
        ms << extraMarkdown;
    }

    // output
    if (genHTML) {
        ts << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">";
        ts << "<html><head>";
        ts << "<title>" << pluginLabel << " - NATRON_DOCUMENTATION</title>";
        ts << "<link rel=\"stylesheet\" href=\"_static/default.css\" type=\"text/css\" /><link rel=\"stylesheet\" href=\"_static/style.css\" type=\"text/css\" /><script type=\"text/javascript\" src=\"_static/jquery.js\"></script><script type=\"text/javascript\" src=\"_static/dropdown.js\"></script>";
        ts << "</head><body>";
        ts << "<div class=\"related\"><h3>" << tr("Navigation") << "</h3><ul>";
        ts << "<li><a href=\"/index.html\">NATRON_DOCUMENTATION</a> &raquo;</li>";
        ts << "<li><a href=\"/_group.html\">" << tr("Reference Guide") << "</a> &raquo;</li>";
        if ( !pluginGroup.empty() ) {
            const QString group = QString::fromUtf8(pluginGroup[0].c_str());
            if (!group.isEmpty()) {
                ts << "<li><a href=\"/_group.html?id=" << group << "\">" << group << "</a> &raquo;</li>";
            }
        }
        ts << "</ul></div>";
        ts << "<div class=\"document\"><div class=\"documentwrapper\"><div class=\"body\"><div class=\"section\">";
        QString html = Markdown::convert2html(markdown);
        ts << Markdown::fixNodeHTML(html);
        ts << "</div></div></div><div class=\"clearer\"></div></div><div class=\"footer\"></div></body></html>";
    } else {
        ts << markdown;
    }
    
    return ret;
} // Node::makeDocumentation

std::string
Node::makeInfoForInput(int inputNumber) const
{
    if ( (inputNumber < -1) || ( inputNumber >= getMaxInputCount() ) ) {
        return "";
    }
    EffectInstancePtr input;
    if (inputNumber != -1) {
        input = _imp->effect->getInput(inputNumber);
    } else {
        input = _imp->effect;
    }

    if (!input) {
        return "";
    }


    ImageBitDepthEnum depth = _imp->effect->getBitDepth(inputNumber);
    TimeValue time = getApp()->getTimeLine()->currentFrame();
    std::stringstream ss;
    { // input name
        QString inputName;
        if (inputNumber != -1) {
            inputName = QString::fromUtf8( getInputLabel(inputNumber).c_str() );
        } else {
            inputName = tr("Output");
        }
        ss << "<b><font color=\"orange\">" << tr("%1:").arg(inputName).toStdString() << "</font></b><br />";
    }
    { // image format
        ss << "<b>" << tr("Image planes:").toStdString() << "</b> <font color=#c8c8c8>";
        EffectInstance::ComponentsAvailableMap availableComps;
        input->getComponentsAvailable(true, true, time, &availableComps);
        EffectInstance::ComponentsAvailableMap::iterator next = availableComps.begin();
        if ( next != availableComps.end() ) {
            ++next;
        }
        for (EffectInstance::ComponentsAvailableMap::iterator it = availableComps.begin(); it != availableComps.end(); ++it) {
            NodePtr origin = it->second.lock();
            if ( (origin.get() != this) || (inputNumber == -1) ) {
                if (origin) {
                    ss << Image::getFormatString(it->first, depth);
                    if (inputNumber != -1) {
                        ss << " " << tr("(from %1)").arg( QString::fromUtf8( origin->getLabel_mt_safe().c_str() ) ).toStdString();
                    }
                }
            }
            if ( next != availableComps.end() ) {
                if (origin) {
                    if ( (origin.get() != this) || (inputNumber == -1) ) {
                        ss << ", ";
                    }
                }
                ++next;
            }
        }
        ss << "</font><br />";
    }
    { // premult
        ImagePremultiplicationEnum premult = input->getPremult();
        QString premultStr = tr("unknown");
        switch (premult) {
            case eImagePremultiplicationOpaque:
                premultStr = tr("opaque");
                break;
            case eImagePremultiplicationPremultiplied:
                premultStr = tr("premultiplied");
                break;
            case eImagePremultiplicationUnPremultiplied:
                premultStr = tr("unpremultiplied");
                break;
        }
        ss << "<b>" << tr("Alpha premultiplication:").toStdString() << "</b> <font color=#c8c8c8>" << premultStr.toStdString() << "</font><br />";
    }
    { // par
        double par = input->getAspectRatio(-1);
        ss << "<b>" << tr("Pixel aspect ratio:").toStdString() << "</b> <font color=#c8c8c8>" << par << "</font><br />";
    }
    { // fps
        double fps = input->getFrameRate();
        ss << "<b>" << tr("Frame rate:").toStdString() << "</b> <font color=#c8c8c8>" << tr("%1fps").arg(fps).toStdString() << "</font><br />";
    }
    {
        double first = 1., last = 1.;
        input->getFrameRange_public(0, &first, &last);
        ss << "<b>" << tr("Frame range:").toStdString() << "</b> <font color=#c8c8c8>" << first << " - " << last << "</font><br />";
    }
    {
        RenderScale scale(1.);
        RectD rod;
        ActionRetCodeEnum stat = input->getRegionOfDefinition_public(0,
                                                                     time,
                                                                     scale, ViewIdx(0), &rod);
        if (!isFailureRetCode(stat)) {
            ss << "<b>" << tr("Region of Definition (at t=%1):").arg(time).toStdString() << "</b> <font color=#c8c8c8>";
            ss << tr("left = %1 bottom = %2 right = %3 top = %4").arg(rod.x1).arg(rod.y1).arg(rod.x2).arg(rod.y2).toStdString() << "</font><br />";
        }
    }
    
    return ss.str();
} // Node::makeInfoForInput

void
Node::refreshInfos()
{
    std::stringstream ssinfo;
    int maxinputs = getMaxInputCount();
    for (int i = 0; i < maxinputs; ++i) {
        std::string inputInfo = makeInfoForInput(i);
        if ( !inputInfo.empty() ) {
            ssinfo << inputInfo << "<br />";
        }
    }
    std::string outputInfo = makeInfoForInput(-1);
    ssinfo << outputInfo << "<br />";
    ssinfo << "<b>" << tr("Supports tiles:").toStdString() << "</b> <font color=#c8c8c8>";
    ssinfo << ( getCurrentSupportTiles() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
    if (_imp->effect) {
        ssinfo << "<b>" << tr("Supports multiresolution:").toStdString() << "</b> <font color=#c8c8c8>";
        ssinfo << ( _imp->effect->supportsMultiResolution() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
        ssinfo << "<b>" << tr("Supports renderscale:").toStdString() << "</b> <font color=#c8c8c8>";
        switch ( _imp->effect->supportsRenderScaleMaybe() ) {
            case EffectInstance::eSupportsMaybe:
                ssinfo << tr("Maybe").toStdString();
                break;

            case EffectInstance::eSupportsNo:
                ssinfo << tr("No").toStdString();
                break;

            case EffectInstance::eSupportsYes:
                ssinfo << tr("Yes").toStdString();
                break;
        }
        ssinfo << "</font><br />";
        ssinfo << "<b>" << tr("Supports multiple clip PARs:").toStdString() << "</b> <font color=#c8c8c8>";
        ssinfo << ( _imp->effect->supportsMultipleClipPARs() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
        ssinfo << "<b>" << tr("Supports multiple clip depths:").toStdString() << "</b> <font color=#c8c8c8>";
        ssinfo << ( _imp->effect->supportsMultipleClipDepths() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
    }
    ssinfo << "<b>" << tr("Render thread safety:").toStdString() << "</b> <font color=#c8c8c8>";
    switch ( getCurrentRenderThreadSafety() ) {
        case eRenderSafetyUnsafe:
            ssinfo << tr("Unsafe").toStdString();
            break;

        case eRenderSafetyInstanceSafe:
            ssinfo << tr("Safe").toStdString();
            break;

        case eRenderSafetyFullySafe:
            ssinfo << tr("Fully safe").toStdString();
            break;

        case eRenderSafetyFullySafeFrame:
            ssinfo << tr("Fully safe frame").toStdString();
            break;
    }
    ssinfo << "</font><br />";
    ssinfo << "<b>" << tr("OpenGL Rendering Support:").toStdString() << "</b>: <font color=#c8c8c8>";
    PluginOpenGLRenderSupport glSupport = getCurrentOpenGLRenderSupport();
    switch (glSupport) {
        case ePluginOpenGLRenderSupportNone:
            ssinfo << tr("No").toStdString();
            break;
        case ePluginOpenGLRenderSupportNeeded:
            ssinfo << tr("Yes but CPU rendering is not supported").toStdString();
            break;
        case ePluginOpenGLRenderSupportYes:
            ssinfo << tr("Yes").toStdString();
            break;
        default:
            break;
    }
    ssinfo << "</font>";
    _imp->nodeInfos.lock()->setValue( ssinfo.str() );
} // refreshInfos

NATRON_NAMESPACE_EXIT;

