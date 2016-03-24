/* $Id: CoinFileIO.cpp 1439 2011-06-13 16:31:21Z stefan $ */
// Copyright (C) 2005, COIN-OR.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinUtilsConfig.h"
#include "CoinFileIO.hpp"

#include "CoinError.hpp"
#include "CoinHelperFunctions.hpp"

#include <vector>
#include <cstring>

// ------ CoinFileIOBase -------

CoinFileIOBase::CoinFileIOBase (const std::string &fileName):
  fileName_ (fileName)
{}

CoinFileIOBase::~CoinFileIOBase ()
{}

const char *CoinFileIOBase::getFileName () const
{
  return fileName_.c_str ();
}


// ------------------------------------------------------
//   next we implement some subclasses of CoinFileInput 
//   for plain text and compressed files
// ------------------------------------------------------

// ------ Input for plain text ------

#include <stdio.h>

// This reads plain text files
class CoinPlainFileInput: public CoinFileInput
{
public:
  CoinPlainFileInput (const std::string &fileName):
    CoinFileInput (fileName), f_ (0)
  {
    readType_="plain";
    if (fileName!="stdin") {
      f_ = fopen (fileName.c_str (), "r");
      if (f_ == 0)
        throw CoinError ("Could not open file for reading!", 
                         "CoinPlainFileInput", 
                         "CoinPlainFileInput");
    } else {
      f_ = stdin;
    }
  }

  virtual ~CoinPlainFileInput ()
  {
    if (f_ != 0)
      fclose (f_);
  }

  virtual int read (void *buffer, int size)
  {
    return static_cast<int>(fread (buffer, 1, size, f_));
  }

  virtual char *gets (char *buffer, int size)
  {
    return fgets (buffer, size, f_);
  }

private:
  FILE *f_;
};

// ------ helper class supporting buffered gets -------

// This is a CoinFileInput class to handle cases, where the gets method
// is not easy to implement (i.e. bzlib has no equivalent to gets, and
// zlib's gzgets is extremely slow). It's subclasses only have to implement
// the readRaw method, while the read and gets methods are handled by this
// class using an internal buffer.
class CoinGetslessFileInput: public CoinFileInput
{
public:
  CoinGetslessFileInput (const std::string &fileName): 
    CoinFileInput (fileName), 
    dataBuffer_ (8*1024), 
    dataStart_ (&dataBuffer_[0]), 
    dataEnd_ (&dataBuffer_[0])
  {}

  virtual ~CoinGetslessFileInput () {}

  virtual int read (void *buffer, int size)
  {
    if (size <= 0)
      return 0;

    // return value
    int r = 0;

    // treat destination as char *
    char *dest = static_cast<char *>(buffer);

    // First consume data from buffer if available.
    if (dataStart_ < dataEnd_)
      {
	int amount = static_cast<int>(dataEnd_ - dataStart_);
	if (amount > size)
	  amount = size;

 CoinMemcpyN( dataStart_, amount, dest);

	dest += amount;
	size -= amount;

	dataStart_ += amount;

	r = amount;
      }

    // If we require more data, use readRaw.
    // We don't use the buffer here, as readRaw is ecpected to be efficient.
    if (size > 0)
      r += readRaw (dest, size);

    return r;
  }

  virtual char *gets (char *buffer, int size)
  {
    if (size <= 1)
      return 0;

    char *dest = buffer;
    char *destLast = dest + size - 2; // last position allowed to be written

    bool initiallyEmpty = (dataStart_ == dataEnd_);
    
    for (;;)
      {
	// refill dataBuffer if needed
	if (dataStart_ == dataEnd_)
	  {
	    dataStart_ = dataEnd_ = &dataBuffer_[0];
	    int count = readRaw (dataStart_, static_cast<int>(dataBuffer_.size ()));

	    // at EOF?
	    if (count <= 0) 
	      {
		*dest = 0;
		// if it was initially empty we had nothing written and should 
		// return 0, otherwise at least the buffer contents were
		// transfered and buffer has to be returned.
		return initiallyEmpty ? 0 : buffer;
	      }

	    dataEnd_ = dataStart_ + count;
	  }

	// copy character from buffer
	*dest = *dataStart_++;

	// terminate, if character was \n or bufferEnd was reached
	if (*dest == '\n' || dest == destLast)
	  {
	    *++dest = 0;
	    return buffer;
	  }

	++dest;
      } 

    // we should never reach this place
    throw CoinError ("Reached unreachable code!", 
		     "gets", 
		     "CoinGetslessFileInput");
  }

protected:
  // This should be implemented by the subclasses. It essentially behaves
  // like fread: the location pointed to by buffer should be filled with
  // size bytes. Return value is the number of bytes written (0 indicates EOF).
  virtual int readRaw (void *buffer, int size) = 0;

private:
  std::vector<char> dataBuffer_; // memory used for buffering 
  char *dataStart_; // pointer to currently buffered data
  char *dataEnd_; // pointer to "one behind last data element"
};


// -------- input for gzip compressed files -------


#ifdef COIN_HAS_ZLIB

#include <zlib.h>

// This class handles gzip'ed files using libz.
// While zlib offers the gzread and gzgets functions which do all we want, 
// the gzgets is _very_ slow as it gets single bytes via the complex gzread.
// So we use the CoinGetslessFileInput as base.
class CoinGzipFileInput: public CoinGetslessFileInput
{
public:
  CoinGzipFileInput (const std::string &fileName):
    CoinGetslessFileInput (fileName), gzf_ (0)
  {
    readType_="zlib";
    gzf_ = gzopen (fileName.c_str (), "r");
    if (gzf_ == 0)
      throw CoinError ("Could not open file for reading!", 
		       "CoinGzipFileInput", 
		       "CoinGzipFileInput");
  }

  virtual ~CoinGzipFileInput ()
  {
    if (gzf_ != 0)
      gzclose (gzf_);
  }

protected:
  virtual int readRaw (void *buffer, int size)
  {
    return gzread (gzf_, buffer, size);
  }

private:
  gzFile gzf_;
};

#endif // COIN_HAS_ZLIB


// ------- input for bzip2 compressed files ------

#ifdef COIN_HAS_BZLIB

#include <bzlib.h>

// This class handles files compressed by bzip2 using libbz.
// As bzlib has no builtin gets, we use the CoinGetslessFileInput.
class CoinBzip2FileInput: public CoinGetslessFileInput
{
public:
  CoinBzip2FileInput (const std::string &fileName):
    CoinGetslessFileInput (fileName), f_ (0), bzf_ (0)
  {
    int bzError = BZ_OK;
    readType_="bzlib";

    f_ = fopen (fileName.c_str (), "r");
    
    if (f_ != 0)
      bzf_ = BZ2_bzReadOpen (&bzError, f_, 0, 0, 0, 0);

    if (f_ == 0 || bzError != BZ_OK || bzf_ == 0)
      throw CoinError ("Could not open file for reading!", 
		       "CoinBzip2FileInput", 
		       "CoinBzip2FileInput");
  }

  virtual ~CoinBzip2FileInput ()
  {
    int bzError = BZ_OK;
    if (bzf_ != 0)
      BZ2_bzReadClose (&bzError, bzf_);

    if (f_ != 0)
      fclose (f_);
  }

protected:
  virtual int readRaw (void *buffer, int size)
  {
    int bzError = BZ_OK;
    int count = BZ2_bzRead (&bzError, bzf_, buffer, size);

    if (bzError == BZ_OK || bzError == BZ_STREAM_END)
      return count;
    
    // Error?
    return 0;
  }

private:
  FILE *f_;
  BZFILE *bzf_;
};

#endif // COIN_HAS_BZLIB


// ----- implementation of CoinFileInput's methods

/// indicates whether CoinFileInput supports gzip'ed files
bool CoinFileInput::haveGzipSupport() {
#ifdef COIN_HAS_ZLIB
  return true;
#else
  return false;
#endif
}

/// indicates whether CoinFileInput supports bzip2'ed files
bool CoinFileInput::haveBzip2Support() {
#ifdef COIN_HAS_BZLIB
  return true;
#else
  return false;
#endif
}

CoinFileInput *CoinFileInput::create (const std::string &fileName)
{
  // first try to open file, and read first bytes 
  unsigned char header[4];
  size_t count ; // So stdin will be plain file
  if (fileName!="stdin") {
    FILE *f = fopen (fileName.c_str (), "r");

    if (f == 0)
      throw CoinError ("Could not open file for reading!",
                       "create",
                       "CoinFileInput");
    count = fread (header, 1, 4, f);
    fclose (f);
  } else {
    // Reading from stdin - for moment not compressed
    count=0 ; // So stdin will be plain file
  }
  // gzip files start with the magic numbers 0x1f 0x8b
  if (count >= 2 && header[0] == 0x1f && header[1] == 0x8b)
    {
#ifdef COIN_HAS_ZLIB
      return new CoinGzipFileInput (fileName);
#else
      throw CoinError ("Cannot read gzip'ed file because zlib was "
		       "not compiled into COIN!",
		       "create",
		       "CoinFileInput");
#endif
    }

  // bzip2 files start with the string "BZh"
  if (count >= 3 && header[0] == 'B' && header[1] == 'Z' && header[2] == 'h')
    {
#ifdef COIN_HAS_BZLIB
      return new CoinBzip2FileInput (fileName);
#else
      throw CoinError ("Cannot read bzip2'ed file because bzlib was "
		       "not compiled into COIN!",
		       "create",
		       "CoinFileInput");
#endif
    }

  // fallback: probably plain text file
  return new CoinPlainFileInput (fileName);
}

CoinFileInput::CoinFileInput (const std::string &fileName): 
  CoinFileIOBase (fileName)
{}

CoinFileInput::~CoinFileInput () 
{}


// ------------------------------------------------------
//   Some subclasses of CoinFileOutput 
//   for plain text and compressed files
// ------------------------------------------------------


// -------- CoinPlainFileOutput ---------

// Class to handle output to text files without compression.
class CoinPlainFileOutput: public CoinFileOutput
{
public:
  CoinPlainFileOutput (const std::string &fileName): 
    CoinFileOutput (fileName), f_ (0)
  {
    if (fileName == "-" || fileName == "stdout") {
      f_ = stdout;
    } else {
      f_ = fopen (fileName.c_str (), "w");
      if (f_ == 0)
	throw CoinError ("Could not open file for writing!",
			 "CoinPlainFileOutput",
			 "CoinPlainFileOutput");
    }
  }

  virtual ~CoinPlainFileOutput () 
  {
    if (f_ != 0 && f_ != stdout)
      fclose (f_);
  }

  virtual int write (const void *buffer, int size)
  {
    return static_cast<int>(fwrite (buffer, 1, size, f_));
  }

  // we have something better than the default implementation
  virtual bool puts (const char *s)
  {
    return fputs (s, f_) >= 0;
  }

private:
  FILE *f_;
};


// ------- CoinGzipFileOutput ---------

#ifdef COIN_HAS_ZLIB

// no need to include the header, as this was done for the input class

// Handle output with gzip compression
class CoinGzipFileOutput: public CoinFileOutput
{
public:
  CoinGzipFileOutput (const std::string &fileName): 
    CoinFileOutput (fileName), gzf_ (0)
  {
    gzf_ = gzopen (fileName.c_str (), "w");
    if (gzf_ == 0)
      throw CoinError ("Could not open file for writing!",
		       "CoinGzipFileOutput",
		       "CoinGzipFileOutput");
  }

  virtual ~CoinGzipFileOutput () 
  {
    if (gzf_ != 0)
      gzclose (gzf_);
  }

  virtual int write (const void * buffer, int size)
  {
    return gzwrite (gzf_, const_cast<void *> (buffer), size);
  }
  
  // as zlib's gzputs is no more clever than our own, there's
  // no need to replace the default.
  
private:
  gzFile gzf_;
};

#endif  // COIN_HAS_ZLIB


// ------- CoinBzip2FileOutput -------

#ifdef COIN_HAS_BZLIB

// no need to include the header, as this was done for the input class

// Output to bzip2 compressed file
class CoinBzip2FileOutput: public CoinFileOutput
{
public:
  CoinBzip2FileOutput (const std::string &fileName): 
    CoinFileOutput (fileName), f_ (0), bzf_ (0)
  {
    int bzError = BZ_OK;

    f_ = fopen (fileName.c_str (), "w");
    
    if (f_ != 0)
      bzf_ = BZ2_bzWriteOpen (&bzError, f_, 
			      9, /* Number of 100k blocks used for compression.
				    Must be between 1 and 9 inclusive. As 9
				    gives best compression and I guess we can
				    spend some memory, we use it. */
			      0, /* verbosity */
			      30 /* suggested by bzlib manual */ );

    if (f_ == 0 || bzError != BZ_OK || bzf_ == 0)
      throw CoinError ("Could not open file for writing!",
		       "CoinBzip2FileOutput",
		       "CoinBzip2FileOutput");
  }

  virtual ~CoinBzip2FileOutput () 
  {
    int bzError = BZ_OK;
    if (bzf_ != 0)
      BZ2_bzWriteClose (&bzError, bzf_, 0, 0, 0);

    if (f_ != 0)
      fclose (f_);
  }

  virtual int write (const void *buffer, int size)
  {
    int bzError = BZ_OK;
    BZ2_bzWrite (&bzError, bzf_, const_cast<void *> (buffer), size);
    return (bzError == BZ_OK) ? size : 0;
  }
  
private:
  FILE *f_;
  BZFILE *bzf_;
};

#endif // COIN_HAS_BZLIB


// ------- implementation of CoinFileOutput's methods

bool CoinFileOutput::compressionSupported (Compression compression)
{
  switch (compression)
    {
    case COMPRESS_NONE: 
      return true;

    case COMPRESS_GZIP:
#ifdef COIN_HAS_ZLIB
      return true;
#else
      return false;
#endif

    case COMPRESS_BZIP2:
#ifdef COIN_HAS_BZLIB
      return true;
#else
      return false;
#endif

    default:
      return false;
    }
}

CoinFileOutput *CoinFileOutput::create (const std::string &fileName, 
					Compression compression)
{
  switch (compression)
    {
    case COMPRESS_NONE: 
      return new CoinPlainFileOutput (fileName);

    case COMPRESS_GZIP:
#ifdef COIN_HAS_ZLIB
      return new CoinGzipFileOutput (fileName);
#endif
      break;
      
    case COMPRESS_BZIP2:
#ifdef COIN_HAS_BZLIB
      return new CoinBzip2FileOutput (fileName);
#endif
      break;

    default:
      break;
    }

  throw CoinError ("Unsupported compression selected!",
		   "create",
		   "CoinFileOutput");
}

CoinFileOutput::CoinFileOutput (const std::string &fileName):
  CoinFileIOBase (fileName)
{}

CoinFileOutput::~CoinFileOutput ()
{}

bool CoinFileOutput::puts (const char *s)
{
  int len = static_cast<int>(strlen (s));
  if (len == 0)
    return true;

  return write (s, len) == len;
}

/*
  Tests if the given string looks like an absolute path to a file.
    - unix:	string begins with `/'
    - windows:	string begins with `\' or `drv:', where drv is a drive
		designator.
*/
bool fileAbsPath (const std::string &path)
{
  const char dirsep =  CoinFindDirSeparator() ;

  // If the first two chars are drive designators then treat it as absolute
  // path (noone in their right mind would create a file named 'Z:' on unix,
  // right?...)
  const size_t len = path.length();
  if (len >= 2 && path[1] == ':') {
    const char ch = path[0];
    if (('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z')) {
      return true;
    }
  }

  return path[0] == dirsep;
}


/*
   Tests if file readable and may change name to add 
   compression extension.  Here to get ZLIB etc in one place

   stdin goes by unmolested by all the fussing with file names. We shouldn't
   close it, either.
*/
bool fileCoinReadable(std::string & fileName, const std::string &dfltPrefix)
{
  if (fileName != "stdin")
  { const char dirsep =  CoinFindDirSeparator();
    std::string directory ;
    if (dfltPrefix == "")
    { directory = (dirsep == '/' ? "./" : ".\\") ; }
    else
    { directory = dfltPrefix ;
      if (directory[directory.length()-1] != dirsep)
      { directory += dirsep ; } }

    bool absolutePath = fileAbsPath(fileName) ;
    std::string field = fileName;

    if (absolutePath) {
      // nothing to do
    } else if (field[0]=='~') {
      char * home_dir = getenv("HOME");
      if (home_dir) {
	std::string home(home_dir);
	field=field.erase(0,1);
	fileName = home+field;
      } else {
	fileName=field;
      }
    } else {
      fileName = directory+field;
    }
  }
  // I am opening it to make sure not odd
  FILE *fp;
  if (strcmp(fileName.c_str(),"stdin")) {
    fp = fopen ( fileName.c_str(), "r" );
  } else {
    fp = stdin;
  }
#ifdef COIN_HAS_ZLIB
  if (!fp) {
    std::string fname = fileName;
    fname += ".gz";
    fp = fopen ( fname.c_str(), "r" );
    if (fp)
      fileName=fname;
  }
#endif
#ifdef COIN_HAS_BZLIB
  if (!fp) {
    std::string fname = fileName;
    fname += ".bz2";
    fp = fopen ( fname.c_str(), "r" );
    if (fp)
      fileName=fname;
  }
#endif
  if (!fp) {
    return false;
  } else {
    if (fp != stdin) {
      fclose(fp);
    }
    return true;
  }
}

