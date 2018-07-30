/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NodePrivate.h"

#include <QtCore/QTextStream>
#include <QtCore/QFile>

#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Utils.h"

NATRON_NAMESPACE_ENTER


// those parameters should be ignored (they are always secret in Natron)
#define kOCIOParamInputSpace "ocioInputSpace"
#define kOCIOParamOutputSpace "ocioOutputSpace"

// those parameters should not have their options in the help file if generating markdown,
// because the options are dinamlically constructed at run-time from the OCIO config.
#define kOCIOParamInputSpaceChoice "ocioInputSpaceIndex"
#define kOCIOParamOutputSpaceChoice "ocioOutputSpaceIndex"

#define kOCIODisplayPluginIdentifier "fr.inria.openfx.OCIODisplay"
#define kOCIODisplayParamDisplay "display"
#define kOCIODisplayParamDisplayChoice "displayIndex"
#define kOCIODisplayParamView "view"
#define kOCIODisplayParamViewChoice "viewIndex"

// not yet implemented (see OCIOCDLTransform.cpp)
//#define kOCIOCDLTransformPluginIdentifier "fr.inria.openfx.OCIOCDLTransform"
//#define kOCIOCDLTransformParamCCCID "cccId"
//#define kOCIOCDLTransformParamCCCIDChoice "cccIdIndex"

#define kOCIOFileTransformPluginIdentifier "fr.inria.openfx.OCIOFileTransform"
#define kOCIOFileTransformParamCCCID "cccId"
#define kOCIOFileTransformParamCCCIDChoice "cccIdIndex"

// genHTML: true for live HTML output for the internal web-server, false for markdown output
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
    QStringList pluginGroup;
    bool pluginDescriptionIsMarkdown = false;
    QVector<QStringList> inputs;
    QVector<QStringList> items;

    {
        QMutexLocker k(&_imp->pyPluginInfoMutex);
        if (_imp->pyPlugInfo.isPyPlug) {
            pluginID = QString::fromUtf8( _imp->pyPlugInfo.pyPlugID.c_str() );
            pluginLabel =  QString::fromUtf8( _imp->pyPlugInfo.pyPlugLabel.c_str() );
            pluginDescription = QString::fromUtf8( _imp->pyPlugInfo.pyPlugDesc.c_str() );
            pluginIcon = QString::fromUtf8( _imp->pyPlugInfo.pyPlugIconFilePath.c_str() );
            for (std::list<std::string>::const_iterator it = _imp->pyPlugInfo.pyPlugGrouping.begin() ; it != _imp->pyPlugInfo.pyPlugGrouping.end(); ++it) {
                pluginGroup.push_back(QString::fromUtf8(it->c_str()));
            }
        } else {
            pluginID = _imp->plugin->getPluginID();
            pluginLabel =  Plugin::makeLabelWithoutSuffix( _imp->plugin->getPluginLabel() );
            pluginDescription =  QString::fromUtf8( _imp->effect->getPluginDescription().c_str() );
            pluginIcon = _imp->plugin->getIconFilePath();
            pluginGroup = _imp->plugin->getGrouping();
            pluginDescriptionIsMarkdown = _imp->effect->isPluginDescriptionInMarkdown();
        }

        for (int i = 0; i < _imp->effect->getNInputs(); ++i) {
            QStringList input;
            input << convertFromPlainTextToMarkdown( QString::fromStdString( _imp->effect->getInputLabel(i) ), genHTML, true );
            input << convertFromPlainTextToMarkdown( QString::fromStdString( _imp->effect->getInputHint(i) ), genHTML, true );
            input << ( _imp->effect->isInputOptional(i) ? tr("Yes") : tr("No") );
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

        if ( (*it)->getDefaultIsSecret() ) {
            continue;
        }

        if ( !(*it)->isDeclaredByPlugin() && !( isPyPlug() && (*it)->isUserKnob() ) ) {
            continue;
        }

        // do not escape characters in the scriptName, since it will be put between backquotes
        QString knobScriptName = /*NATRON_NAMESPACE::convertFromPlainTextToMarkdown(*/ QString::fromUtf8( (*it)->getName().c_str() )/*, genHTML, true)*/;
        QString knobLabel = NATRON_NAMESPACE::convertFromPlainTextToMarkdown( QString::fromUtf8( (*it)->getLabel().c_str() ), genHTML, true);
        QString knobHint = NATRON_NAMESPACE::convertFromPlainTextToMarkdown( QString::fromUtf8( (*it)->getHintToolTip().c_str() ), genHTML, true);

        // totally ignore the documentation for these parameters (which are always secret in Natron)
        if ( knobScriptName.startsWith( QString::fromUtf8("NatronOfxParam") ) ||
             knobScriptName == QString::fromUtf8("exportAsPyPlug") ||
             knobScriptName == QString::fromUtf8(kOCIOParamInputSpace) ||
             knobScriptName == QString::fromUtf8(kOCIOParamOutputSpace) ||
             ( ( pluginID == QString::fromUtf8(kOCIODisplayPluginIdentifier) ) &&
               ( knobScriptName == QString::fromUtf8(kOCIODisplayParamDisplay) ) ) ||
             ( ( pluginID == QString::fromUtf8(kOCIODisplayPluginIdentifier) ) &&
               ( knobScriptName == QString::fromUtf8(kOCIODisplayParamView) ) ) ||
             //( ( pluginID == QString::fromUtf8(kOCIOCDLTransformPluginIdentifier) ) &&
             //  ( knobScriptName == QString::fromUtf8(kOCIOCDLTransformParamCCCID) ) ) ||
             ( ( pluginID == QString::fromUtf8(kOCIOFileTransformPluginIdentifier) ) &&
               ( knobScriptName == QString::fromUtf8(kOCIOFileTransformParamCCCID) ) ) ||
             false ) {
            continue;
        }

        QString defValuesStr, knobType;
        std::vector<std::pair<QString, QString> > dimsDefaultValueStr;
        KnobInt* isInt = dynamic_cast<KnobInt*>( it->get() );
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>( it->get() );
        KnobBool* isBool = dynamic_cast<KnobBool*>( it->get() );
        KnobDouble* isDbl = dynamic_cast<KnobDouble*>( it->get() );
        KnobString* isString = dynamic_cast<KnobString*>( it->get() );
        bool isLabel = isString && isString->isLabel();
        KnobSeparator* isSep = dynamic_cast<KnobSeparator*>( it->get() );
        KnobButton* isBtn = dynamic_cast<KnobButton*>( it->get() );
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>( it->get() );
        KnobGroup* isGroup = dynamic_cast<KnobGroup*>( it->get() );
        KnobPage* isPage = dynamic_cast<KnobPage*>( it->get() );
        KnobColor* isColor = dynamic_cast<KnobColor*>( it->get() );

        if (isInt) {
            knobType = tr("Integer");
        } else if (isChoice) {
            knobType = tr("Choice");
        } else if (isBool) {
            knobType = tr("Boolean");
        } else if (isDbl) {
            knobType = tr("Double");
        } else if (isString) {
            if (isLabel) {
                knobType = tr("Label");
            } else {
                knobType = tr("String");
            }
        } else if (isSep) {
            knobType = tr("Separator");
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
            for (int i = 0; i < (*it)->getDimension(); ++i) {
                QString valueStr;

                if (!isBtn && !isSep && !isParametric) {
                    // If this is a ChoiceParam and we are not generating live HTML doc,
                    // only add the list of entries and their halp if this node should not be
                    // ignored (eg. OCIO colorspace knobs).
                    if ( isChoice &&
                         (genHTML || !( knobScriptName == QString::fromUtf8(kOCIOParamInputSpaceChoice) ||
                                        knobScriptName == QString::fromUtf8(kOCIOParamOutputSpaceChoice) ||
                                        ( ( pluginID == QString::fromUtf8(kOCIODisplayPluginIdentifier) ) &&
                                          ( knobScriptName == QString::fromUtf8(kOCIODisplayParamDisplayChoice) ) ) ||
                                        ( ( pluginID == QString::fromUtf8(kOCIODisplayPluginIdentifier) ) &&
                                          ( knobScriptName == QString::fromUtf8(kOCIODisplayParamViewChoice) ) ) ||
                                        //( ( pluginID == QString::fromUtf8(kOCIOCDLTransformPluginIdentifier) ) &&
                                        //   ( knobScriptName == QString::fromUtf8(kOCIOCDLTransformParamCCCIDChoice) ) ) ||
                                        ( ( pluginID == QString::fromUtf8(kOCIOFileTransformPluginIdentifier) ) &&
                                          ( knobScriptName == QString::fromUtf8(kOCIOFileTransformParamCCCIDChoice) ) ) ||
                                        ( ( pluginID == QString::fromUtf8("net.fxarena.openfx.Text") ) &&
                                          ( knobScriptName == QString::fromUtf8("name") ) ) || // font family from Text plugin
                                        ( ( pluginID == QString::fromUtf8("net.fxarena.openfx.Polaroid") ) &&
                                          ( knobScriptName == QString::fromUtf8("font") ) ) || // font family from Polaroid plugin
                                        ( ( pluginID == QString::fromUtf8(PLUGINID_NATRON_PRECOMP) ) &&
                                          ( knobScriptName == QString::fromUtf8("writeNode") ) ) ||
                                        ( ( pluginID == QString::fromUtf8(PLUGINID_NATRON_ONEVIEW) ) &&
                                          ( knobScriptName == QString::fromUtf8("view") ) ) ) ) ) {
                        // see also KnobChoice::getHintToolTipFull()
                        int index = isChoice->getDefaultValue(i);
                        std::vector<ChoiceOption> entries = isChoice->getEntries_mt_safe();
                        if ( (index >= 0) && ( index < (int)entries.size() ) ) {
                            valueStr = QString::fromUtf8( entries[index].label.c_str() );
                        }
                        bool first = true;
                        for (size_t i = 0; i < entries.size(); i++) {
                            QString entryHelp = QString::fromUtf8( entries[i].tooltip.c_str() );
                            QString entry;
                            if (entries[i].id != entries[i].label) {
                                entry = QString::fromUtf8( "%1 (%2)" ).arg(QString::fromUtf8( entries[i].label.c_str() )).arg(QString::fromUtf8( entries[i].id.c_str() ));
                            } else {
                                entry = QString::fromUtf8( entries[i].label.c_str() );
                            }
                            if (!entry.isEmpty()) {
                                if (first) {
                                    // empty line before the option descriptions
                                    if (genHTML) {
                                        if ( !knobHint.isEmpty() ) {
                                            knobHint.append( QString::fromUtf8("<br />") );
                                        }
                                        // "Possible values:" clutters the documentation.
                                        //knobHint.append( tr("Possible values:") + QString::fromUtf8("<br />") );
                                    } else {
                                        // we do a hack for multiline elements, because the markdown->rst conversion by pandoc doesn't use the line block syntax.
                                        // what we do here is put a supplementary dot at the beginning of each line, which is then converted to a pipe '|' in the
                                        // genStaticDocs.sh script by a simple sed command after converting to RsT
                                        if ( !knobHint.isEmpty() ) {
                                            if (!knobHint.startsWith( QString::fromUtf8(". ") )) {
                                                knobHint.prepend( QString::fromUtf8(". ") );
                                            }
                                            knobHint.append( QString::fromUtf8("\\\n") );
                                        }
                                        // "Possible values:" clutters the documentation.
                                        //knobHint.append( QString::fromUtf8(". ") + tr("Possible values:") +  QString::fromUtf8("\\\n") );
                                    }
                                    first = false;
                                }
                                if (genHTML) {
                                    knobHint.append( QString::fromUtf8("<br />") );
                                } else {
                                    knobHint.append( QString::fromUtf8("\\\n") );
                                    // we do a hack for multiline elements, because the markdown->rst conversion by pandoc doesn't use the line block syntax.
                                    // what we do here is put a supplementary dot at the beginning of each line, which is then converted to a pipe '|' in the
                                    // genStaticDocs.sh script by a simple sed command after converting to RsT
                                    knobHint.append( QString::fromUtf8(". ") );
                                }
                                if (entryHelp.isEmpty()) {
                                    knobHint.append( QString::fromUtf8("**%1**").arg( convertFromPlainTextToMarkdown(entry, genHTML, true) ) );
                                } else {
                                    knobHint.append( QString::fromUtf8("**%1**: %2").arg( convertFromPlainTextToMarkdown(entry, genHTML, true) ).arg( convertFromPlainTextToMarkdown(entryHelp, genHTML, true) ) );
                                }
                            }
                        }

                    } else if (isInt) {
                        valueStr = QString::number( isInt->getDefaultValue(i) );
                    } else if (isDbl) {
                        valueStr = QString::number( isDbl->getDefaultValue(i) );
                    } else if (isBool) {
                        valueStr = isBool->getDefaultValue(i) ? tr("On") : tr("Off");
                    } else if (isString) {
                        valueStr = QString::fromUtf8( isString->getDefaultValue(i).c_str() );
                    } else if (isColor) {
                        valueStr = QString::number( isColor->getDefaultValue(i) );
                    }
                }

                dimsDefaultValueStr.push_back( std::make_pair(convertFromPlainTextToMarkdown( QString::fromUtf8( (*it)->getDimensionName(i).c_str() ), genHTML, true ),
                                                              convertFromPlainTextToMarkdown(valueStr, genHTML, true)) );
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

        if (!isPage && !isSep && !isGroup && !isLabel) {
            QStringList row;
            row << knobLabel << knobScriptName << knobType << defValuesStr << knobHint;
            items.append(row);
        }
    } // for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {


    // generate plugin info
    ms << tr("%1 node").arg(pluginLabel) << "\n==========\n\n";

    // a hack to avoid repeating the documentation for the various merge plugins
    if ( pluginID.startsWith( QString::fromUtf8("net.sf.openfx.Merge") ) ) {
        std::string id = pluginID.toStdString();
        std::string op;
        if (id == PLUGINID_OFX_MERGE) {
            // do nothing
        } else if (id == "net.sf.openfx.MergeDifference") {
            op = "difference (a.k.a. absminus)";
        } else if (id == "net.sf.openfx.MergeIn") {
            op = "in";
        } else if (id == "net.sf.openfx.MergeMatte") {
            op = "matte";
        } else if (id == "net.sf.openfx.MergeMax") {
            op = "max";
        } else if (id == "net.sf.openfx.MergeMin") {
            op = "min";
        } else if (id == "net.sf.openfx.MergeMultiply") {
            op = "multiply";
        } else if (id == "net.sf.openfx.MergeOut") {
            op = "out";
        } else if (id == "net.sf.openfx.MergePlus") {
            op = "plus";
        } else if (id == "net.sf.openfx.MergeScreen") {
            op = "screen";
        }
        if ( !op.empty() ) {
            // we should use the custom link "[Merge node](|http::/plugins/" PLUGINID_OFX_MERGE ".html||rst::net.sf.openfx.MergePlugin|)"
            // but pandoc borks it
            ms << tr("The *%1* node is a convenience node identical to the %2, except that the operator is set to *%3* by default.")
            .arg(pluginLabel)
            .arg(genHTML ? QString::fromUtf8("<a href=\"" PLUGINID_OFX_MERGE ".html\">Merge node</a>") :
                 QString::fromUtf8(":ref:`" PLUGINID_OFX_MERGE "`")
                 //QString::fromUtf8("[Merge node](http::/plugins/" PLUGINID_OFX_MERGE ".html)")
                 )
            .arg( QString::fromUtf8( op.c_str() ) );
            goto OUTPUT;
        }

    }

    if (!pluginIconUrl.isEmpty()) {
        // add a nonbreaking space so that pandoc doesn't use the alt-text as a caption
        // http://pandoc.org/MANUAL.html#images
        ms << "![pluginIcon](" << pluginIconUrl << ")";
        if (!genHTML) {
            // specify image width for pandoc-generated printed doc
            // (for hoedown-generated HTML, this handled by the CSS using the alt=pluginIcon attribute)
            // see http://pandoc.org/MANUAL.html#images
            // note that only % units are understood both by pandox and sphinx
            ms << "{ width=10% }";
        }
        ms << "&nbsp;\n\n"; // &nbsp; required so that there is no legend when converted to rst by pandoc
    }
    ms << tr("*This documentation is for version %2.%3 of %1.*").arg(pluginLabel).arg(majorVersion).arg(minorVersion) << "\n\n";

    ms << "\n" << tr("Description") << "\n--------------------------------------------------------------------------------\n\n";

    if (!pluginDescriptionIsMarkdown) {
        if (genHTML) {
            pluginDescription = NATRON_NAMESPACE::convertFromPlainText(pluginDescription, NATRON_NAMESPACE::WhiteSpaceNormal);

            // replace URLs with links
            QRegExp re( QString::fromUtf8("((http|ftp|https)://([\\w_-]+(?:(?:\\.[\\w_-]+)+))([\\w.,@?^=%&:/~+#-]*[\\w@?^=%&/~+#-])?)") );
            pluginDescription.replace( re, QString::fromUtf8("<a href=\"\\1\">\\1</a>") );
        } else {
            pluginDescription = convertFromPlainTextToMarkdown(pluginDescription, genHTML, false);
        }
    }

    ms << pluginDescription << "\n";

    // create markdown table
    ms << "\n" << tr("Inputs") << "\n--------------------------------------------------------------------------------\n\n";
    ms << tr("Input") << " | " << tr("Description") << " | " << tr("Optional") << "\n";
    ms << "--- | --- | ---\n";
    if (inputs.size() > 0) {
        Q_FOREACH(const QStringList &input, inputs) {
            QString inputName = input.at(0);
            QString inputDesc = input.at(1);
            QString inputOpt = input.at(2);

            ms << inputName << " | " << inputDesc << " | " << inputOpt << "\n";
        }
    }
    ms << "\n" << tr("Controls") << "\n--------------------------------------------------------------------------------\n\n";
    if (!genHTML) {
        // insert a special marker to be replaced in rst by the genStaticDocs.sh script)
        ms << "CONTROLSTABLEPROPS\n\n";
    }
    ms << tr("Parameter / script name") << " | " << tr("Type") << " | " << tr("Default") << " | " << tr("Function") << "\n";
    ms << "--- | --- | --- | ---\n";
    if (items.size() > 0) {
        Q_FOREACH(const QStringList &item, items) {
            QString itemLabel = item.at(0);
            QString itemScript = item.at(1);
            QString itemType = item.at(2);
            QString itemDefault = item.at(3);
            QString itemFunction = item.at(4);

            ms << itemLabel << " / `" << itemScript << "` | " << itemType << " | " << itemDefault << " | " << itemFunction << "\n";
        }
    }

    // add extra markdown if available
    if ( !extraMarkdown.isEmpty() ) {
        ms << "\n\n";
        ms << extraMarkdown;
    }

OUTPUT:
    // output
    if (genHTML) {
        // use hoedown to convert to HTML

        ts << ("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
               "  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
               "\n"
               "\n"
               "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
               "  <head>\n"
               "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
               "    \n"
               "    <title>") << tr("%1 node").arg(pluginLabel) << " &#8212; NATRON_DOCUMENTATION</title>\n";
        ts << ("    \n"
               "    <link rel=\"stylesheet\" href=\"_static/markdown.css\" type=\"text/css\" />\n"
               "    \n"
               "    <script type=\"text/javascript\" src=\"_static/jquery.js\"></script>\n"
               "    <script type=\"text/javascript\" src=\"_static/dropdown.js\"></script>\n"
               "    <link rel=\"index\" title=\"Index\" href=\"genindex.html\" />\n"
               "    <link rel=\"search\" title=\"Search\" href=\"search.html\" />\n"
               "  </head>\n"
               "  <body role=\"document\">\n"
               "    <div class=\"related\" role=\"navigation\" aria-label=\"related navigation\">\n"
               "      <h3>") << tr("Navigation") << "</h3>\n";
        ts << ("      <ul>\n"
               "        <li class=\"right\" style=\"margin-right: 10px\">\n"
               "          <a href=\"genindex.html\" title=\"General Index\"\n"
               "             accesskey=\"I\">") << tr("index") << "</a></li>\n";
        ts << ("        <li class=\"right\" >\n"
               "          <a href=\"py-modindex.html\" title=\"Python Module Index\"\n"
               "             >") << tr("modules") << "</a> |</li>\n";
        ts << ("        <li class=\"nav-item nav-item-0\"><a href=\"index.html\">NATRON_DOCUMENTATION</a> &#187;</li>\n"
               "          <li class=\"nav-item nav-item-1\"><a href=\"_group.html\" >") << tr("Reference Guide") << "</a> &#187;</li>\n";
        if ( !pluginGroup.isEmpty() ) {
            QString group = pluginGroup.at(0);
            if (!group.isEmpty()) {
                ts << "          <li class=\"nav-item nav-item-2\"><a href=\"_group.html?id=" << group << "\">" << group << " nodes</a> &#187;</li>";
            }
        }
        ts << ("      </ul>\n"
               "    </div>  \n"
               "\n"
               "    <div class=\"document\">\n"
               "      <div class=\"documentwrapper\">\n"
               "          <div class=\"body\" role=\"main\">\n"
               "            \n"
               "  <div class=\"section\">\n");
        QString html = Markdown::convert2html(markdown);
        ts << Markdown::fixNodeHTML(html);
        ts << ("</div>\n"
               "\n"
               "\n"
               "          </div>\n"
               "      </div>\n"
               "      <div class=\"clearer\"></div>\n"
               "    </div>\n"
               "\n"
               "    <div class=\"footer\" role=\"contentinfo\">\n"
               "        &#169; Copyright 2013-2018 The Natron documentation authors, licensed under CC BY-SA 4.0.\n"
               "    </div>\n"
               "  </body>\n"
               "</html>");
    } else {
        // this markdown will be processed externally by pandoc

        ts << markdown;
    }

    return ret;
} // Node::makeDocumentation


NATRON_NAMESPACE_EXIT
