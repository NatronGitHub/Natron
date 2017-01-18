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
#include <QFile>

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

#pragma message WARN("TODO: restore getDefaultIsSecret from RB-2.2")
        //if ( (*it)->getDefaultIsSecret() ) {
        if ( (*it)->getIsSecret() ) {
            continue;
        }

        if ( !(*it)->isDeclaredByPlugin() && !( isPyPlug() && (*it)->isUserKnob() ) ) {
            continue;
        }

        // do not escape characters in the scriptName, since it will be put between backquotes
        QString knobScriptName = /*NATRON_NAMESPACE::convertFromPlainTextToMarkdown(*/ QString::fromUtf8( (*it)->getName().c_str() )/*, genHTML, true)*/;
        QString knobLabel = NATRON_NAMESPACE::convertFromPlainTextToMarkdown( QString::fromUtf8( (*it)->getLabel().c_str() ), genHTML, true);
        QString knobHint = NATRON_NAMESPACE::convertFromPlainTextToMarkdown( QString::fromUtf8( (*it)->getHintToolTip().c_str() ), genHTML, true);

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
                        std::vector<ChoiceOption> entries = isChoice->getEntries();
                        if ( (index >= 0) && ( index < (int)entries.size() ) ) {
                            valueStr = QString::fromUtf8( entries[index].id.c_str() );
                        }
                        bool first = true;
                        for (size_t i = 0; i < entries.size(); i++) {
                            QString entry = QString::fromUtf8( entries[i].id.c_str() );
                            QString entryHelp = QString::fromUtf8( entries[i].tooltip.c_str() );
                            if (!entry.isEmpty() && !entryHelp.isEmpty() ) {
                                if (first) {
                                    // empty line before the option descriptions
                                    if (genHTML) {
                                        knobHint.append( QString::fromUtf8("<br />") );
                                    } else {
                                        // we do a hack for multiline elements, because the markdown->rst conversion by pandoc doesn't use the line block syntax.
                                        // what we do here is put a supplementary dot at the beginning of each line, which is then converted to a pipe '|' in the
                                        // genStaticDocs.sh script by a simple sed command after converting to RsT
                                        if (!knobHint.startsWith( QString::fromUtf8(". ") )) {
                                            knobHint.prepend( QString::fromUtf8(". ") );
                                        }
                                        knobHint.append( QString::fromUtf8("\\\n") );
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
                                knobHint.append( QString::fromUtf8("**%1**: %2").arg( convertFromPlainTextToMarkdown(entry, genHTML, true) ).arg( convertFromPlainTextToMarkdown(entryHelp, genHTML, true) ) );
                            }
                        }
                    } else if (isInt) {
                        valueStr = QString::number( isInt->getDefaultValue( DimIdx(i) ) );
                    } else if (isDbl) {
                        valueStr = QString::number( isDbl->getDefaultValue( DimIdx(i) ) );
                    } else if (isBool) {
                        valueStr = isBool->getDefaultValue( DimIdx(i) ) ? tr("On") : tr("Off");
                    } else if (isString) {
                        valueStr = QString::fromUtf8( isString->getDefaultValue( DimIdx(i) ).c_str() );
                    } else if (isColor) {
                        valueStr = QString::number( isColor->getDefaultValue( DimIdx(i) ) );
                    }
                }

                dimsDefaultValueStr.push_back( std::make_pair(convertFromPlainTextToMarkdown( QString::fromUtf8( (*it)->getDimensionName( DimIdx(i) ).c_str() ), genHTML, true ),
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

        if (!isPage && !isSep && !isGroup) {
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
            // we should use the custom link "[Merge node](|http::/plugins/"PLUGINID_OFX_MERGE".html||rst::net.sf.openfx.MergePlugin|)"
            // but pandoc borks it
            ms << tr("The *%1* node is a convenience node identical to the %2, except that the operator is set to *%3* by default.")
            .arg(pluginLabel)
            .arg(genHTML ? QString::fromUtf8("<a href=\""PLUGINID_OFX_MERGE".html\">Merge node</a>") :
                 QString::fromUtf8(":ref:`"PLUGINID_OFX_MERGE"`")
                 //QString::fromUtf8("[Merge node](http::/plugins/"PLUGINID_OFX_MERGE".html)")
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
        if ( !pluginGroup.empty() ) {
            const QString group = QString::fromUtf8(pluginGroup[0].c_str());
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
               "        &#169; Copyright 2013-2017 The Natron documentation authors, licensed under CC BY-SA 4.0.\n"
               "    </div>\n"
               "  </body>\n"
               "</html>");
    } else {
        // this markdown will be processed externally by pandoc

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


    TimeValue time(getApp()->getTimeLine()->currentFrame());
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

        std::list<ImageComponents> availableLayers;
        _imp->effect->getAvailableLayers(time, ViewIdx(0), inputNumber, TreeRenderNodeArgsPtr(), &availableLayers);

        std::list<ImageComponents>::iterator next = availableLayers.begin();
        if ( next != availableLayers.end() ) {
            ++next;
        }
        for (std::list<ImageComponents>::iterator it = availableLayers.begin(); it != availableLayers.end(); ++it) {

            ss << " "  << it->getLayerName();
            if ( next != availableLayers.end() ) {
                ss << ", ";
                ++next;
            }
        }
        ss << "</font><br />";
    }
    {
        ImageBitDepthEnum depth = _imp->effect->getBitDepth(TreeRenderNodeArgsPtr(),inputNumber);
        QString depthStr = tr("unknown");
        switch (depth) {
            case eImageBitDepthByte:
                depthStr = tr("8u");
                break;
            case eImageBitDepthShort:
                depthStr = tr("16u");
                break;
            case eImageBitDepthFloat:
                depthStr = tr("32fp");
                break;
            case eImageBitDepthHalf:
                depthStr = tr("16fp");
            case eImageBitDepthNone:
                break;
        }
        ss << "<b>" << tr("BitDepth:").toStdString() << "</b> <font color=#c8c8c8>" << depthStr.toStdString() << "</font><br />";
    }
    { // premult
        ImagePremultiplicationEnum premult = input->getPremult(TreeRenderNodeArgsPtr());
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
        double par = input->getAspectRatio(TreeRenderNodeArgsPtr(),-1);
        ss << "<b>" << tr("Pixel aspect ratio:").toStdString() << "</b> <font color=#c8c8c8>" << par << "</font><br />";
    }
    { // fps
        double fps = input->getFrameRate(TreeRenderNodeArgsPtr());
        ss << "<b>" << tr("Frame rate:").toStdString() << "</b> <font color=#c8c8c8>" << tr("%1fps").arg(fps).toStdString() << "</font><br />";
    }
    {
        RangeD range = {1., 1.};
        {
            GetFrameRangeResultsPtr results;
            ActionRetCodeEnum stat = input->getFrameRange_public(TreeRenderNodeArgsPtr(), &results);
            if (!isFailureRetCode(stat)) {
                results->getFrameRangeResults(&range);
            }
        }
        ss << "<b>" << tr("Frame range:").toStdString() << "</b> <font color=#c8c8c8>" << range.min << " - " << range.max << "</font><br />";
    }

    {
        GetRegionOfDefinitionResultsPtr results;
        ActionRetCodeEnum stat = input->getRegionOfDefinition_public(time, RenderScale(1.), ViewIdx(0), TreeRenderNodeArgsPtr(), &results);
        if (!isFailureRetCode(stat)) {
            RectD rod = results->getRoD();
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
        if (!getCurrentSupportRenderScale()) {
            ssinfo << tr("No").toStdString();
        } else {
            ssinfo << tr("Yes").toStdString();
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

