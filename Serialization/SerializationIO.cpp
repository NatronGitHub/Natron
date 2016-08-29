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

#include "SerializationIO.h"

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#include "Serialization/ProjectGuiSerialization.h"
#endif

SERIALIZATION_NAMESPACE_ENTER

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT

void tryReadAndConvertOlderWorkspace(std::istream& stream, WorkspaceSerialization* obj)
{
    try {
        boost::archive::xml_iarchive iArchive(ifile);
        // Try first to load an old gui layout
        GuiLayoutSerialization s;
        iArchive >> boost::serialization::make_nvp("Layout", s);
        s.convertToWorkspaceSerialization(obj);
    } catch (...) {
        throw std::invalid_argument("Invalid workspace file");
    }

} // void tryReadAndConvertOlderWorkspace

void tryReadAndConvertOlderProject(std::istream& stream, ProjectSerialization* obj)
{
    try {
        boost::archive::xml_iarchive iArchive(ifile);
        bool bgProject;
        iArchive >> boost::serialization::make_nvp("Background_project", bgProject);
        iArchive >> boost::serialization::make_nvp("Project", obj);

        if (!bgProject) {
            ProjectGuiSerialization deprecatedGuiSerialization;
            iArchive >> boost::serialization::make_nvp("ProjectGui", deprecatedGuiSerialization);

            // Prior to this version the layout cannot be recovered (this was when Natron 1 was still in beta)
            bool isToolOld = deprecatedGuiSerialization.getVersion() < PROJECT_GUI_SERIALIZATION_MAJOR_OVERHAUL;

            if (!isToolOld) {

                // Restore the old backdrops from old version prior to Natron 1.1
                const std::list<NodeBackdropSerialization> & backdrops = deprecatedGuiSerialization.getBackdrops();
                for (std::list<NodeBackdropSerialization>::const_iterator it = backdrops.begin(); it != backdrops.end(); ++it) {

                    // Emulate the old backdrop which was not a node to a node as it is now in newer versions
                    NodeSerializationPtr node(new NodeSerialization);


                    double x, y;
                    it->getPos(x, y);
                    int w, h;
                    it->getSize(w, h);

                    KnobSerializationPtr labelSerialization = it->getLabelSerialization();
                                            bd->setPos(x, y);
                    /*bd->resize(w, h);
                    if (labelSerialization->_values[0]->_value.type == ValueSerializationStorage::eSerializationValueVariantTypeString) {
                        bd->onLabelChanged( QString::fromUtf8( labelSerialization->_values[0]->_value.value.isString.c_str() ) );
                    }
                    float r, g, b;
                    it->getColor(r, g, b);
                    QColor c;
                    c.setRgbF(r, g, b);
                    bd->setCurrentColor(c);
                    node->setLabel( it->getFullySpecifiedName() );*/

                }


                // Now convert what we can convert to our newer format...
                deprecatedGuiSerialization.convertToProjectSerialization(obj);
            }

        }
    } catch (...) {
        throw std::invalid_argument("Invalid project file");
    }
} // void tryReadAndConvertOlderProject

#endif // #ifdef NATRON_BOOST_SERIALIZATION_COMPAT

SERIALIZATION_NAMESPACE_EXIT