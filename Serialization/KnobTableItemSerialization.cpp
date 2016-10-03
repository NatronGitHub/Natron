#include "KnobTableItemSerialization.h"


GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON


SERIALIZATION_NAMESPACE_ENTER;

void
KnobTableItemSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::BeginMap;
    em << YAML::Key << "ScriptName" << YAML::Value << scriptName;
    if (label != scriptName) {
        em << YAML::Key << "Label" << YAML::Value << label;
    }
    if (!children.empty()) {
        em << YAML::Key << "Children" << YAML::Value;
        em << YAML::BeginSeq;
        for (std::list<KnobTableItemSerializationPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }
    if (!knobs.empty()) {
        em << YAML::Key << "Params" << YAML::Value << YAML::BeginSeq;
        for (KnobSerializationList::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }
    em << YAML::EndMap;
}

void
KnobTableItemSerialization::decode(const YAML::Node& node)
{
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }

    scriptName = node["ScriptName"].as<std::string>();
    if (node["Label"]) {
        label = node["Label"].as<std::string>();
    } else {
        label = scriptName;
    }
    if (node["Children"]) {
        YAML::Node childrenNode = node["Children"];
        for (std::size_t i = 0; i < childrenNode.size(); ++i) {
            KnobTableItemSerializationPtr child(new KnobTableItemSerialization);
            child->decode(childrenNode[i]);
            children.push_back(child);
        }
    }
    if (node["Params"]) {
        YAML::Node paramsNode = node["Params"];
        for (std::size_t i = 0; i < paramsNode.size(); ++i) {
            KnobSerializationPtr child(new KnobSerialization);
            child->decode(paramsNode[i]);
            knobs.push_back(child);
        }
    }
}

void
KnobItemsTableSerialization::encode(YAML::Emitter& em) const
{
    if (nodeScriptName.empty() && items.empty()) {
        return;
    }
    em << YAML::BeginMap;
    if (!nodeScriptName.empty()) {
        em << YAML::Key << "Node" << YAML::Value << nodeScriptName;
    }
    em << YAML::Key << "ID" << YAML::Value << tableIdentifier;
    if (!items.empty()) {
        em << YAML::Key << "Items" << YAML::Value << YAML::BeginSeq;
        for (std::list<KnobTableItemSerializationPtr>::const_iterator it = items.begin(); it!= items.end(); ++it) {
            (*it)->encode(em);
        }
    }
    em << YAML::EndSeq;
    em << YAML::EndMap;
}

void
KnobItemsTableSerialization::decode(const YAML::Node& node)
{
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }
    
    if (node["Node"]) {
        nodeScriptName = node["Node"].as<std::string>();
    }
    
    tableIdentifier = node["ID"].as<std::string>();
    
    if (node["Items"]) {
        YAML::Node itemsNode = node["Items"];
        for (std::size_t i = 0; i < itemsNode.size(); ++i) {
            KnobTableItemSerializationPtr s(new KnobTableItemSerialization);
            s->decode(itemsNode[i]);
            items.push_back(s);
        }
    }
}



SERIALIZATION_NAMESPACE_EXIT;