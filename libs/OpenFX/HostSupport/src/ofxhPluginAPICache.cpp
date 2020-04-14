/*
  Software License :

  Copyright (c) 2007-2009, The Open Effects Association Ltd. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
  * Neither the name The Open Effects Association Ltd, nor the names of its 
  contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <assert.h>

#include <string>
#include <map>

// ofx
#include "ofxCore.h"
#include "ofxImageEffect.h"

// ofx host
#include "ofxhBinary.h"
#include "ofxhPropertySuite.h"
#include "ofxhClip.h"
#include "ofxhParam.h"
#include "ofxhMemory.h"
#include "ofxhImageEffect.h"
#include "ofxhPluginAPICache.h"
#include "ofxhPluginCache.h"
#include "ofxhHost.h"
#include "ofxhImageEffectAPI.h"
#include "ofxhXml.h"

namespace OFX
{
  namespace Host
  {
    namespace APICache
    {
      void PluginAPICacheI::registerInCache(OFX::Host::PluginCache &pluginCache) {
        pluginCache.registerAPICache(_apiName, _apiVersionMin, _apiVersionMax, this);
      }      

      void propertySetXMLRead(const std::string &el,
                              std::map<std::string, std::string> map,
                              Property::Set &set,
                              Property::Property *&currentProp) {
        if (el == "property") {
          std::string propName = map["name"];
          std::string propType = map["type"];
          int dimension = atoi(map["dimension"].c_str());
          
          currentProp = set.fetchProperty(propName, false);
          
          if(!currentProp) {
            if (propType == "int") {
              currentProp = new Property::Int(propName, dimension, false, 0);
            } else if (propType == "string") {
              currentProp = new Property::String(propName, dimension, false, "");
            } else if (propType == "double") {
              currentProp = new Property::Double(propName, dimension, false, 0);
            } else if (propType == "pointer") {
              currentProp = new Property::Pointer(propName, dimension, false, 0);
            }
            set.addProperty(currentProp);
          }
          return;
        }
        
        if (el == "value" && currentProp) {
          int index = atoi(map["index"].c_str());
          std::string value = map["value"];
          
          switch (currentProp->getType()) {
          case Property::eInt:
            set.setIntProperty(currentProp->getName(), atoi(value.c_str()), index);
            break;
          case Property::eString:
            set.setStringProperty(currentProp->getName(), value, index);
            break;
          case Property::eDouble:
            set.setDoubleProperty(currentProp->getName(), atof(value.c_str()), index);
            break;
          case Property::ePointer:
            break;
          default:
            break;
          }

          return;
        }

        std::cout << "got unrecognised key " << el << "\n";

        assert(false);
      }
      
      static void propertyXMLWrite(std::ostream &o, Property::Property *prop, const std::string &indent="")
      {        
        if (prop->getType() != Property::ePointer)  {
          
          o << indent << "<property "
            << XML::attribute("name", prop->getName())
            << XML::attribute("type", Property::gTypeNames[prop->getType()])
            << XML::attribute("dimension", prop->getFixedDimension()) 
            << ">\n";
          
          for (int i=0;i<prop->getDimension();i++) {
            o << indent << "  <value " 
              << XML::attribute("index", i)
              << XML::attribute("value", prop->getStringValue(i)) 
              << "/>\n";
          }
          
          o << indent << "</property>\n";
        }
      }

      void propertyXMLWrite(std::ostream &o, const Property::Set &set, const std::string &name, int indent)
      {
        Property::Property *prop = set.fetchProperty(name);

        if(prop) {
          std::string indent_prefix(indent, ' ');
          propertyXMLWrite(o, prop, indent_prefix);
        }
      }

      void propertySetXMLWrite(std::ostream &o, const Property::Set &set, int indent) 
      {
        std::string indent_prefix(indent, ' ');

        for (Property::PropertyMap::const_iterator i = set.getProperties().begin();
             i != set.getProperties().end();
             i++)
          {
            Property::Property *prop = i->second;
            propertyXMLWrite(o, prop, indent_prefix);
          }
      }

    }
  }
}
