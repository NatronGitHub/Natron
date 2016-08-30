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

#include "WorkspaceSerialization.h"

#include "Serialization/KnobSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
ViewportData::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
    em << left << bottom << zoomFactor << par << zoomOrPanSinceLastFit;
    em << YAML_NAMESPACE::EndSeq;
}

void
ViewportData::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsSequence() || node.size() != 5) {
        throw YAML_NAMESPACE::InvalidNode();
    }
    left = node[0].as<double>();
    bottom = node[1].as<double>();
    zoomFactor = node[2].as<double>();
    par = node[3].as<double>();
    zoomOrPanSinceLastFit = node[4].as<bool>();
}

void
PythonPanelSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::BeginMap;
    em << YAML_NAMESPACE::Key << "ScriptName" << YAML_NAMESPACE::Value << name;
    em << YAML_NAMESPACE::Key << "Func" << YAML_NAMESPACE::Value << pythonFunction;
    if (!userData.empty()) {
        em << YAML_NAMESPACE::Key << "Data" << YAML_NAMESPACE::Value << userData;
    }
    if (!knobs.empty()) {
        em << YAML_NAMESPACE::Key << "Params" << YAML_NAMESPACE::Value << YAML_NAMESPACE::BeginSeq;
        for (KnobSerializationList::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML_NAMESPACE::EndSeq;
    }
    em << YAML_NAMESPACE::EndMap;
}

void
PythonPanelSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsMap()) {
        throw YAML_NAMESPACE::InvalidNode();
    }
    name = node["ScriptName"].as<std::string>();
    pythonFunction = node["Func"].as<std::string>();
    if (node["Data"]) {
        userData = node["Data"].as<std::string>();
    }
    if (node["Params"]) {
        YAML_NAMESPACE::Node paramsNode = node["Params"];
        for (std::size_t i = 0; i < paramsNode.size(); ++i) {
            KnobSerializationPtr s(new KnobSerialization);
            s->decode(paramsNode[i]);
            knobs.push_back(s);
        }
    }

}

void
TabWidgetSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginMap;
    em << YAML_NAMESPACE::Key << "ScriptName" << YAML_NAMESPACE::Value << scriptName;
    if (currentIndex != 0) {
        em << YAML_NAMESPACE::Key << "Current" << YAML_NAMESPACE::Value << currentIndex;
    }
    if (isAnchor) {
        em << YAML_NAMESPACE::Key << "Anchor" << YAML_NAMESPACE::Value << isAnchor;
    }
    if (!tabs.empty()) {
        em << YAML_NAMESPACE::Key << "Tabs" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        for (std::list<std::string>::const_iterator it = tabs.begin(); it!=tabs.end(); ++it) {
            em << *it;
        }
        em << YAML_NAMESPACE::EndSeq;
    }
    em << YAML_NAMESPACE::EndMap;
}

void
TabWidgetSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsMap()) {
        throw YAML_NAMESPACE::InvalidNode();
    }
    scriptName = node["ScriptName"].as<std::string>();
    if (node["Current"]) {
        currentIndex = node["Current"].as<int>();
    } else {
        currentIndex = 0;
    }
    if (node["Anchor"]) {
        isAnchor = node["Anchor"].as<bool>();
    } else {
        isAnchor = false;
    }
    if (node["Tabs"]) {
        YAML_NAMESPACE::Node tabsNode = node["Tabs"];
        for (std::size_t i = 0; i < tabsNode.size(); ++i) {
            tabs.push_back(tabsNode[i].as<std::string>());
        }
    }
}

void
WidgetSplitterSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::BeginMap;
    em << YAML_NAMESPACE::Key << "Layout" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq << orientation << leftChildSize << rightChildSize << YAML_NAMESPACE::EndSeq;
    em << YAML_NAMESPACE::Key << "LeftType" << YAML_NAMESPACE::Value << leftChild.type;
    em << YAML_NAMESPACE::Key << "LeftChild" << YAML_NAMESPACE::Value;
    if (leftChild.childIsSplitter) {
        leftChild.childIsSplitter->encode(em);
    } else if (leftChild.childIsTabWidget) {
        leftChild.childIsTabWidget->encode(em);
    }
    em << YAML_NAMESPACE::Key << "RightType" << YAML_NAMESPACE::Value << rightChild.type;
    em << YAML_NAMESPACE::Key << "RightChild" << YAML_NAMESPACE::Value;
    if (rightChild.childIsSplitter) {
        rightChild.childIsSplitter->encode(em);
    } else if (rightChild.childIsTabWidget) {
        rightChild.childIsTabWidget->encode(em);
    }
    em << YAML_NAMESPACE::EndMap;
}

void
WidgetSplitterSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsMap()) {
        throw YAML_NAMESPACE::InvalidNode();
    }
    {
        YAML_NAMESPACE::Node n = node["Layout"];
        if (n.size() != 3) {
            throw YAML_NAMESPACE::InvalidNode();
        }
        orientation = n[0].as<std::string>();
        leftChildSize = n[1].as<int>();
        rightChildSize = n[2].as<int>();
    }
    leftChild.type = node["LeftType"].as<std::string>();
    if (leftChild.type == kSplitterChildTypeSplitter) {
        leftChild.childIsSplitter.reset(new WidgetSplitterSerialization);
        leftChild.childIsSplitter->decode(node["LeftChild"]);
    } else if (leftChild.type == kSplitterChildTypeTabWidget) {
        leftChild.childIsTabWidget.reset(new TabWidgetSerialization);
        leftChild.childIsTabWidget->decode(node["LeftChild"]);
    }
    rightChild.type = node["RightType"].as<std::string>();
    if (rightChild.type == kSplitterChildTypeSplitter) {
        rightChild.childIsSplitter.reset(new WidgetSplitterSerialization);
        rightChild.childIsSplitter->decode(node["RightChild"]);
    } else if (rightChild.type == kSplitterChildTypeTabWidget) {
        rightChild.childIsTabWidget.reset(new TabWidgetSerialization);
        rightChild.childIsTabWidget->decode(node["RightChild"]);
    }

}

void
WindowSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::BeginMap;
    em << YAML_NAMESPACE::Key << "Pos" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq << windowPosition[0] << windowPosition[1] << YAML_NAMESPACE::EndSeq;
    em << YAML_NAMESPACE::Key << "Size" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq << windowSize[0] << windowSize[1] << YAML_NAMESPACE::EndSeq;
    em << YAML_NAMESPACE::Key << "ChildType" << YAML_NAMESPACE::Value << childType;
    em << YAML_NAMESPACE::Key << "Child" << YAML_NAMESPACE::Value;
    if (isChildSplitter) {
        isChildSplitter->encode(em);
    } else if (isChildTabWidget) {
        isChildTabWidget->encode(em);
    } else if (!isChildSettingsPanel.empty()) {
        em << isChildSettingsPanel;
    }
    em << YAML_NAMESPACE::EndMap;
}

void
WindowSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    {
        YAML_NAMESPACE::Node n = node["Pos"];
        if (!n.IsSequence() || n.size() != 2) {
            throw YAML_NAMESPACE::InvalidNode();
        }
        windowPosition[0] = n[0].as<int>();
        windowPosition[1] = n[1].as<int>();

    }
    {
        YAML_NAMESPACE::Node n = node["Size"];
        if (!n.IsSequence() || n.size() != 2) {
            throw YAML_NAMESPACE::InvalidNode();
        }
        windowSize[0] = n[0].as<int>();
        windowSize[1] = n[1].as<int>();

    }
    childType = node["ChildType"].as<std::string>();
    YAML_NAMESPACE::Node childNode = node["Child"];
    if (childType == kSplitterChildTypeSplitter) {
        isChildSplitter.reset(new WidgetSplitterSerialization);
        isChildSplitter->decode(childNode);
    } else if (childType == kSplitterChildTypeTabWidget) {
        isChildTabWidget.reset(new TabWidgetSerialization);
        isChildTabWidget->decode(childNode);
    } else if (childType == kSplitterChildTypeSettingsPanel) {
        isChildSettingsPanel = childNode.as<std::string>();
    }
}

void
WorkspaceSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    if (_histograms.empty() && _pythonPanels.empty() && !_mainWindowSerialization && _floatingWindowsSerialization.empty()) {
        return;
    }
    em << YAML_NAMESPACE::BeginMap;
    if (!_histograms.empty()) {
        em << YAML_NAMESPACE::Key << "Histograms" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        for (std::list<std::string>::const_iterator it = _histograms.begin(); it!=_histograms.end(); ++it) {
            em << *it;
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    if (!_pythonPanels.empty()) {
        em << YAML_NAMESPACE::Key << "PyPanels" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        for (std::list<PythonPanelSerialization>::const_iterator it = _pythonPanels.begin(); it!=_pythonPanels.end(); ++it) {
            it->encode(em);
        }
        em << YAML_NAMESPACE::EndSeq;
    }
    if (_mainWindowSerialization) {
        em << YAML_NAMESPACE::Key << "MainWindow" << YAML_NAMESPACE::Value;
        _mainWindowSerialization->encode(em);
    }
    if (!_floatingWindowsSerialization.empty()) {
        em << YAML_NAMESPACE::Key << "FloatingWindows" << YAML_NAMESPACE::Value << YAML_NAMESPACE::BeginSeq;
        for (std::list<boost::shared_ptr<WindowSerialization> >::const_iterator it = _floatingWindowsSerialization.begin(); it!=_floatingWindowsSerialization.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    em << YAML_NAMESPACE::EndMap;
}

void
WorkspaceSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsMap()) {
        throw YAML_NAMESPACE::InvalidNode();
    }
    if (node["Histograms"]) {
        YAML_NAMESPACE::Node n = node["Histograms"];
        for (std::size_t i = 0; i < n.size(); ++i) {
            _histograms.push_back(n[i].as<std::string>());
        }
    }
    if (node["PyPanels"]) {
        YAML_NAMESPACE::Node n = node["PyPanels"];
        for (std::size_t i = 0; i < n.size(); ++i) {
            PythonPanelSerialization p;
            p.decode(n[i]);
            _pythonPanels.push_back(p);
        }
    }
    if (node["MainWindow"]) {
        _mainWindowSerialization.reset(new WindowSerialization);
        _mainWindowSerialization->decode(node["MainWindow"]);
    }
    if (node["FloatingWindows"]) {
        YAML_NAMESPACE::Node n = node["FloatingWindows"];
        for (std::size_t i = 0; i < n[i].size(); ++i) {
            boost::shared_ptr<WindowSerialization> p(new WindowSerialization);
            p->decode(n[i]);
            _floatingWindowsSerialization.push_back(p);
        }
    }
}

SERIALIZATION_NAMESPACE_EXIT




