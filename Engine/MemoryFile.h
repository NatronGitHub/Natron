/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#ifndef NATRON_ENGINE_MEMORYFILE_H_
#define NATRON_ENGINE_MEMORYFILE_H_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <string>

#include "Global/GlobalDefines.h"
#include "Global/Enums.h"

struct MemoryFilePrivate;

/**
 * @brief A memory file wrapper that maps a file to the virtual memory of the process.
 * This is not MT-safe.
 **/
class MemoryFile
{
public:

    enum FileOpenModeEnum
    {
        eFileOpenModeEnumIfExistsFailElseCreate = 0,

        eFileOpenModeEnumIfExistsKeepElseFail,

        eFileOpenModeEnumIfExistsKeepElseCreate,

        eFileOpenModeEnumIfExistsTruncateElseFail,

        eFileOpenModeEnumIfExistsTruncateElseCreate
    };

    /**
     * @brief Creates an empty object. No file is created/opened and no mapping is effective yet.
     * You can then call open(...) to open a file and then resize(...) it as needed.
     **/
    MemoryFile();

    /**
     * @brief The constructor attemps to create the file if the file
     * doesn't exist already. If the file is empty, this function doesn't create the mapping
     * (i.e: data() will return NULL).
     * To open the file mapping if the file didn't exist already, call resize() or call the other constructor.
     * The constructor might throw an exception upon failure to open the file.
     *
     * Note: if a constructor finishes by throwing an exception,
     * the memory associated with the object itself is cleaned up — there is no memory leak.
     * http://www.parashift.com/c++-faq-lite/ctors-can-throw.html
     **/
    MemoryFile(const std::string & filepath,
               FileOpenModeEnum open_mode);

    /**
     * @brief The constructor attemps to create the file if the file
     * doesn't exist already and creates the file mapping to the memory. The file size will be
     * resized to 'size' bytes.
     * This is equivalent to calling the other constructor and then calling resize(size)
     * The constructor might throw an exception upon failure to open the file.
     *
     * Note: if a constructor finishes by throwing an exception,
     * the memory associated with the object itself is cleaned up — there is no memory leak.
     * http://www.parashift.com/c++-faq-lite/ctors-can-throw.html
     **/
    MemoryFile(const std::string & filepath,
               size_t size,
               FileOpenModeEnum open_mode);

    /**
     * @brief The destructor closes the mapping, effectively removing the RAM portion but not the file.
     **/
    ~MemoryFile();

    /**
     * @brief Attemps to create the file if the file didn't exist already.
     * If the file did exist, this function will also map the file to memory and the data
     * can be read using the pointer returned by data(). Otherwise data() returns NULL and you
     * explicitly need to call resize(...) to create the file mapping.
     *
     * WARNING: Calling this function whilst the mapping is already opened has no effect
     * This function might throw an exception upon failure to open the file.
     **/
    void open(const std::string & filepath,FileOpenModeEnum open_mode);

    /**
     * @brief Returns a pointer to the beginning of the file,
     * if the file has been successfully opened, otherwise it returns 0.
     **/
    char* data() const;

    /**
     * @brief Changes the number of bytes of the significant
     * part of the file. The resulting size can be retrieved
     * using the "size" function.
     *
     * WARNING: Any pointer held to data() previously to calling resize()
     * will be garbage after calling this.
     *
     * This function might throw an exception upon failure to map the file or truncate the file.
     **/
    void resize(size_t new_size);

    /**
     * @brief Returns the size of the file in bytes.
     **/
    size_t size() const;

    /**
     * @brief Ensures that the backing file is in sync. with the data in memory
     **/
    bool flush();

    /**
     * @brief Returns the filepath of the backing file.
     **/
    std::string path() const;

    /**
     * @brief Removes the backing file and closes the mapping to the virtual memory.
     * After that you could re-use the object calling the open(...) function again.
     **/
    void remove();

private:

    MemoryFilePrivate* _imp;
};


#endif /* defined(NATRON_ENGINE_MEMORYFILE_H_) */
