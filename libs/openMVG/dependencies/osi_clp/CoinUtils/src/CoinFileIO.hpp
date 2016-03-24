/* $Id: CoinFileIO.hpp 1439 2011-06-13 16:31:21Z stefan $ */
// Copyright (C) 2005, COIN-OR.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinFileIO_H
#define CoinFileIO_H

#include <string>

/// Base class for FileIO classes.
class CoinFileIOBase
{
public:
  /// Constructor.
  /// @param fileName The name of the file used by this object.
  CoinFileIOBase (const std::string &fileName);

  /// Destructor.
  ~CoinFileIOBase ();

  /// Return the name of the file used by this object.
  const char *getFileName () const;

  /// Return the method of reading being used
  inline std::string getReadType () const
  { return readType_.c_str();}
protected:
  std::string readType_;
private:
  CoinFileIOBase ();
  CoinFileIOBase (const CoinFileIOBase &);

  std::string fileName_;
};

/// Abstract base class for file input classes.
class CoinFileInput: public CoinFileIOBase
{
public:
  /// indicates whether CoinFileInput supports gzip'ed files
  static bool haveGzipSupport();
  /// indicates whether CoinFileInput supports bzip2'ed files
  static bool haveBzip2Support();

  /// Factory method, that creates a CoinFileInput (more precisely
  /// a subclass of it) for the file specified. This method reads the 
  /// first few bytes of the file and determines if this is a compressed
  /// or a plain file and returns the correct subclass to handle it.
  /// If the file does not exist or uses a compression not compiled in
  /// an exception is thrown.
  /// @param fileName The file that should be read.
  static CoinFileInput *create (const std::string &fileName);

  /// Constructor (don't use this, use the create method instead).
  /// @param fileName The name of the file used by this object.
  CoinFileInput (const std::string &fileName);

  /// Destructor.
  virtual ~CoinFileInput ();

  /// Read a block of data from the file, similar to fread.
  /// @param buffer Address of a buffer to store the data into.
  /// @param size Number of bytes to read (buffer should be large enough).
  /// @return Number of bytes read.
  virtual int read (void *buffer, int size) = 0;

  /// Reads up to (size-1) characters an stores them into the buffer, 
  /// similar to fgets.
  /// Reading ends, when EOF or a newline occurs or (size-1) characters have
  /// been read. The resulting string is terminated with '\0'. If reading
  /// ends due to an encoutered newline, the '\n' is put into the buffer, 
  /// before the '\0' is appended.
  /// @param buffer The buffer to put the string into.
  /// @param size The size of the buffer in characters.
  /// @return buffer on success, or 0 if no characters have been read.
  virtual char *gets (char *buffer, int size) = 0;
};

/// Abstract base class for file output classes.
class CoinFileOutput: public CoinFileIOBase
{
public:

  /// The compression method.
  enum Compression { 
    COMPRESS_NONE = 0, ///< No compression.
    COMPRESS_GZIP = 1, ///< gzip compression.
    COMPRESS_BZIP2 = 2 ///< bzip2 compression.
  };

  /// Returns whether the specified compression method is supported 
  /// (i.e. was compiled into COIN).
  static bool compressionSupported (Compression compression);

  /// Factory method, that creates a CoinFileOutput (more precisely
  /// a subclass of it) for the file specified. If the compression method
  /// is not supported an exception is thrown (so use compressionSupported
  /// first, if this is a problem). The reason for not providing direct 
  /// access to the subclasses (and using such a method instead) is that
  /// depending on the build configuration some of the classes are not 
  /// available (or functional). This way we can handle all required ifdefs
  /// here instead of polluting other files.
  /// @param fileName The file that should be read.
  /// @param compression Compression method used.
  static CoinFileOutput *create (const std::string &fileName, 
				 Compression compression);

  /// Constructor (don't use this, use the create method instead).
  /// @param fileName The name of the file used by this object.
  CoinFileOutput (const std::string &fileName);

  /// Destructor.
  virtual ~CoinFileOutput ();

  /// Write a block of data to the file, similar to fwrite.
  /// @param buffer Address of a buffer containing the data to be written.
  /// @param size Number of bytes to write.
  /// @return Number of bytes written.
  virtual int write (const void * buffer, int size) = 0;

  /// Write a string to the file (like fputs).
  /// Just as with fputs no trailing newline is inserted!
  /// The terminating '\0' is not written to the file.
  /// The default implementation determines the length of the string
  /// and calls write on it.
  /// @param s The zero terminated string to be written.
  /// @return true on success, false on error.
  virtual bool puts (const char *s);

  /// Convenience method: just a 'puts(s.c_str())'.
  inline bool puts (const std::string &s)
  {
    return puts (s.c_str ());
  } 
};

/*! \relates CoinFileInput
    \brief Test if the given string looks like an absolute file path

    The criteria are:
    - unix: string begins with `/'
    - windows: string begins with `\' or with `drv:' (drive specifier)
*/
bool fileAbsPath (const std::string &path) ;

/*! \relates CoinFileInput
    \brief Test if the file is readable, using likely versions of the file
	   name, and return the name that worked.

   The file name is constructed from \p name using the following rules:
   <ul>
     <li> An absolute path is not modified.
     <li> If the name begins with `~', an attempt is made to replace `~'
	  with the value of the environment variable HOME.
     <li> If a default prefix (\p dfltPrefix) is provided, it is
	  prepended to the name.
   </ul>
   If the constructed file name cannot be opened, and CoinUtils was built
   with support for compressed files, fileCoinReadable will try any
   standard extensions for supported compressed files.

   The value returned in \p name is the file name that actually worked.
*/
bool fileCoinReadable(std::string &name,
		      const std::string &dfltPrefix = std::string(""));
#endif
